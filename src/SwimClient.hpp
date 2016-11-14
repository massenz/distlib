// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.

#pragma once


#include "swim.pb.h"
#include "network.h"


#define DEFAULT_TIMEOUT_MSEC    250

namespace swim {

/**
 * Creates a Protobuf representation of the given hostname:port server.
 *
 * @param hostname the server's name, if known (or its IP address).
 * @param port the listening port
 * @param ip the IP address (optional, may be useful if the name is not DNS-resolvable)
 * @return the PB representation of this server
 */
static Server makeServer(const std::string &hostname, int port, const std::string &ip = "") {
  Server server;
  server.set_hostname(hostname);
  server.set_port(port);
  if (ip.length() > 0) {
    server.set_ip_addr(ip);
  }
  return server;
}


class SwimClient {

  Server server_;
  unsigned long timeout_;
  Server self_;


public:

  /**
   * Builds a status update message to be sent to the `server`.
   *
   * @param server where to send the message to.
   */
  SwimClient(const Server &server, int self_port, unsigned long timeout = DEFAULT_TIMEOUT_MSEC);

  SwimClient(const std::string &hostname, int port, int self_port = 0,
             unsigned long timeout = DEFAULT_TIMEOUT_MSEC) :
      SwimClient(makeServer(hostname, port), self_port, timeout) {}

  virtual ~SwimClient() {}

  /**
   * Sends a `ping` request to a remote server; if it receives a response within the given
   * `timeout_` it will mark the server as "healthy" (and, at the same time, the receiving `server_`
   * can mark this sender (`self_`) as "healthy" too).
   *
   * This can also be proactively used by `self_` if it discovers that some servers "suspect" it
   * to be in an "unhealthy" state (and must do so before the "grace period" expires).
   */
  // TODO: make it return a Future instead
  void ping();

  /**
   * Sends a status report on all known nodes' statuses changes.
   *
   * The report will contain all changes to the membership list since the last status update was
   * sent out; it may be deliberately truncated if the number of changes (and thus the payload
   * size) exceeds the configured "max allowed reports."
   *
   * @param report
   */
  void send(const SwimReport *report);

  /**
   * Requests the server to ping `other` and verify its status.
   * See the SWIM protocol for more details.
   *
   * @param other the server to be pinged on behalf of this one
   */
  void requestPing(const Server *other);

  /**
   * Utility method to obtain this server's coordinates.
   * 
   * @return this server
   */
  const Server& self() const { return self_; }
};

} // namespace swim
