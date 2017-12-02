#ifndef SIMPLE_HTTP_REQUEST_H_
#define SIMPLE_HTTP_REQUEST_H_

#define SIMPLE_HTTP_REQUEST_VERSION_MAJOR 0
#define SIMPLE_HTTP_REQUEST_VERSION_MINOR 1
#define SIMPLE_HTTP_REQUEST_VERSION_PATCH 0

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <unistd.h>

#include "uv.h"
#include "http_parser.h"
#ifdef ENABLE_SSL
//#include "openssl/err.h"
#include "openssl/ssl.h"
#endif

namespace request {
using namespace std;

#ifndef ULLONG_MAX
# define ULLONG_MAX ((uint64_t) -1) /* 2^64-1 */
#endif

#if defined(NDEBUG)
# define ASSERT(exp)
#else
# define ASSERT(exp)  assert(exp)
#endif

// FIXME : to stderr with __LINE__, __PRETTY_FUNCTION__ ?
#define LOGE(...) LOGI(__VA_ARGS__)
void _LOGI(){}
template <typename T, typename ...Args>
void _LOGI(T t, Args && ...args)
{
  if (getenv("DEBUG_LOG")) {
    try {
      std::ofstream logFile;
      logFile.open(getenv("DEBUG_LOG"), std::ios::out | std::ios::app);
      logFile << std::forward<T>(t);
      logFile.close();
    } catch(const std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  } else
    std::cout << std::forward<T>(t);

  _LOGI(std::forward<Args>(args)...);
}
template <typename T, typename ...Args>
void LOGI(T t, Args && ...args)
{
  if (!getenv("DEBUG")) return;

  _LOGI("[" + std::to_string(getpid()) + "] ");  // insert pid
  _LOGI(std::forward<T>(t));
  _LOGI(std::forward<Args>(args)...);
  _LOGI('\n');
}


// Forward declaration
class Error;
class Response;
class SimpleHttpRequest;

template<class... T>
using Callback = std::map<std::string, std::function<void(T...)>>;

static uv_alloc_cb uvAllocCb = [](uv_handle_t* handle, size_t size, uv_buf_t* buf) {
  buf->base = (char*)malloc(size);
  buf->len = size;
};

// Error
class Error {
public:
  Error(){};
  Error(string name, string message) : name(name), message(message) {};
  string name;
  string message;
};

// Response
class Response : public stringstream {
public:
  unsigned int statusCode = 0;
  map<string, string> headers;
};

// Http Client
class SimpleHttpRequest {
 public:
  SimpleHttpRequest(const map<string, string> &options, uv_loop_t *loop) :  SimpleHttpRequest(loop) {
    this->options = options;
  }
  SimpleHttpRequest(const map<string, string> &options, const map<string, string> &requestHeaders) :  SimpleHttpRequest(uv_default_loop()) {
    this->options = options;
    this->requestHeaders = requestHeaders;
  }
  SimpleHttpRequest(const map<string, string> &options, const map<string, string> &requestHeaders, uv_loop_t *loop) :  SimpleHttpRequest(loop) {
    this->options = options;
    this->requestHeaders = requestHeaders;
  }
  SimpleHttpRequest() : SimpleHttpRequest(uv_default_loop()) {
    _defaultLoopAbsented = true;
  }
  SimpleHttpRequest(uv_loop_t *loop) : uv_loop(loop) {
    // FIXME : so far, we dont support keep-alive
    //         even thought 'HTTP/1.1'
    requestHeaders["Connection"] = "close";

    tcp.data = this;          //FIXME : use one
    connect_req.data = this;
    parser.data = this;
    write_req.data = this;
    timer.data = this;

    http_parser_init(&parser, HTTP_RESPONSE);
    http_parser_settings_init(&parser_settings);
    parser_settings.on_message_begin = [](http_parser* parser) {
      return 0;
    };
    parser_settings.on_url = [](http_parser *p, const char *buf, size_t len) {
      LOGI("Url: ", string(buf,len));
      return 0;
    };

    parser_settings.on_header_field = [](http_parser *p, const char *buf, size_t len) {
      LOGI("Header field: ", string(buf,len));
      SimpleHttpRequest *client = (SimpleHttpRequest*)p->data;
      client->lastHeaderFieldBuf = (char*)buf;
      client->lastHeaderFieldLenth = (int)len;
      return 0;
    };
    parser_settings.on_header_value = [](http_parser *p, const char* buf, size_t len) {
      LOGI("Header value: ", string(buf,len));

      SimpleHttpRequest *client = (SimpleHttpRequest*)p->data;

      //on_header_value called without on_header_field
      if (client->lastHeaderFieldBuf == NULL) {
        LOGI("broken header field.");
        return 0;
      }

      string field = string(client->lastHeaderFieldBuf, client->lastHeaderFieldLenth);
      string value = string(buf, len);

      transform(field.begin(), field.end(), field.begin(), ::tolower);
      client->response.headers[field] = value;

      client->lastHeaderFieldBuf = NULL;
      return 0;
    };
    parser_settings.on_headers_complete = [](http_parser *parser) {
      SimpleHttpRequest *client = (SimpleHttpRequest*)parser->data;
      LOGI("on_headers_complete. status code : ", std::to_string(parser->status_code));
      client->response.statusCode = parser->status_code;

      // if there's no body(301 Move Permanently or HEAD)
      int hasBody = parser->flags & F_CHUNKED ||
          (parser->content_length > 0 && parser->content_length != ULLONG_MAX);
      if (!hasBody) {
        LOGI("no body");
        return 1;
      }

      return 0;
    };

    parser_settings.on_body = [](http_parser* parser, const char* buf, size_t len) {
      SimpleHttpRequest *client = (SimpleHttpRequest*)parser->data;
      if (buf)
        client->response.rdbuf()->sputn(buf, len);

      //fprintf("Body: %.*s\n", (int)length, at);
      if (http_body_is_final(parser)) {
          LOGI("http_body_is_final");
      } else {
      }

      return 0;
    };

    parser_settings.on_message_complete = [](http_parser* parser) {
      SimpleHttpRequest *client = (SimpleHttpRequest*)parser->data;
      client->_clearTimer();

      LOGI("on_message_complete. response size: ", client->response.tellp());
      if (http_should_keep_alive(parser)) {
        LOGI("http_should_keep_alive");
        client->_clearConnection();
      }

      // response should be called after on_message_complete
      //LOGI(client->response.tellp());
      client->emit("response");

      return 0;
    };
  }
  ~SimpleHttpRequest() {}

  //FIXME : variadic ..Args using tuple.
  template <typename... Args>
  SimpleHttpRequest& emit(const string name, Args... args) {
    if (name.compare("response") == 0 && responseCallback != nullptr)
      responseCallback(std::forward<Response>(response));
    else if (name.compare("error") == 0 && errorCallback != nullptr)
      errorCallback(Error(args...));
    else if (eventListeners.count(name))
      eventListeners[name]();

    return *this;
  }

  SimpleHttpRequest& on(const string name, std::function<void(Response&&)> func) {
    if (name.compare("response") == 0)
      responseCallback = func;

    return *this;
  }
  SimpleHttpRequest& on(const string name, std::function<void(Error&&)> func) {
    if (name.compare("error") == 0)
      errorCallback = func;

    return *this;
  }
  template <typename... Args>
  SimpleHttpRequest& on(const string name, std::function<void(Args...)> func) {
    eventListeners[name] = func;

    return *this;
  }
  SimpleHttpRequest& on(const string name, std::function<void()> func) {
    eventListeners[name] = func;

    return *this;
  }

  //FIXME : send directly later when tcp is open
  SimpleHttpRequest& write(const std::istream &stream) {
    this->requestHeaders["content-type"] = "application/octet-stream";
    requestBody << stream.rdbuf();

    return *this;
  }
  SimpleHttpRequest& write(const char* s, std::streamsize n) {
    requestBody.rdbuf()->sputn(s, n);
    return *this;
  }
  SimpleHttpRequest& write(const string &data) {
    requestBody << data;

    return *this;
  }

  SimpleHttpRequest& setHeader(const string &field, const string &value) {
    this->requestHeaders[field] = value;

    return *this;
  }

  // GET/HEAD verb
  SimpleHttpRequest& get(const string &url) {
    return _get(url, "GET");
  }
  SimpleHttpRequest& head(const string &url) {
    return _get(url, "HEAD");
  }

  // POST/PUT verbs with payloads. 'DELETE' is reserved in c. so, use 'del()'
  SimpleHttpRequest& post(const string &url, const string &body) {
    return _post(url, "POST", body);
  }
  SimpleHttpRequest& put(const string &url, const string &body) {
    return _post(url, "PUT", body);
  }
  SimpleHttpRequest& del(const string &url, const string &body) {
    return _post(url, "DELETE", body);
  }

  void end() {
    int r = 0;
    r = uv_timer_init(uv_loop, &timer);
    ASSERT(r == 0);
    r = uv_timer_start(&timer, [](uv_timer_t* timer){
      LOGE("timeout fired");
      SimpleHttpRequest *client = (SimpleHttpRequest*)timer->data;
      client->_clearTimer();
      client->_clearConnection();

      int _err = -ETIMEDOUT;
      client->emit("error", uv_err_name(_err), uv_strerror(_err));
    }, timeout, 0);
    ASSERT(r == 0);

    // determine 'hostname' whether IP address or Domain
    struct sockaddr_in dest;
    r = uv_ip4_addr(options["hostname"].c_str(), stoi(options["port"]), &dest);
    if (r != 0) {
      struct addrinfo hints;

      memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;

      r = uv_getaddrinfo(uv_loop,
                         &getaddrinfo_req,
                         NULL,  // sync
                         options["hostname"].c_str(),
                         options["port"].c_str(),
                         &hints);
      if (r != 0) {
        LOGE("uv_getaddrinfo");
        this->emit("error", uv_err_name(r), uv_strerror(r));
        return;
      }

      char ip[17] {'\0'};
      r = uv_ip4_name((struct sockaddr_in*) getaddrinfo_req.addrinfo->ai_addr, ip, 16);
      LOGI("hostname : ", options["hostname"], ", ip : ", ip);
      uv_freeaddrinfo(getaddrinfo_req.addrinfo);

      r = uv_ip4_addr(ip, stoi(options["port"]), &dest);
      if (r != 0) {
        LOGE("uv_ip4_addr");
        this->emit("error", uv_err_name(r), uv_strerror(r));
        return;
      }

      // let server knows the domain.
      requestHeaders["host"] = options["hostname"];
    }

#ifdef ENABLE_SSL
    // SSL
    if (options.count("protocol") && ( options["protocol"].compare("https") == 0
                                    || options["protocol"].compare("https:") == 0)) {
      sslTimer.data = this;

      // temporary async by timer :)
      r = uv_timer_init(uv_loop, &sslTimer);
      ASSERT(r == 0);
      r = uv_timer_start(&sslTimer, [](uv_timer_t* timer){
        SimpleHttpRequest *client = (SimpleHttpRequest*)timer->data;
        uv_close((uv_handle_t*)&client->sslTimer, [](uv_handle_t*){});

        // ERR_load_crypto_strings();
        // ERR_load_SSL_strings();
        SSL_library_init();
        client->ctx = SSL_CTX_new(SSLv23_client_method());
        client->sbio = BIO_new_ssl_connect(client->ctx);
        BIO_get_ssl(client->sbio, &client->ssl);

        if (!client->ssl) {
          LOGE("ssl - Can't locate SSL pointer");
          client->emit("error", "SSL", "Can't locate SSL pointer");
          return;
        }

        /* Don't want any retries */
        SSL_set_mode(client->ssl, SSL_MODE_AUTO_RETRY);

        /* We might want to do other things with ssl here */
        BIO_set_conn_hostname(client->sbio, string(client->options["hostname"] + ":" + client->options["port"]).c_str());

        if (BIO_do_connect(client->sbio) <= 0) {
          LOGE("ssl - Error connecting to server");
          client->emit("error", "SSL", "Error connecting to server");
          return;
        }

        if (BIO_do_handshake(client->sbio) <= 0) {
          LOGE("ssl - Error establishing SSL connection");
          client->emit("error", "SSL", "Error establishing SSL connection");
          return;
        }

        LOGI("HTTPS connection established to ",
          client->options["hostname"].c_str(),
          ":",client->options["port"].c_str());

        /* Could examine ssl here to get connection info */

        stringstream res;
        res << client->options["method"] << " " << client->options["path"] << " " << "HTTP/1.1\r\n";
        for (const auto &kv : client->requestHeaders) {
          res << kv.first << ":" << kv.second << "\r\n";
        }
        if (client->requestBody.tellp() > 0) {
          res << "Content-Length: " << std::to_string(client->requestBody.tellp()) << "\r\n";
          res << "\r\n";
          res << client->requestBody.rdbuf();
          res << "\r\n";
        }
        res << "\r\n";

        BIO_puts(client->sbio, res.str().c_str());

        char tmpbuf[65536];
        for (;;) {
          int nread = BIO_read(client->sbio, tmpbuf, sizeof(tmpbuf));
          ssize_t parsed;
          LOGI("HTTPS read/buf ", nread, "/", sizeof(tmpbuf));

          if (nread <= 0)
            break;  // EOF

          http_parser *parser = &client->parser;
          parsed = (ssize_t)http_parser_execute(parser, &client->parser_settings, tmpbuf, nread);
          LOGI("parsed: ", parsed);
          if (parser->upgrade) {
            LOGE("raise upgrade unsupported");
            client->emit("error", "upgrade", "upgrade is not supported");
            return;
          } else if (parsed != nread) {
            LOGE("http parse error. ", parsed," parsed of", nread);
            LOGE(http_errno_description(HTTP_PARSER_ERRNO(parser)));
            client->emit("error", "httpparse", "http parser error");
            return;
          }
        }

        BIO_free_all(client->sbio);
      }, 0, 0);
      ASSERT(r == 0);

      if (_defaultLoopAbsented) {
        uv_run(uv_loop, UV_RUN_DEFAULT);
      }
      return;
    }
#endif

    // HTTP
    r = uv_tcp_init(uv_loop, &tcp);
    if (r != 0) {
      LOGE("uv_tcp_init");
      this->emit("error", uv_err_name(r), uv_strerror(r));
      return;
    }

    r = uv_tcp_connect(&connect_req, &tcp, reinterpret_cast<const sockaddr*>(&dest),
      [](uv_connect_t *req, int status) {
        SimpleHttpRequest *client = (SimpleHttpRequest*)req->data;
        if (status != 0) {
          if (status == -ECANCELED)  // ECANCELED by timer
            return;                  // handled. do nothing

          client->_clearTimer();
          client->_clearConnection();
          LOGE("uv_connect_cb");
          client->emit("error", uv_err_name(status), uv_strerror(status));
          return;
        }

        LOGI("TCP connection established to ",
          client->options["hostname"].c_str(),
          ":",client->options["port"].c_str());

        int r = uv_read_start(req->handle, uvAllocCb,
          [](uv_stream_t *tcp, ssize_t nread, const uv_buf_t * buf) {
            SimpleHttpRequest* client = (SimpleHttpRequest*)tcp->data;
            ssize_t parsed;
            LOGI("read/buf ", nread, "/", buf->len);
            if (nread > 0) {
              http_parser *parser = &client->parser;
              parsed = (ssize_t)http_parser_execute(parser, &client->parser_settings, buf->base, nread);

              LOGI("parsed: ", parsed);
              if (parser->upgrade) {
                LOGE("raise upgrade unsupported");
                client->emit("error", "upgrade", "upgrade is not supported");
                return;
              } else if (parsed != nread) {
                LOGE("http parse error. ", parsed," parsed of", nread);
                LOGE(http_errno_description(HTTP_PARSER_ERRNO(parser)));
                client->emit("error", "httpparse", "http parser error");
                return;
              }
            } else {
              if (nread != UV_EOF) {
                LOGE("uv_read_cb");
                client->emit("error", uv_err_name(nread), uv_strerror(nread));
                return;
              }
            }

            //LOGI(buf->base);

            free(buf->base);
        });

        if (r != 0) {
          LOGE("uv_read_start");
          client->emit("error", uv_err_name(r), uv_strerror(r));
          return;
        }

        uv_buf_t resbuf;
        stringstream res;
        res << client->options["method"] << " " << client->options["path"] << " " << "HTTP/1.1\r\n";
        for (const auto &kv : client->requestHeaders) {
          res << kv.first << ":" << kv.second << "\r\n";
        }
        if (client->requestBody.tellp() > 0) {
          res << "Content-Length: " << std::to_string(client->requestBody.tellp()) << "\r\n";
          res << "\r\n";
          res << client->requestBody.rdbuf();
          res << "\r\n";
        }
        res << "\r\n";

        // FIXME : expensive to copy.
        string resString = res.str();
        resbuf.base = (char *)resString.c_str();
        resbuf.len = res.tellp();

        r = uv_write(&client->write_req, req->handle, &resbuf, 1,
          [](uv_write_t* req, int status) {
            SimpleHttpRequest* client = (SimpleHttpRequest*)req->data;
            LOGI("uv_write");
            if (status != 0) {
              LOGE("uv_write_cb");
              client->emit("error", uv_err_name(status), uv_strerror(status));
              return;
            }
        });

        if (r != 0) {
          LOGE("uv_write");
          client->emit("error", uv_err_name(r), uv_strerror(r));
          return;
        }
    });

    if (r != 0) {
      LOGE("uv_tcp_connect");
      this->emit("error", uv_err_name(r), uv_strerror(r));
      return;
    }

    if (_defaultLoopAbsented) {
      uv_run(uv_loop, UV_RUN_DEFAULT);
    }
  }

  Response response;
  unsigned int timeout = 60*2*1000; // 2 minutes default.
 private:
  uv_loop_t* uv_loop;
  bool _defaultLoopAbsented = false;

  uv_getaddrinfo_t getaddrinfo_req;
  uv_tcp_t tcp;
  uv_connect_t connect_req;
  uv_write_t write_req;
  uv_timer_t timer;

#ifdef ENABLE_SSL
  uv_timer_t sslTimer;
  BIO *sbio = NULL;
  SSL_CTX *ctx;
  SSL *ssl;
#endif

  http_parser_settings parser_settings;
  http_parser parser;
  stringstream requestBody;

  Callback<> eventListeners;
  std::function<void(Response&&)> responseCallback = nullptr;
  std::function<void(Error&&)> errorCallback = nullptr;

  map<string, string> options;
  map<string, string> requestHeaders;

  char* lastHeaderFieldBuf;
  int lastHeaderFieldLenth;

  SimpleHttpRequest& _get(const string &url, const string &method) {
    options["method"] = method;
    if(!_parseUrl(url)) {
      //FIXME : logic error. on("error") may be next. USE uv_loop!!
      this->emit("error", "parseUrl", "parseUrl");
    }
    return *this;
  }
  SimpleHttpRequest& _post(const string &url, const string &method, const string& body) {
    options["method"] = method;
    if(!_parseUrl(url)) {
      this->emit("error", "parseUrl", "parseUrl");  //FIXME : logic error.
    }
    this->write(body);
    return *this;
  }

  bool _parseUrl(const string &url) {
    struct http_parser_url u;
    http_parser_url_init(&u);

    if (http_parser_parse_url(url.c_str(), url.length(), 0, &u)) {
      LOGE("failed to parse URL - ", url);
      return false;
    }

    options["protocol"] = url.substr(u.field_data[UF_SCHEMA].off, u.field_data[UF_SCHEMA].len);
    options["protocol"] += ':';
    options["hostname"] = url.substr(u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);
    options["port"] = url.substr(u.field_data[UF_PORT].off, u.field_data[UF_PORT].len);
    options["path"] = url.substr(u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);

    if(options["port"].length() == 0) {
      // FIXME : well-known protocols
      if ( options["protocol"].compare("http") == 0
        || options["protocol"].compare("http:") == 0) {
        options["port"] = "80";
      } else if ( options["protocol"].compare("https") == 0
               || options["protocol"].compare("https:") == 0) {
        options["port"] = "443";
      } else {
        options["port"] = "0";
      }
    }

    if (options["path"].length() == 0) {
      options["path"] = "/";
    }

    return true;
  }


  void _clearConnection() {
    LOGE("_clearConnection");
    if (uv_is_active((uv_handle_t*)&this->tcp) && !uv_is_closing((uv_handle_t*)&this->tcp)) {
      uv_close((uv_handle_t*)&this->tcp, [](uv_handle_t*){});
    }
  }
  void _clearTimer() {
    LOGE("_clearTimer");
    if (!uv_is_closing((uv_handle_t*)&this->timer)) {
      uv_close((uv_handle_t*)&this->timer, [](uv_handle_t*){});
    }
  }

};

} // namespace request

#endif // SIMPLE_HTTP_REQUEST_H_