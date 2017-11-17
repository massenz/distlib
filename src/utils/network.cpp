// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/9/16.


#include "utils.hpp"

namespace utils {

std::string InetAddress(const std::string& host) {
  char ip_addr[INET6_ADDRSTRLEN];
  struct addrinfo hints;
  struct addrinfo *result;
  std::string retval;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV;

  int resp = getaddrinfo(host.empty() ? Hostname().c_str() : host.c_str(), nullptr,
                         &hints, &result);
  if (resp) {
    LOG(ERROR) << "Cannot find IP address for '" << host << "': " << gai_strerror(resp);
  } else {
    // Iterate the addr_info fields, until we find an IPv4 field
    for (auto rp = result; rp != nullptr; rp = rp->ai_next) {
      if (rp->ai_family == AF_INET) {
        if (inet_ntop(AF_INET,
                      &((struct sockaddr_in *)rp->ai_addr)->sin_addr,
                      ip_addr,
                      INET6_ADDRSTRLEN)) {
          retval = ip_addr;
          break;
        }
      }
    }
  }

  if (result != nullptr) {
    freeaddrinfo(result);
  }
  return retval;
}


std::string SocketAddress(unsigned int port) {
  struct addrinfo hints;
  struct addrinfo *result;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

  std::string retval;
  std::string port_str = std::to_string(port);

  if (!getaddrinfo(nullptr, port_str.c_str(), &hints, &result) && result != nullptr) {
    // Simply return the first available socket.
    char host[NI_MAXHOST];
    if (getnameinfo(result->ai_addr, result->ai_addrlen, host, NI_MAXHOST,
                    nullptr, 0, NI_NUMERICHOST) == 0) {
      std::ostringstream ostream;
      ostream << "tcp://" << host << ":" << port;
      retval = ostream.str();
    }
    freeaddrinfo(result);
  }

  return retval;
}


google::uint64 CurrentTime() {
  return static_cast<google::uint64>(std::time(nullptr));
}


std::string Hostname() {
  char name[NI_MAXHOST];
  if (gethostname(name, NI_MAXHOST) != 0) {
    LOG(ERROR) << "Could not determine hostname";
    return "";
  }
  return std::string(name);
}

} // namespace utils
