#include <iostream>
#include "SimpleHttpRequest.hpp"

using namespace std;
using namespace request;

int main(int argc, char **argv) {

  SimpleHttpRequest request;

  if (argc < 2) {
    cerr << "Usage: example URL" << endl
    return EXIT_ERROR;
  }

  string url = argv[1];
  request.timeout = 5000;

  try {
    request.get(url)
           .on("error", [](Error&& err) {
              cerr << err.name << endl << err.message << endl;
              throw std::runtime_error(err.message);
           }).on("response", [](Response&& res) {
              cout << res.str();
           }).end();
  } catch(const std::exception &e) {
    cerr << "Caught exception: " << e.what() << endl ;
    return EXIT_ERROR;
  }

  return EXIT_SUCCESS;
}

int use_options(const std::string& hostname,
                unsigned int port,
                const std::string& path) {
  uv_loop = uv_default_loop();

  // request(options) GET
  map<string, string> options = {
    { "hostname", hostname },
    { "port", std::to_string(port) },
    { "protocol", "http:" },
    { "path", path },
    { "method", "GET" }
  };

  SimpleHttpRequest request(options, uv_loop);
  request.setHeader("content-type","application/json")
      .on("error", [](Error&& err){
        cerr << err.name << endl << err.message << endl;
      }).on("response", [](Response&& res){
        for (const auto &kv : res.headers)
          cout << kv.first << " : " << kv.second << endl;

        cout << res.str();
      }).end();

  return uv_run(uv_loop, UV_RUN_DEFAULT);
}


#if 0
  uv_loop = uv_default_loop();

// request.post(url, body) POST
  string body = "{\"archive\":3}";
  SimpleHttpRequest request(uv_loop);
  request.setHeader("content-type","application/json")
  .post("http://" + HOSTNAME + ":" + PORT + PATH, body)
  .on("error", [](Error&& err){
    cerr << err.name << endl << err.message << endl;
  }).on("response", [](Response&& res){
    cout << endl << res.statusCode << endl;
    cout << res.str() << endl;
  })
  .end();

  return uv_run(uv_loop, UV_RUN_DEFAULT);
#endif


#if 0
  uv_loop = uv_default_loop();

// request(options, headers) POST write(string)
  map<string, string> options;
  map<string, string> headers;
  options["hostname"] = HOSTNAME;
  options["port"] = PORT;
  options["path"] = PATH;
  options["method"] = "POST";
  headers["content-type"] = "application/json";

  SimpleHttpRequest request(options, headers, uv_loop);
  request.on("error", [](Error&& err){
    cerr << err.name << endl << err.message << endl;
  });
  request.on("response", [](Response&& res){
    cout << endl << res.statusCode << endl;
    cout << res.str() << endl;
  });
  request.write("{\"archive\":3}");
  request.end();

  return uv_run(uv_loop, UV_RUN_DEFAULT);
#endif

#if 0
  uv_loop = uv_default_loop();

// request(options, headers) POST write(stream)
  map<string, string> options;
  map<string, string> headers;
  options["hostname"] = HOSTNAME;
  options["port"] = PORT;
  options["path"] = PATH;
  options["method"] = "POST";
  headers["content-type"] = "application/json"; // ignored!!

  stringstream body;
  body << "{\"archive\":3}";
  SimpleHttpRequest request(options, headers, uv_loop);
  request.on("error", [](Error&& err){
    cerr << err.name << endl << err.message << endl;
  });
  request.on("response", [](Response&& res){
    cout << endl << res.statusCode << endl;
    cout << res.str() << endl;
  });
  request.write(body);  //contnet-type = "application/octet-stream"
  request.end();

  return uv_run(uv_loop, UV_RUN_DEFAULT);
#endif

