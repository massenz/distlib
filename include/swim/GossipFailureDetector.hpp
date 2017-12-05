// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.

#pragma once

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <set>

#include "swim.pb.h"
#include "SwimServer.hpp"
#include "SwimCommon.hpp"

#include "../utils/utils.hpp"


namespace swim {


/**
 * A failure detector that implements the [SWIM protocol](https://goo.gl/VUn4iQ)
 *
 * <p>The detector embeds a `SwimServer` that is actually used to connect to external "neighbors"
 * and report/detect health: a const reference can be retrieved via the
 * `#gossip_server()` method.
 *
 * <p>This detector runs several background threads that will __not__ be started at creation, but
 * __must__ be started after the embedded `SwimServer` is running (use `SwimServer::isRunning()`)
 * using `InitAllBackgroundThreads()`.
 *
 * <p>Each thread is responsibe for the various tasks that this detector must undertake:
 * <ul>
 * <li>`SendReport()` to other detectors, to report and detect failures;; and
 * <li>`GarbageCollectSuspected()` to remove "suspected" failed server, after the #grace_period()
 *      has elapsed.
 * </ul>
 */
class GossipFailureDetector {

  /**
   * The time between sending out updates to neighboring servers; these will also serve as
   * "liveness probes".
   */
  seconds update_round_interval_{};

  /**
   * The time we will wait for a `suspected` server to get back online.
   */
  seconds grace_period_{};

  /**
   * The timeout for when we ping a server and that we'll wait for a response to be sent.
   */
  milliseconds ping_timeout_{};

  /**
   * When sending reports (and probing neighbors' health) we will send this many reports out.
   */
  unsigned int num_reports_;

  /**
   * When we find a "suspected" server for the first time, we ask `num_forwards_` neighbors to
   * try and contact it on our behalf.
   */
  unsigned int num_forwards_;

private:

  /**
   * This is the server that will be listening to incoming
   * pings and update reports from neighbors.
   */
  std::unique_ptr<SwimServer> gossip_server_{};

  std::vector<std::unique_ptr<std::thread>> threads_{};

public:

  // TODO: for now, deleting the copy constructor and assignment; but maybe they would make sense?
  GossipFailureDetector(const GossipFailureDetector &) = delete;

  GossipFailureDetector operator=(const GossipFailureDetector &) = delete;

  // TODO: would it make any sense to have a move constructor too?
  GossipFailureDetector(const GossipFailureDetector &&) = delete;

  GossipFailureDetector operator=(const GossipFailureDetector &&) = delete;

  const unsigned int kDefaultNumReports = 6;

  const unsigned int kDefaultNumForward = 3;

  /**
   * Creates a new detector and starts the embedded Gossip Server; the background threads are
   * <strong>not</strong> started: use `InitAllBackgroundThreads()` once the server `isRunning()`.
   *
   * <p>All time intervals, except for `ping_timeout_msec`, are expressed in seconds.
   *
   * @param port for this server to listening to incoming requests
   * @param interval the time, in seconds, between updates to neighbors
   * @param grace_period the time, in seconds, we will wait for a suspected server to come back
   * @param ping_timeout_msec the time, in milliseconds, we will wait for the ping response
   */
  GossipFailureDetector(unsigned short port,
                        const long interval,
                        const long grace_period,
                        const long ping_timeout_msec = kDefaultTimeoutMsec) :
      update_round_interval_(interval),
      grace_period_(grace_period),
      ping_timeout_(ping_timeout_msec)
  {
    num_reports_ = kDefaultNumReports;
    num_forwards_ = kDefaultNumForward;
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

  /**
   * @return the time between two successive reports (sent using `SendReport()`)
   */
  const seconds &update_round_interval() const { return update_round_interval_; }

  /**
   * @return the time in seconds after which a "suspected" server is declared "dead"
   */
  const seconds &grace_period() const { return grace_period_; }

  /**
   * @return the timeout for a PingNeighbor() to a server: if the server does not respond within
   * the given timeout, it will be declared "suspected" and placed in the `SwimServer::suspected_`
   * set.
   */
  const milliseconds &ping_timeout() const { return ping_timeout_; }

  /**
   * Updates the interval between sending reports to neighbors.
   *
   * @param update_round_interval
   */
  void set_update_round_interval(const seconds &update_round_interval) {
    update_round_interval_ = update_round_interval;
  }

  /**
   * Updates the time after which we evict a suspected server.
   *
   * @param grace_period
   */
  void set_grace_period(const seconds &grace_period) {
    grace_period_ = grace_period;
  }

  /**
   * Updates the time in milliseconds we are prepared to wait for a server to respond, before we
   * suspect it to be unavailable.
   *
   * @param ping_timeout
   */
  void set_ping_timeout(const milliseconds &ping_timeout) {
    ping_timeout_ = ping_timeout;
  }

  /**
   * @return the configured number of servers to contact at every round of reporting.
   */
  unsigned int num_reports() const {
    return num_reports_;
  }

  /**
   * Sets the number of servers to contact at each round.
   *
   * @param num_reports how servers to contact
   */
  void set_num_reports(unsigned int num_reports) {
    num_reports_ = num_reports;
  }

  /**
   * @return the configured number of servers to contact for forwarding requests to an
   * unresponsive server.
   */
  unsigned int num_forwards() const {
    return num_forwards_;
  }

  /**
   * Sets the number of servers to contact when forwarding requests.
   *
   * @param num_reports how many servers to contact
   */
  void set_num_forwards(unsigned int num_forwards) {
    num_forwards_ = num_forwards;
  }


  /**
   * @return A reference to the embedded SwimServer that is used to connect to neighbors.
   */
  const SwimServer &gossip_server() const { return *gossip_server_; }

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
  void AddNeighbor(const Server &host) {
    if (!gossip_server_->AddAlive(host)) {
      LOG(WARNING) << "Failed to add host " << host << " to neighbors sets";
    }
  }

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

  /**
   * Convenience method to obtain a set of `k` servers from the `alive` set.
   *
   * @param k the number of servers we seek at most (it may be less, if the `alive` size is less
   *        than `k`)
   * @return a set of at most `k` neighboring servers, known to be healthy.
   */
  std::set<Server> GetUniqueNeighbors(unsigned int k) const;
};
} // namespace swim
