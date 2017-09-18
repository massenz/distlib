// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.

#pragma once

#include <atomic>
#include <chrono>
#include <iomanip>
#include <memory>
#include <mutex>
#include <thread>

#include <glog/logging.h>
#include <zmq.hpp>

#include "utils/network.h"
#include "SwimCommon.hpp"
#include "SwimClient.hpp"

namespace swim {

class SwimServer {

  unsigned short port_;
  unsigned int num_threads_;
  std::atomic<bool> stopped_;
  unsigned long polling_interval_;

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

  /**
   * Access to the collection of "alive" servers must be thread-safe;
   * use a std::lock_guard to protect access.
   *
   * @todo std::mutex is not re-entrant: use a std::unique_lock instead.
   */
  mutable std::mutex alive_mutex_;

  /**
   * Access to the collection of "suspected" servers must be thread-safe;
   * use a std::lock_guard to protect access.
   *
   * @todo std::mutex is not re-entrant: use a std::unique_lock instead.
   */
  mutable std::mutex suspected_mutex_;
protected:

  /**
   * Invoked when the `client` sends a `SwimEnvelope::Type::STATUS_UPDATE` message to this server.
   *
   * <p>This is a callback method that will be invoked by the server's loop, in its
   * own thread.
   *
   * @param client the server that just sent the message; the callee obtains ownership of the
   *        pointer and is responsible for freeing the memory.
   */
  virtual void onUpdate(Server *client);

  /**
   * Invoked when the `client` sends a `SwimEnvelope::Type::STATUS_REPORT` message to this server.
   *
   * <p>This is a callback method that will be invoked by the server's loop, in its
   * own thread.
   *
   * @param sender the server that just sent the message; the callee obtains ownership of the
   *        pointer and is responsible for freeing the memory.
   * @param report the `SwimReport` that the `client` sent and can be used by this server to
   *        update its membership list.
   */
  virtual void onReport(Server *sender, SwimReport *report);

  /**
   * Invoked when the `client` sends a `SwimEnvelope::Type::STATUS_REQUEST` message to this server.
   *
   * <p>This is essentially a callback method that will be invoked by the server's loop, in its
   * own thread.
   *
   * <p>This message requests this server to send a ping on behalf of the `sender`, to
   * the `destination` server; if the latter responds within the given timeout, this server will
   * send back a report to notify that it is alive.
   *
   * <p>If the `destination` does not respond, no action is taken, beyond marking it as a
   * `suspected` crashed server.
   *
   * @param sender the server that just sent the message; the callee obtains ownership of the
   *        pointer and is responsible for freeing the memory.
   * @param destination the other server that the requestor (`client`) is asksing this server to
   *        ping and report status back.
   */
  virtual void onPingRequest(Server *sender, Server *destination);

public:

  /** Number of parallel threads handling ZMQ socket requests. */
  static const unsigned int NUM_THREADS = 5;

  /**
   * Default value for the interval in msec for the server to wait for incoming data, between checks
   * of having been stopped.
   *
   * <p>This is a timeout value, so there is no adverse effect to set it at a higher value,
   * however this will impact the time it takes for the server to stop or shutdown.
   */
  static const unsigned long DEFAULT_POLLING_INTERVAL_MSEC;

  /**
   * ZMQ allows the socket to linger after having been closed: currently this is set to 0
   * to disable that.
   */
  static const unsigned int DEFAULT_SOCKET_LINGER_MSEC;


  SwimServer(unsigned short port,
             unsigned int threads = NUM_THREADS,
             unsigned long polling_interval = DEFAULT_POLLING_INTERVAL_MSEC
  ) : port_(port), num_threads_(threads), stopped_(true), polling_interval_(polling_interval) {}

  virtual ~SwimServer() {
    int retry_count = 5;

    stop();
    while (isRunning() && retry_count-- > 0) {
      VLOG(2) << "Waiting for server to stop...";
      std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_POLLING_INTERVAL_MSEC));
    }
    if (retry_count == 0) {
      LOG(ERROR) << "Timed out waiting for server to shut down; giving up.";
    }
    VLOG(2) << "Server shutdown complete";
  }

  /**
   * Run the server forever until `stop()` is called.
   *
   * <p>If you want to run it asynchronously, the easiest way is to override this virtual
   * function and run it inside its own thread.
   */
  virtual void start();

  void stop() { stopped_.store(true); }

  bool isRunning() const { return !stopped_; }

  unsigned short port() const { return port_; }

  /**
   *
   * @return the coordinates of this server (host and port; optionally, the IP address too)
   */
  const Server self() const {
    auto server = Server();
    server.set_port(port());
    server.set_hostname(utils::Hostname());
    server.set_ip_addr(utils::InetAddress());
    return server;
  }

  /**
   * This is thread-safe to access.
   * @return the number of currently detected `alive` neighbors.
   */
  unsigned long alive_size() const {
    unsigned long num;
    {
      mutex_guard lock(alive_mutex_);
      num = alive_.size();
    }
    return num;
  }

  /**
   * This is thread-safe to access.
   * @return the number of currently detected `suspected` neighbors.
   */
  unsigned long suspected_size() const {
    mutex_guard lock(suspected_mutex_);
    return suspected_.size();
  }

  /**
   * This is thread-safe to access.
   * @return whether the set of `alive` servers is empty.
   */
  bool alive_empty() const { return alive_size() == 0; }

  /**
   * This is thread-safe to access.
   * @return whether the set of `suspected` servers is emtpy.
   */
  bool suspected_empty() const { return suspected_size() == 0; }

  /**
   * Prepares a report that can then be sent to neighbors to gossip about.
   *
   * @return a list of all known alive and suspected servers, including only those that have been
   *    added since the last time a report was sent (i.e., the ones we haven't gossiped about yet).
   */
  SwimReport PrepareReport() const;

  /**
   * @return From the set of `alive` neighbors, it will return a randomly chosen one.
   * @throws `swim::empty_set` if the set of alive neighbors is empty.
   */
  Server GetRandomNeighbor() const;

  /**
   * Used to report a non-responding server.
   *
   * @param server that failed to respond to a ping request and thus is suspected of being in a
   * failed state
   * @return whethet adding `server` to the `suspected_` set was successful
   */
  bool ReportSuspect(const Server &);

  /**
   * Use this for either a newly discovered neighbor, or for a `suspected_` server that was in
   * fact found to be in a healthy state.
   *
   * @param server that will be added to the `alive_` set (and, if necessary, removed from the
   * `suspected_` one).
   * @return whether adding `server` to `alive_` set was successful
   */
  bool AddAlive(const Server&);

  /**
   * Removes the given server from the suspected set, thus marking it as terminally unreachable
   * (unless subsequently added back using `AddAlive(const Server&)`).
   *
   * <p>Typically used when the `grace_period()` expires and the `server` has not been
   * successfully contacted, by either this `SwimServer` or other "forwarders."
   *
   * @param server to remove from the set
   */
  void RemoveSuspected(const Server&);
};


/**
 * Factory method for `SwimServer` implementation classes.
 *
 * <p>Users of this library will have to provide their own implementation of this method and,
 * possibly, also a class implementation for the server to be returned.
 *
 * <p>Otherwise any one of the provided default implementations can be returned, properly
 * initialized.
 *
 * @param port the port the server will be listening on; use -1 for a randomly chosen one.
 * @return an implementation of the `SwimServer` server.
 */
std::unique_ptr<SwimServer> CreateServer(unsigned short port);

} // namespace swim



