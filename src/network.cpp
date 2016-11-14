// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/9/16.


#include "network.h"

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


google::protobuf::int64 current_time() {
  return std::time(nullptr);
}


std::string hostname() {
  char name[MAXBUFSIZE];
  int retcode = gethostname(name, MAXBUFSIZE);
  if (retcode != 0) {
    LOG(ERROR) << "Could not determine hostname";
    return "UNKNOWN";
  }
  return std::string(name);
}
