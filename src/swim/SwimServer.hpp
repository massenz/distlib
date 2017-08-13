// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.

#pragma once

#include <atomic>
#include <chrono>
#include <iomanip>
#include <memory>
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

protected:

  /**
   * Invoked when the `client` sends a `SwimEnvelope::Type::STATUS_UPDATE` message to this server.
   *
   * <p>This is a callback method that will be invoked by the server's loop, in its
   * own thread.
   *
   * <p><strong>Note</strong> that this server class is currently <strong>NOT</strong>
   * thread-safe and that none of its members (apart from the `atomic<bool>` stopped_) should be
   * modified while processing this call.
   *
   * @param client the server that just sent the message; the callee obtains ownership of the
   *        pointer and is responsible for freeing the memory.
   */
  virtual void onUpdate(Server* client) {
    // Make sure client will be deleted, even if an exception is thrown.
    std::unique_ptr<Server> ps(client);

    std::shared_ptr<ServerRecord> record = MakeRecord(*client);
    alive_.insert(record);

    // If it was previously suspected of being unresponsive, this server is removed from the
    // suspected list:
    auto removed = suspected_.erase(record);
    if (removed > 0) {
      VLOG(2) << *client << " previously suspected; added back to healthy set";
    } else {
      VLOG(3) << "Received a ping from " << *client;
    }
  }

  /**
   * Invoked when the `client` sends a `SwimEnvelope::Type::STATUS_REPORT` message to this server.
   *
   * <p>This is a callback method that will be invoked by the server's loop, in its
   * own thread.
   *
   * <p><strong>Note</strong> that this server class is currently <strong>NOT</strong>
   * thread-safe and that none of its members (apart from the `atomic<bool>` stopped_) should be
   * modified while processing this call.
   *
   * @param sender the server that just sent the message; the callee obtains ownership of the
   *        pointer and is responsible for freeing the memory.
   * @param report the `SwimReport` that the `client` sent and can be used by this server to
   *        update its membership list.
   * @param timestamp when the message was sent (according to the `client`'s clock).
   */
  virtual void onReport(Server* sender, SwimReport* report) {
    std::unique_ptr<Server> ps(sender);

    VLOG(2) << self() << " received: " << *report;
    for (auto record : report->alive()) {
      if (record.server() == self()) {
        // yes, we know we are alive, thank you very much.
        continue;
      }

      std::shared_ptr<ServerRecord> pRecord(new ServerRecord(record));
      pRecord->set_didgossip(false);

      auto was_suspected = suspected_.find(pRecord);
      if (was_suspected != suspected_.end() &&
          (*was_suspected)->timestamp() < record.timestamp()) {
        suspected_.erase(pRecord);
        alive_.insert(pRecord);
      } else if (was_suspected == suspected_.end()) {
        alive_.insert(pRecord);
      }
    }

    for (auto record : report->suspected()) {
      if (record.server() == self()) {
        // Reports of our death were greatly exaggerated.
        VLOG(2) << *sender << " reported this server (" << self() << ") as 'suspected'; pinging";
        SwimClient client(*sender, port());
        client.Ping();
        continue;
      }

      std::shared_ptr<ServerRecord> pRecord(new ServerRecord(record));
      if (suspected_.find(pRecord) == suspected_.end()) {

        pRecord->set_didgossip(false);
        pRecord->set_timestamp(::utils::CurrentTime());

        auto was_alive = alive_.find(pRecord);
        if (was_alive != alive_.end() && (*was_alive)->timestamp() < record.timestamp()) {
          // This is genuine new news and we need to update our records.
          suspected_.insert(pRecord);
          alive_.erase(pRecord);
        } else if (was_alive == alive_.end()) {
          // If we are here, it means that we haven't seen this server before, and
          // we should record it as "suspicious."
          suspected_.insert(pRecord);
        }
      }
    }

    VLOG(2) << "After merging, alive: " << alive_
            << "; suspected: " << suspected_;
  }

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
   * <p><strong>Note</strong> that this server class is currently <strong>NOT</strong>
   * thread-safe and that none of its members (apart from the `atomic<bool>` stopped_) should be
   * modified while processing this call.
   *
   * @param sender the server that just sent the message; the callee obtains ownership of the
   *        pointer and is responsible for freeing the memory.
   * @param destination the other server that the requestor (`client`) is asksing this server to
   *        ping and report status back.
   * @param timestamp when the message was sent (according to the `client`'s clock).
   */
  virtual void onPingRequest(Server* sender, Server* destination);

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


  const ServerRecordsSet& alive() const { return alive_; }

  const ServerRecordsSet& suspected() const { return suspected_; }

  /**
   * Gives access to the internally-kept list of servers that are deemed to be "alive" and
   * responding to pings (or, conversely, have just recently pinged this server).
   *
   * <p>The returned pointer, while guaranteed to be valid for the duration of this object's
   * lifetime, should not be stored and should only be assumed valide temporarily.
   *
   * <p>If you only need access to the set of servers, without modifying it, it is best to use
   * the `alive()` method instead.
   *
   * @return a pointer that allows modifying the list of internally kept healthy servers.
   */
  ServerRecordsSet* const mutable_alive() { return &alive_; }

  /**
   * Gives access to the internally-kept list of servers that are deemed to be "unhealthy," i.e.
   * not responding to pings from this server; but not for long enough (`grace_period()`) to have
   * been considered "dead."
   *
   * <p>The returned pointer, while guaranteed to be valid for the duration of this object's
   * lifetime, should not be stored and should only be assumed valide temporarily.
   *
   * <p>If you only need access to the set of servers, without modifying it, it is best to use
   * the `suspected()` method instead.
   *
   * @return a pointer that allows modifying the list of internally kept unhealthy servers.
   */
  ServerRecordsSet* const mutable_suspected() { return &suspected_; }
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



