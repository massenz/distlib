// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.

#pragma once

#include <vector>
#include <swim.pb.h>
#include <chrono>
#include <bits/unique_ptr.h>
#include "SwimServer.hpp"

using std::vector;
using namespace std::chrono;

using Timestamp = std::chrono::system_clock::time_point;

namespace swim {

struct ServerRecord {
  Server server;
  Timestamp timestamp;

  ServerRecord(const Server& host) : server(host), timestamp(system_clock::now()) { }
};


class GossipFailureDetector {

  // The list of servers that we deem to be healthy (they responded to a ping request).
  // The timestamp is the last time we successfully pinged the server.
  vector<const ServerRecord*> alive_;

  // The list of servers we suspect to have crashed; the timestamp was the time at which
  // the server was placed on this list (and will dictate when we finally determine it
  // to have crashed, after the `grace_period_` time has elapsed).
  vector<const ServerRecord*> suspected_;

  milliseconds update_round_interval_;
  milliseconds grace_period_;
  milliseconds ping_timeout_;
  milliseconds min_ping_interval_;


  // This is the server that will be listening to incoming
  // pings and update reports from neighbors.
  std::unique_ptr<SwimServer> gossip_server_;

public:
  GossipFailureDetector(const long &update_round_interval,
                        const long &grace_period, const long &ping_timeout,
                        const long &min_ping_interval);

  virtual ~GossipFailureDetector() {}

  // This is a convenience method to add more "neighbors" to this server, that will then
  // start "gossiping" with each other.
  void AddNeighbor(const Server& host) {
    for (const ServerRecord* record : alive_) {
      if (record->server.hostname() == host.hostname() &&
          record->server.port() == host.port()) {
        return;
      }
    }
    alive_.push_back(new ServerRecord(host));
  }

  const vector<const swim::ServerRecord *>& alive() const {
    return alive_;
  }

  const vector<const swim::ServerRecord *>& suspected() const {
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
