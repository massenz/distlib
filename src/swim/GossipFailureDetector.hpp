// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.

#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <set>

#include "swim.pb.h"
#include "SwimServer.hpp"
#include "SwimCommon.hpp"

#include "utils/network.h"


namespace swim {


/**
 * A failure detector that implements the SWIM protocol.
 * See: https://goo.gl/VUn4iQ
 */
class GossipFailureDetector {

  milliseconds update_round_interval_;
  milliseconds grace_period_;
  milliseconds ping_timeout_;
  milliseconds min_ping_interval_;


  // This is the server that will be listening to incoming
  // pings and update reports from neighbors.
  std::unique_ptr<SwimServer> gossip_server_;

public:
  GossipFailureDetector(unsigned short port,
                        const long update_round_interval,
                        const long grace_period,
                        const long ping_timeout,
                        const long min_ping_interval) :
      update_round_interval_(update_round_interval), grace_period_(grace_period),
      ping_timeout_(ping_timeout), min_ping_interval_(min_ping_interval)
  {
    gossip_server_ = CreateServer(port);

    std::thread t([this] { gossip_server_->start(); });
    t.detach();
  }

  virtual ~GossipFailureDetector() {
    VLOG(2) << "Destroying detector, listening on port " << gossip_server_->port();
    if (gossip_server_ && gossip_server_->isRunning()) {
      VLOG(2) << "Stopping server";
      gossip_server_->stop();
    }
    VLOG(2) << "done";
  }

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
    record->set_timestamp(utils::CurrentTime());
    record->set_didgossip(false);

    ServerRecordsSet* ps = gossip_server_->mutable_alive();
    ps->insert(record);
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

  const ServerRecordsSet& alive() const { return gossip_server_->alive(); }
  const ServerRecordsSet& suspected() const { return gossip_server_->suspected(); }

  SwimServer& gossip_server() const { return *gossip_server_; }

  /**
   * Prepares a report that can then be sent to neighbors to gossip about.
   *
   * @return a list of all known alive and suspected servers, including only those that have been
   *    added since the last time a report was sent (i.e., the ones we haven't gossiped about yet).
   */
  SwimReport PrepareReport();
};

} // namespace swim
