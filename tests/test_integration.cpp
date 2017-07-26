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


class IntegrationTests : public ::testing::Test {
protected:
  std::shared_ptr<GossipFailureDetector> detector{};

  void SetUp() override {
    detector.reset(new GossipFailureDetector(
        ::tests::RandomPort(),
        1,  // time between reports
        2,  // grace period, dictates length of tests, must not be too long.
        20, // ping timeout, in milliseconds: can be really short.
        1   // interval between pings.
    ));

    // Wait for the Gossip Detector to start.
    int retries = 5;
    while (retries-- > 0 && !detector->gossip_server().isRunning()) {
      std::this_thread::sleep_for(milliseconds(20));
    }
    EXPECT_TRUE(detector->gossip_server().isRunning());
    detector->InitAllBackgroundThreads();
  }

  void TearDown() override {
    detector->gossip_server().stop();
    ASSERT_TRUE(::tests::WaitAtMostFor([this]() -> bool {
      return !detector->gossip_server().isRunning();
    }, milliseconds(500)));
  }
};

TEST_F(IntegrationTests, detectFailingNeighbor) {

  auto neighbor = std::unique_ptr<SwimServer>(new SwimServer(::tests::RandomPort()));
  std::thread neighbor_thread([&neighbor]() { neighbor->start(); });

  detector->AddNeighbor(neighbor->self());
  EXPECT_EQ(1, detector->alive().size());

// Give other background threads a chance to do work, and verify the neighbor is still alive.
  std::this_thread::sleep_for(seconds(2));
  EXPECT_EQ(1, detector->alive().size());

  neighbor->stop();
  EXPECT_FALSE(neighbor->isRunning());
  neighbor_thread.join();

// Wait long enough for the stopped server to be suspected, but not evicted.
  std::this_thread::sleep_for(seconds(2));
  EXPECT_EQ(1, detector->suspected().size());

// Now, wait long enough for the stopped server to be evicted.
  std::this_thread::sleep_for(seconds(3));
  EXPECT_TRUE(detector->suspected().empty());
}

TEST_F(IntegrationTests, gossipSpreads) {
  auto neighbor = std::unique_ptr<SwimServer>(new SwimServer(::tests::RandomPort()));
  std::thread neighbor_thread([&]() { neighbor->start(); });

  auto flaky = std::unique_ptr<SwimServer>(new SwimServer(::tests::RandomPort()));
  std::thread flaky_thread([&]() { flaky->start(); });

  ASSERT_TRUE(::tests::WaitAtMostFor([&]() -> bool {
    return neighbor->isRunning() && flaky->isRunning();
  }, milliseconds(500)));

  detector->AddNeighbor(neighbor->self());
  detector->AddNeighbor(flaky->self());

  // Give the detector enough time to ping and make reports.
  std::this_thread::sleep_for(seconds(5));

  // Verify that the happy news about flaky have traveled to the neighbor.
  EXPECT_EQ(1, neighbor->alive().size());

  flaky->stop();
  ASSERT_TRUE(::tests::WaitAtMostFor([&]() -> bool { return !flaky->isRunning(); },
                                     milliseconds (200)));
  flaky_thread.join();

  // Give the detector enough time to ping and make reports.
  std::this_thread::sleep_for(seconds(3));

  // It should now be suspected, but still the grace period should have not expired.
  EXPECT_EQ(1, neighbor->suspected().size());

  neighbor->stop();
  ASSERT_TRUE(::tests::WaitAtMostFor([&]() -> bool { return !neighbor->isRunning(); },
                                     milliseconds (200)));
  neighbor_thread.join();

  // Give the detector enough time to ping and make reports.
  std::this_thread::sleep_for(seconds(3));

  EXPECT_TRUE(detector->suspected().empty());
}

TEST_F(IntegrationTests, canStopThreads) {

  std::vector<std::unique_ptr<SwimServer>> neighbors{};

  for (int i = 0; i < 5; ++i) {
    auto neighbor = new SwimServer(::tests::RandomPort());
    std::thread neighbor_thread([&]() { neighbor->start(); });
    neighbor_thread.detach();

    ASSERT_TRUE(::tests::WaitAtMostFor([&]() -> bool {
      return neighbor->isRunning();
    }, milliseconds(500)));

    detector->AddNeighbor(neighbor->self());
    neighbors.push_back(std::unique_ptr<SwimServer>(neighbor));
  }

  std::this_thread::sleep_for(seconds(6));
  EXPECT_EQ(5, detector->alive().size());

  // Stopping the background threads will cause the alive/suspected to stay frozen where they are
  // now.
  detector->StopAllBackgroundThreads();

  for (const auto &server : neighbors) {
    server->stop();
  }

  std::this_thread::sleep_for(seconds(3));
  EXPECT_EQ(5, detector->alive().size());

  // Now, when we restart, we should pretty soon find out they're all gone.
  detector->InitAllBackgroundThreads();
  std::this_thread::sleep_for(seconds(6));
  EXPECT_TRUE(detector->alive().empty());
}
