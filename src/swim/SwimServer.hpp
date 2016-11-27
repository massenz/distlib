// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.

#pragma once

#include <atomic>
#include <memory>

#include <zmq.hpp>

#include "swim.pb.h"
#include "utils/network.h"


namespace swim {

class SwimServer {

  unsigned int port_;
  unsigned int threads_;
  std::atomic<bool> stopped_;
  unsigned long polling_interval_;

protected:

  /**
   * Helper method to log the client hostname and the timestamp.
   *
   * @param client
   * @param timestamp
   */
  void logClient(const Server& client, long timestamp) {
    VLOG(2) << "Message from '" << client.hostname() << "' at "
            << std::put_time(std::gmtime(&timestamp), "%c %Z");
  }

  /**
   * Invoked when the `server` sends a `SwimEnvelope::Type::STATUS_UPDATE` message to this server.
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
  virtual void onUpdate(Server* client, long timestamp) {
    std::unique_ptr<Server> ps(client);
    logClient(*ps, timestamp);
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
  virtual void onReport(Server* client, SwimReport* report, long timestamp) {
    std::unique_ptr<Server> ps(client);
    logClient(*ps, timestamp);
  }

  /**
   * Invoked when the `server` sends a `SwimEnvelope::Type::STATUS_REQUEST` message to this server.
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
  virtual void onPingRequest(Server* client, Server* destination, long timestamp) {
    std::unique_ptr<Server> ps(client);
    logClient(*ps, timestamp);
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


  SwimServer(unsigned int port, unsigned int threads = NUM_THREADS,
             unsigned long polling_interval = DEFAULT_POLLING_INTERVAL_MSEC) :
      port_(port), threads_(threads), stopped_(true),
      polling_interval_(polling_interval) {}

  virtual ~SwimServer() {
    while (isRunning()) {
      stop();
      VLOG(2) << "Waiting for server to stop...";
      std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_POLLING_INTERVAL_MSEC));
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
};

} // namespace swim



