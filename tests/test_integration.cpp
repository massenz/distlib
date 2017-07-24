// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 7/23/17.



#include <memory>
#include <thread>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <swim/SwimClient.hpp>

#include "swim/GossipFailureDetector.hpp"

#include "tests.h"

using namespace swim;
using namespace std::chrono;


TEST(IntegrationTests, detectFailingNeighbor) {

  GossipFailureDetector detector(
      ::tests::RandomPort(),
      10, // time between reports, not used here
      2,  // grace period, dictates length of this tests, must not be too long.
      20, // ping timeout, in milliseconds: can be really short.
      1   // interval between pings.
  );

// Wait for the Gossip Detector to start.
  int retries = 5;
  while (retries-- > 0 && !detector.gossip_server().isRunning()) {
    std::this_thread::sleep_for(milliseconds(20));
  }
  EXPECT_TRUE(detector.gossip_server().isRunning());
  detector.InitAllBackgroundThreads();

  auto neighbor = std::unique_ptr<SwimServer>(new SwimServer(::tests::RandomPort()));
  std::thread neighbor_thread([&neighbor]() { neighbor->start(); });

  detector.AddNeighbor(neighbor->self());
  ASSERT_EQ(1, detector.alive().size());

// Give other background threads a chance to do work, and verify the neighbor is still alive.
  std::this_thread::sleep_for(seconds(2));
  ASSERT_EQ(1, detector.alive().size());

  neighbor->stop();
  ASSERT_FALSE(neighbor->isRunning());
  neighbor_thread.join();

// Wait long enough for the stopped server to be suspected, but not evicted.
  std::this_thread::sleep_for(seconds(2));
  EXPECT_EQ(1, detector.suspected().size());

// Now, wait long enough for the stopped server to be evicted.
  std::this_thread::sleep_for(seconds(3));
  EXPECT_TRUE(detector.suspected().empty());
}
