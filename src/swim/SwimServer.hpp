// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.

#pragma once

#include <atomic>
#include <memory>

#include <zmq.hpp>
#include <chrono>
#include <thread>
#include <iomanip>

#include "swim.pb.h"
#include "utils/network.h"


namespace swim {

class SwimServer {

  unsigned short port_;
  unsigned int num_threads_;
  std::atomic<bool> stopped_;
  unsigned long polling_interval_;

protected:

  /**
   * Helper method to log the client hostname and the timestamp.
   *
   * @param client
   * @param timestamp
   */
  void logClient(const Server& client, const std::string& msg) {
    VLOG(2) << "Received message from '" << client.hostname() << "': " << msg;
  }

  /**
   * Invoked when the `client` sends a `SwimEnvelope::Type::STATUS_UPDATE` message to this server.
   *
   * <p>This is essentially a callback method that will be invoked by the server's loop, in its
   * own thread: the default implementation simply logs the request and returns.
   *
   * <p><strong>Note</strong> that this server class is currently <strong>NOT</strong>
   * thread-safe and that none of its members (apart from the `atomic<bool>` stopped_) should be
   * modified while processing this call.
   *
   * @param client the server that just sent the message; the callee obtains ownership of the
   *        pointer and is responsible for freeing the memory.
   * @param timestamp when the message was sent (according to the `client`'s clock).
   */
  virtual void onUpdate(Server* client) {
    std::unique_ptr<Server> ps(client);
    logClient(*ps, "ping");
  }

  /**
   * Invoked when the `server` sends a `SwimEnvelope::Type::STATUS_REPORT` message to this server.
   *
   * <p>This is essentially a callback method that will be invoked by the server's loop, in its
   * own thread: the default implementation simply logs the request and returns.
   *
   * <p><strong>Note</strong> that this server class is currently <strong>NOT</strong>
   * thread-safe and that none of its members (apart from the `atomic<bool>` stopped_) should be
   * modified while processing this call.
   *
   * @param client the server that just sent the message; the callee obtains ownership of the
   *        pointer and is responsible for freeing the memory.
   * @param report the `SwimReport` that the `client` sent and can be used by this server to
   *        update its membership list.
   * @param timestamp when the message was sent (according to the `client`'s clock).
   */
  virtual void onReport(Server* client, SwimReport* report) {
    std::unique_ptr<Server> ps(client);
    logClient(*ps, "received a report");
  }

  /**
   * Invoked when the `client` sends a `SwimEnvelope::Type::STATUS_REQUEST` message to this server.
   *
   * <p>This is essentially a callback method that will be invoked by the server's loop, in its
   * own thread: the default implementation simply logs the request and returns.
   *
   * <p><strong>Note</strong> that this server class is currently <strong>NOT</strong>
   * thread-safe and that none of its members (apart from the `atomic<bool>` stopped_) should be
   * modified while processing this call.
   *
   * @param client the server that just sent the message; the callee obtains ownership of the
   *        pointer and is responsible for freeing the memory.
   * @param destination the other server that the requestor (`client`) is asksing this server to
   *        ping and report status back.
   * @param timestamp when the message was sent (according to the `client`'s clock).
   */
  virtual void onPingRequest(Server* client, Server* destination) {
    std::unique_ptr<Server> ps(client);
    logClient(*ps, "request to ping " + destination->hostname());
  }

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
    while (isRunning() && retry_count > 0) {
      stop();
      VLOG(2) << "Waiting for server to stop...";
      std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_POLLING_INTERVAL_MSEC));
      retry_count--;
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
};

} // namespace swim



