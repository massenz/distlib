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
                      void **con_cls);

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
