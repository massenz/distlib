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

  static const unsigned int NUM_THREADS = 5;

  unsigned int port_;
  unsigned int threads_;
  std::atomic<bool> stopped_;

  std::unique_ptr<zmq::socket_t> socket_;

//  void processMessage();

public:
  SwimServer(unsigned int port, unsigned int threads = NUM_THREADS) :
      port_(port), threads_(threads), stopped_(false) {}
  virtual ~SwimServer() {}

  // TODO: create and register all the callbacks for each different payload type.
  void start();

  void stop() { stopped_.store(true); }

  bool isRunning() const { return stopped_.load(); }

  void onUpdate(const Server &server);
};

} // namespace swim



