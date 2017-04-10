// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.

#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <set>

#include "swim.pb.h"
#include "SwimServer.hpp"
#include "utils/network.h"


using std::set;
using namespace std::chrono;

using Timestamp = std::chrono::system_clock::time_point;

namespace swim {

/**
 * Ordering operator for server records, uses the `Server` IP address as a total ordering
 * criterion.  For servers on the same host (IP), the port number is used; the timestamp is
 * not used as a sorting criterion.
 *
 * <p>The sorting by IP addresses is done solely lexicographically, no meaning is assigned
 * to the octets; it is simply a means to allow us to store `ServerRecord` objects into a `set`.
 *
 * @param other the ServerRecord to compare against
 * @return whether this server record is logically "less than" `other`
 */
bool operator<(const ServerRecord &lhs, const ServerRecord& rhs);


/**
 * Without this, associative containers (such as `set`) would compare the value of the pointers
 * (as opposed to the actual value of the records) for insertion and/or retrieval.
 *
 * <p>See Item 20 of "Effective STL", Scott Meyers.
 */
struct ServerRecordPtrLess:
    public std::binary_function<
        const std::shared_ptr<ServerRecord>&,
        const std::shared_ptr<ServerRecord>&,
        bool> {

  /**
   * Comparison operator for the "less than" predicate (associative containers semantics) for
   * containers of pointers.
   *
   * @param lhs the left-hand side of the operation
   * @param rhs the right-hand side of the operation
   * @return whether the record pointed to by `lhs` is "less than" the one pointed to by `rhs`
   */
  bool operator()(const std::shared_ptr<ServerRecord>& lhs,
                  const std::shared_ptr<ServerRecord>& rhs) {
    return *lhs < *rhs;
  }
};


typedef set<std::shared_ptr<ServerRecord>, ServerRecordPtrLess> ServerRecordsSet;


/**
 * A failure detector that implements the SWIM protocol.
 * See: https://goo.gl/VUn4iQ
 */
class GossipFailureDetector {

  /**
   * The list of servers that we deem to be healthy (they responded to a ping request).
   * The timestamp is the last time we successfully pinged the server.
   */
  ServerRecordsSet alive_;

  /**
   * The list of servers we suspect to have crashed; the timestamp was the time at which
   * the server was placed on this list (and will dictate when we finally determine it
   * to have crashed, after the `grace_period_` time has elapsed).
   */
  ServerRecordsSet suspected_;

  milliseconds update_round_interval_;
  milliseconds grace_period_;
  milliseconds ping_timeout_;
  milliseconds min_ping_interval_;


  // This is the server that will be listening to incoming
  // pings and update reports from neighbors.
  std::unique_ptr<SwimServer> gossip_server_;

public:
  GossipFailureDetector(unsigned int port,
                        const long update_round_interval,
                        const long grace_period,
                        const long ping_timeout,
                        const long min_ping_interval) :
      update_round_interval_(update_round_interval), grace_period_(grace_period),
      ping_timeout_(ping_timeout), min_ping_interval_(min_ping_interval),
      gossip_server_(new SwimServer(port))
  {
    std::thread t([this] { gossip_server_->start(); });
    t.detach();
  }

  virtual ~GossipFailureDetector() {}

  /**
   * This is a convenience method to add more "neighbors" to this server; those will then
   * start "gossiping" with each other.
   *
   * @param host the host to add to this server's neighbors list
   */
  void AddNeighbor(const Server& host) {
    std::shared_ptr<ServerRecord> record = std::make_shared<ServerRecord>();

    Server *server = record->mutable_server();
    server->set_hostname(host.hostname());
    server->set_port(host.port());
    record->set_timestamp(utils::current_time());

    alive_.insert(record);
  }

  const ServerRecordsSet& alive() const {
    return alive_;
  }

  const ServerRecordsSet& suspected() const {
    return suspected_;
  }

  const milliseconds& update_round_interval() const {
    return update_round_interval_;
  }

  const milliseconds& grace_period() const {
    return grace_period_;
  }

  const milliseconds& ping_timeout() const {
    return ping_timeout_;
  }

  const milliseconds& min_ping_interval() const {
    return min_ping_interval_;
  }
};

} // namespace swim
