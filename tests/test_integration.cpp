// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 7/23/17.


#include <memory>
#include <thread>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <SimpleHttpRequest.hpp>
#include <google/protobuf/util/json_util.h>

#include "../include/swim/SwimClient.hpp"
#include "../include/swim/GossipFailureDetector.hpp"
#include "apiserver/api/rest/ApiServer.hpp"

#include "tests.h"

using namespace swim;
using namespace std::chrono;
using namespace google::protobuf::util;


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
        20  // ping timeout, in milliseconds: can be really short.
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
    const_cast<SwimServer &>(detector->gossip_server()).stop();
    ASSERT_TRUE(::tests::WaitAtMostFor([this]() -> bool {
      return !detector->gossip_server().isRunning();
    }, milliseconds(500)));
  }

  const SwimServer &server() { return detector->gossip_server(); }
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
    return neighbor->alive_size() >= 1;
  }, milliseconds(6000))
  ) << "Failed to register new 'flaky' neighbor before timeout";

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
  EXPECT_EQ(1, neighbor->suspected_size());

  neighbor->stop();
  ASSERT_TRUE(::tests::WaitAtMostFor([&]() -> bool { return !neighbor->isRunning(); },
                                     milliseconds(200)));
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


TEST_F(IntegrationTests, wrongApiServerEndpointReturnsNotFound) {
  std::shared_ptr<api::rest::ApiServer> server =
      std::make_shared<api::rest::ApiServer>(7999);

  try {
    request::SimpleHttpRequest simpleClient;
    simpleClient.timeout = 2500;

    simpleClient.get("http://localhost:7999/not/valid/api")
        .on("error", [](request::Error &&err) -> void {
          FAIL() << "Could not connect to API Server: "
                 << err.message;
        }).on("response", [](request::Response &&res) -> void {
          EXPECT_EQ(404, res.statusCode);
          EXPECT_NE(std::string::npos, res.str().find("Unknown API endpoint")) << "Found "
                    "instead: " << res.str();
        }).end();
  } catch (const std::exception &e) {
    FAIL() << e.what();
  }
}


TEST_F(IntegrationTests, reportsApiServer) {
  auto neighbor = std::make_unique<SwimServer>(::tests::RandomPort());
  std::thread neighbor_thread([&]() { neighbor->start(); });

  ASSERT_TRUE(::tests::WaitAtMostFor([&]() -> bool {
    return neighbor->isRunning();
  }, milliseconds(500)));
  detector->AddNeighbor(neighbor->self());


  std::shared_ptr<api::rest::ApiServer> server =
      std::make_shared<api::rest::ApiServer>(7999);
  ASSERT_NE(nullptr, server.get());

  // Verify that we can get an empty Report.
  request::SimpleHttpRequest simpleClient;
  try {
    simpleClient.setHeader("Accept", "application/json");
    simpleClient.timeout = 2500;
    simpleClient.get("http://localhost:7999/api/v1/report")
        .on("error", [](request::Error &&err) {
          FAIL() << "Could not connect to API Server: "
                 << err.message;
        }).on("response", [this, &neighbor](request::Response &&res) {
          EXPECT_FALSE(res.str().empty());
          SwimReport report;
          auto status = JsonStringToMessage(res.str(), &report);
          if (!status.ok()) {
            FAIL() << "Cannot conver JSON (" << status.error_code()
                   << "): " << status.error_message();
          }
          std::for_each(res.headers.begin(), res.headers.end(),
                        [](std::pair<std::string, std::string> header) {
                          LOG(INFO) << header.first << ": " << header.second;
                        });
          EXPECT_EQ("application/json", res.headers["content-type"]);
          EXPECT_EQ(report.sender(), detector->gossip_server().self());
          EXPECT_EQ(1, report.alive_size());
          EXPECT_EQ(neighbor->self(), report.alive(0).server());
        }).end();
  } catch (const std::exception &e) {
    FAIL() << e.what();
  }

  ASSERT_TRUE(::tests::WaitAtMostFor([&]() -> bool {
    neighbor->stop();
    neighbor_thread.join();
    return true;
  }, milliseconds(400)));
}


TEST_F(IntegrationTests, postApiServer) {
  auto neighbor = std::make_unique<SwimServer>(::tests::RandomPort());
  Server svr = neighbor->self();
  std::string jsonBody;
  auto status = ::google::protobuf::util::MessageToJsonString(svr, &jsonBody);
  ASSERT_TRUE(status.ok()) << "Could not parse PB into JSON";

  std::shared_ptr<api::rest::ApiServer> server =
      std::make_shared<api::rest::ApiServer>(7999);

  // Verify that we can get an empty Report.
  request::SimpleHttpRequest simpleClient;
  simpleClient.timeout = 2500;

  try {
    simpleClient.setHeader("Content-Type", "application/json");
    simpleClient.post("http://localhost:7999/api/v1/server", jsonBody)
        .on("error", [](request::Error &&err) {
          FAIL() << "Could not connect to API Server: "
                 << err.message;
        }).on("response", [this, &neighbor](request::Response &&res) {
          EXPECT_TRUE(res.good());
          EXPECT_EQ("OK", res.str());
          EXPECT_EQ(201, res.statusCode);
        }).end();
  } catch (const std::exception &e) {
    FAIL() << e.what();
  }
}
