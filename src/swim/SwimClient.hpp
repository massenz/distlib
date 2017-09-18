// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.

#pragma once

#include <zmq.hpp>

#include "swim.pb.h"

#include "utils/network.h"
#include "SwimCommon.hpp"


// Default items to be reported; see SWIM protocol and `SwimReport` in swim.proto.
#define MAX_ALLOWED_DEFAULT  256


namespace swim {

/**
 * Client that is used to connect to a SwimServer and exchange messages.
 *
 * <p>Clients solely initiate connections and query the server about status, or to
 * send reports.
 */
class SwimClient {

  static const unsigned long DEFAULT_TIMEOUT_MSEC;
  static const unsigned int DEFAULT_SOCKET_LINGER_MSEC;

  Server dest_;
  Server self_;
  unsigned long timeout_;
  unsigned int max_allowed_reports_ = MAX_ALLOWED_DEFAULT;

  /**
   * Builds the URI for the `dest` server, using the hostname and port.
   *
   * @return a string of the form `tcp://hostname:port`
   */
  const std::string destinationUri() const {
    return "tcp://" + dest_.hostname() + ":" + std::to_string(dest_.port());
  }

  /**
   * Common functionality to send an envelop to the destination server; adds this host (`self_`)
   * coordinates and a timestamp.
   *
   * @param envelope the message to be sent to the destination (`dest_`) server.
   * @return the response from the server, is successful; `nullptr` otherwise.
   */
  bool postMessage(SwimEnvelope *envelope) const;

public:

  /**
   * A client to be used to send SWIM messages to the listening remote `dest` server.
   *
   * @param dest where to send the message to.
   * @param self_port if this server is also listening on a port, specify it here.
   * @param timeout an optional timeout, in milliseconds.
   */
  SwimClient(const Server &dest, int self_port = 0, unsigned long timeout = DEFAULT_TIMEOUT_MSEC);

  virtual ~SwimClient() {}

  /**
   * Sends a `ping` request to a remote server; if it receives a response within the given
   * `timeout_` it will mark the server as "healthy" (and, at the same time, the receiving `server_`
   * can mark this sender (`self_`) as "healthy" too).
   *
   * This can also be proactively used by `self_` if it discovers that some servers "suspect" it
   * to be in an "unhealthy" state (and must do so before the "grace period" expires).
   *
   * @return `true` if the server returned an `OK`.
   */
  // TODO: make it return a Future instead
  bool Ping() const;

  /**
   * Sends a status report on all known nodes' statuses changes.
   *
   * The report will contain all changes to the membership list since the last status update was
   * sent out; it may be deliberately truncated if the number of changes (and thus the payload
   * size) exceeds the configured "max allowed reports."
   *
   * @param report
   * @return `false` if the request timed out
   */
  bool Send(const SwimReport& report) const;

  /**
   * Requests the `dest` server to ping `other` and verify its status.
   * See the SWIM protocol for more details.
   *
   * @param other the server to be pinged on behalf of this one
   * @return `false` if the request timed out
   */
  bool RequestPing(const Server *other) const;

  /**
   * Utility method to obtain this server's coordinates.
   *
   * @return this server
   */
  const Server& self() const { return self_; }

  unsigned long timeout() const;

  void set_timeout(unsigned long timeout);

  unsigned int max_allowed_reports() const;

  void set_max_allowed_reports(unsigned int max_allowed_reports_);

  void setSelf(const Server& other) {
    self_.set_hostname(other.hostname());
    self_.set_ip_addr(other.ip_addr());
    self_.set_port(other.port());
  }
};

} // namespace swim
