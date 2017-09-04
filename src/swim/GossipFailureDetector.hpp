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

  // The time between sending out updates to neighboring servers.
  seconds update_round_interval_{};

  // The time we will wait for a `suspected_` server to get back online.
  seconds grace_period_{};

  // The timeout for when we ping a server and that we'll wait for a response to be sent.
  milliseconds ping_timeout_{};

  // The time between subsequent `ping()` sent to neighbors.
  seconds ping_interval_{};

  // Used to pick a random host to ping from the list of alive neighbors.
  static std::default_random_engine random_engine_;

  // This is the server that will be listening to incoming
  // pings and update reports from neighbors.
  std::unique_ptr<SwimServer> gossip_server_{};

  std::vector<std::unique_ptr<std::thread>> threads_{};


public:

  // TODO: for now, deleting the copy constructor and assignment; but maybe they would make sense?
  GossipFailureDetector(const GossipFailureDetector&) = delete;

  // TODO: would it make any sense to have a move constructor?
  GossipFailureDetector(const GossipFailureDetector&&) = delete;
  GossipFailureDetector operator=(const GossipFailureDetector&) = delete;

  GossipFailureDetector operator=(const GossipFailureDetector&&) = delete;

  /**
   * Creates a new detector and starts the embedded Gossip Server; the background threads are
   * <strong>not</strong> started: use `InitAllBackgroundThreads()` once the server `isRunning()`.
   *
   * <p>All time intervals, except for `ping_timeout_msec`, are expressed in seconds.
   *
   * param port for this server to listening to incoming requests
   * @param update_round_interval the time, in seconds, between updates to neighbors
   * @param grace_period the time, in seconds, we will wait for a suspected server to come back
   * @param ping_timeout_msec the time, in milliseconds, we will wait for the ping response
   * @param ping_interval time, in seconds, between ping requests to neighboring servers
   */
  GossipFailureDetector(unsigned short port,
                        const long update_round_interval,
                        const long grace_period,
                        const long ping_timeout_msec,
                        const long ping_interval) :
      update_round_interval_(update_round_interval), grace_period_(grace_period),
      ping_timeout_(ping_timeout_msec), ping_interval_(ping_interval)
  {
    gossip_server_.reset(new SwimServer(port));

    std::thread t([this] { gossip_server_->start(); });
    t.detach();
  }

  virtual ~GossipFailureDetector() {
    VLOG(2) << "Destroying detector, listening on port " << gossip_server_->port();
    if (gossip_server_ && gossip_server_->isRunning()) {
      VLOG(2) << "Stopping server";
      gossip_server_->stop();

      // Wait a little while for the server to stop.
      int retries = 5;
      while (gossip_server_->isRunning() && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }

      if (gossip_server_->isRunning()) {
        LOG(FATAL) << "Failed to stop the server, aborting process";
      }
    }
    VLOG(2) << "stopping background threads";
    StopAllBackgroundThreads();

    VLOG(2) << "done";
  }

  // Getters

  const seconds& update_round_interval() const { return update_round_interval_; }

  const seconds& grace_period() const { return grace_period_; }

  const milliseconds& ping_timeout() const { return ping_timeout_; }

  const seconds& ping_interval() const { return ping_interval_; }


  void set_update_round_interval(const seconds &update_round_interval) {
    update_round_interval_ = seconds(update_round_interval);
  }

  void set_grace_period(const seconds &grace_period) {
    grace_period_ = seconds(grace_period);
  }

  void set_ping_timeout(const milliseconds &ping_timeout) {
    ping_timeout_ = milliseconds(ping_timeout);
  }

  void set_ping_interval(const seconds &ping_interval) {
    ping_interval_ = seconds(ping_interval);
  }

  const SwimServer& gossip_server() const { return *gossip_server_; }

  /**
   * Starts all the gossip protocol threads: ping neighbors, send reports and evict suspects once
   * the grace period has elapsed.
   *
   * <p>This method starts a number of (detached) background threads that will be running for as
   * long as the `gossip_server()` is running.
   */
  void InitAllBackgroundThreads();

  /**
   * Terminates all background threads for this detector.
   */
  void StopAllBackgroundThreads();

  /**
   * Convenience method to add a "neighbor" to this server; those will then
   * start "gossiping" with each other.
   *
   * @param host the host to add to this server's neighbors list; a new instance will be
   * allocated inside the `ServerRecord` that is created to be added to the "alive set," hence
   * passing a temporary variable that gets destroyed after this call will not cause any issue.
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

  /**
   * Prepares a report that can then be sent to neighbors to gossip about.
   *
   * @return a list of all known alive and suspected servers, including only those that have been
   *    added since the last time a report was sent (i.e., the ones we haven't gossiped about yet).
   */
  SwimReport PrepareReport() const;

  /**
   * Pick a neighbor at random from the list of `alive()` servers and sends a ping: if it
   * receives no response, remove the server from the set and add it to the `suspected()` set.
   */
  void PingNeighbor() const;

  /**
   * It sends an update report to a neighbor from the `alive()` set, choosing it according to the
   * following algorithm:
   *
   * <ul>
   *   <li>find one neighbor it hasn't already sent a report to (`didGossip` is false);</li>
   *   <li>if we already have gossipped with all the neighbors, pick one at random from the set</li>
   * </ul>
   */
  void SendReport() const;

  /**
   * Scan the set of `suspected()` servers: if any has been there for longer than the allowed
   * `grace_period()`, then evict it and consider it "dead".
   */
  void GarbageCollectSuspected() const;
};

} // namespace swim
