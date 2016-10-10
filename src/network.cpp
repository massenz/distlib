// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/9/16.


#include <glog/logging.h>

#include "network.h"

/**
 * Given a Linux {@link struct sockaddr} structure, it returns a string that
 * represents the {@literal host:port} for the network address.
 *
 * @param addr the address information struct
 * @param socklen the size of the struct pointed at by `addr`
 * @return a string of the form "localhost:8080"
 */
std::string inetAddress(const struct sockaddr *addr, socklen_t socklen) {
  char host[NI_MAXHOST],
       service[NI_MAXSERV];

  if (getnameinfo(addr, socklen, host, NI_MAXHOST,
                  service, NI_MAXSERV, NI_NUMERICSERV) == 0) {
    std::ostringstream ostream;
    ostream << host << ":" << service;
    return ostream.str();
  }

  return "";
}


/**
 * Given a port number will return a suitable socket address in string
 * format for the server to be listening on (e.g., "tcp://0.0.0.0:5000").
 *
 * @param port the host port the server would like to listen to
 * @return a string suitable for use with ZMQ {@link socket_t socket()} call.
 *      Or an empty string, if no suitable socket is available.
 */
std::string sockAddr(unsigned int port) {
  struct addrinfo hints;
  struct addrinfo *result;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

  std::string port_str = std::to_string(port);

  int resp = getaddrinfo(nullptr, port_str.c_str(), &hints, &result);
  if (resp != 0 || result == nullptr) {
    LOG(ERROR) << "Could not find address info: " << gai_strerror(resp);
    return "";
  }

  // Simply return the first available socket.
  std::string retval =  inetAddress(result->ai_addr, result->ai_addrlen);
  freeaddrinfo(result);

  return retval.empty() ? "" : "tcp://" + retval;
}
