// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/17/17.

#include <microhttpd.h>

#include "swim/GossipFailureDetector.hpp"

namespace swim {

namespace rest {

class HttpCannotStartError : public std::exception {

};

/**
 * Implements a minimalist REST API for a `GossipFailureDetector` server.
 * Uses GNU `libmicrohttpd` as the underlying HTTP daemon.
 *
 * <p>In the current implementation it is uniquely associated with a reference to the
 * detector server, thus <strong>it must not outlive</strong> the referenced server's
 * life.
 *
 * @see https://www.gnu.org/software/libmicrohttpd/
 */
class ApiServer {

  unsigned int port_;
  const GossipFailureDetector *detector_;
  struct MHD_Daemon *httpd_;

  static int ConnectCallback(void *cls, struct MHD_Connection *connection,
                      const char *url,
                      const char *method,
                      const char *version,
                      const char *upload_data,
                      size_t *upload_data_size,
                      void **con_cls) {
    struct MHD_Response *response;
    int ret;

    auto gfd = static_cast<const swim::GossipFailureDetector *>(cls);

    std::stringstream stream;
    stream << "<html><body><h3>Gossip Failure Detector: " << gfd->gossip_server().self()
           << "</h3>"
           << "<p>Report: <pre>" << gfd->gossip_server().PrepareReport() << "</pre></p>";
    stream << "</body></html>";

    response = MHD_create_response_from_buffer(stream.str().size(),
                                               (void *) stream.str().c_str(),
                                               MHD_RESPMEM_MUST_COPY);
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
  }

public:
  explicit ApiServer(const swim::GossipFailureDetector *detector, unsigned int port) :
      detector_(detector), port_(port) {
    httpd_ = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
                              port_,
                              nullptr,    // Allow all clients to connect
                              nullptr,
                              ApiServer::ConnectCallback,
                              (void *)detector_,       // The GFD as the extra arguments.
                              MHD_OPTION_END);  // No extra options to daemon either.

    if (httpd_ == nullptr) {
      throw HttpCannotStartError();
    }
  }

  void stop() { MHD_stop_daemon(httpd_); }
};


} // namespace rest
} // namespace swim
