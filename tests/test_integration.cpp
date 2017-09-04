// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 7/23/17.


#include <memory>
#include <thread>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "swim/SwimClient.hpp"
#include "swim/GossipFailureDetector.hpp"

#include "tests.h"

using namespace swim;
using namespace std::chrono;


class IntegrationTests : public ::testing::Test {
protected:
  std::shared_ptr<GossipFailureDetector> detector;

  void SetUp() override {
    // The intervals, timeouts etc. configured here are just for convenience's sake:
    // if a test requires different timings, just stop the threads, change the values, then
    // restart the background threads:
    //
    // detector
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
    const_cast<SwimServer&>(detector->gossip_server()).stop();
    ASSERT_TRUE(::tests::WaitAtMostFor([this]() -> bool {
      return !detector->gossip_server().isRunning();
    }, milliseconds(500)));
  }

  const SwimServer& server() { return detector->gossip_server(); }
};

TEST_F(IntegrationTests, detectFailingNeighbor) {

  auto neighbor = std::unique_ptr<SwimServer>(new SwimServer(::tests::RandomPort()));
  std::thread neighbor_thread([&neighbor]() { neighbor->start(); });

  detector->AddNeighbor(neighbor->self());
  EXPECT_EQ(1, server().alive_size());

// Give other background threads a chance to do work, and verify the neighbor is still alive.
  std::this_thread::sleep_for(seconds(2));
  EXPECT_EQ(1, server().alive_size());

  neighbor->stop();
  EXPECT_FALSE(neighbor->isRunning());
  neighbor_thread.join();

// Wait long enough for the stopped server to be suspected, but not evicted.
  std::this_thread::sleep_for(seconds(2));
  EXPECT_EQ(1, server().suspected_size());

// Now, wait long enough for the stopped server to be evicted.
  std::this_thread::sleep_for(seconds(3));
  EXPECT_TRUE(server().suspected_empty());
}

TEST_F(IntegrationTests, gossipSpreads) {
  // For this test to work, we need the grace period to be long enough for the neighbor to pick
  // this up.
  detector->StopAllBackgroundThreads();

  detector->set_grace_period(seconds(3));

  ASSERT_TRUE(detector->gossip_server().isRunning());
  detector->InitAllBackgroundThreads();

  auto neighbor = std::unique_ptr<SwimServer>(new SwimServer(::tests::RandomPort()));
  std::thread neighbor_thread([&]() { neighbor->start(); });

  auto flaky = std::unique_ptr<SwimServer>(new SwimServer(::tests::RandomPort()));
  std::thread flaky_thread([&]() { flaky->start(); });

  ASSERT_TRUE(::tests::WaitAtMostFor([&]() -> bool {
    return neighbor->isRunning() && flaky->isRunning();
  }, milliseconds(500)));

  detector->AddNeighbor(neighbor->self());
  detector->AddNeighbor(flaky->self());

  // Verify that the happy news about flaky has traveled to the neighbor
  // within a reasonable time frame (see the paper in the README References
  // for a mathematical derivation of a rigorous upper bound: this one it sure ain't).
  ASSERT_TRUE(::tests::WaitAtMostFor([&neighbor]() -> bool {
    // TODO: this needs to change to == 2 once we fix didGossip (see #149950890)
    // Until then, this test is flaky, as the value depends on whether `neighbor` gets pinged
    // first or `flaky` does.
        return neighbor->alive().size() >= 1;
      }, milliseconds(6000))
  ) << neighbor->alive();

  flaky->stop();
  ASSERT_TRUE(::tests::WaitAtMostFor([&flaky]() -> bool {
        return !flaky->isRunning();
    }, milliseconds(200))
  );
  flaky_thread.join();

  // Give the detector enough time to ping and make reports.
  std::this_thread::sleep_for(seconds(2));
  ASSERT_EQ(1, server().suspected_size());

  // It should now be suspected, but still the grace period should have not expired.
  std::this_thread::sleep_for(seconds(1));
  EXPECT_EQ(1, neighbor->suspected().size());

  neighbor->stop();
  ASSERT_TRUE(::tests::WaitAtMostFor([&]() -> bool { return !neighbor->isRunning(); },
                                     milliseconds (200)));
  neighbor_thread.join();

  // Give the detector enough time to evict all the now-gone servers.
  std::this_thread::sleep_for(seconds(5));
  EXPECT_TRUE(server().alive_empty());
  EXPECT_TRUE(server().suspected_empty());
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
  EXPECT_EQ(5, server().alive_size());

  // Stopping the background threads will cause the alive/suspected to stay frozen where they are
  // now.
  detector->StopAllBackgroundThreads();

  for (const auto &server : neighbors) {
    server->stop();
  }

  std::this_thread::sleep_for(seconds(3));
  EXPECT_EQ(5, server().alive_size());

  // Now, when we restart, we should pretty soon find out they're all gone.
  detector->InitAllBackgroundThreads();
  std::this_thread::sleep_for(seconds(6));
  EXPECT_TRUE(server().alive_empty());
}
