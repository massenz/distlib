// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.




#include <memory>
#include <thread>

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <swim/SwimClient.hpp>

#include "swim/GossipFailureDetector.hpp"

using namespace swim;


std::unique_ptr<GossipFailureDetector> makeDetector() {
  std::unique_ptr<GossipFailureDetector> p(new GossipFailureDetector(2222, 10, 5, 5, 5));
  return p;
}

TEST(GossipFailureDetectorTests, create) {

  LOG(INFO) << "starting gossip...";
  GossipFailureDetector detector(5678, 10, 20, 30, 40);
  LOG(INFO) << "gossip started";

  Server h1;
  h1.set_hostname("h1");
  h1.set_ip_addr("10.10.1.5");
  h1.set_port(8080);

  detector.AddNeighbor(h1);
  // Adding twice the same server will have no effect.
  detector.AddNeighbor(h1);
  ASSERT_EQ(1, detector.alive().size());

  // However, a different port is regarded as a different server.
  Server h2;
  h2.set_hostname("h1");
  h2.set_ip_addr("10.10.1.5");
  h1.set_port(8090);
  detector.AddNeighbor(h2);

  ASSERT_EQ(2, detector.alive().size());
};

TEST(GossipFailureDetectorTests, recordsets) {

  ServerRecordsSet records;

  std::shared_ptr<Server> server = MakeServer("localhost", 8081);
  std::shared_ptr<Server> server2 = MakeServer("localhost", 8088);
  std::shared_ptr<Server> sameServer = MakeServer("localhost", 8081);

  records.insert(MakeRecord(*server));
  records.insert(MakeRecord(*server2));
  ASSERT_EQ(2, records.size());

  // A server with the same hostname/port will be considered equal and thus
  // will NOT be added to the Set.
  records.insert(MakeRecord(*sameServer));
  ASSERT_EQ(2, records.size());
}

TEST(GossipFailureDetectorTests, streamOut) {
  ServerRecordsSet records;

  std::shared_ptr<Server> server = MakeServer("localhost", 8081);
  std::shared_ptr<Server> server2 = MakeServer("localhost", 8088);

  records.insert(MakeRecord(*server));
  records.insert(MakeRecord(*server2));

  std::ostringstream s;
  s << records;
  ASSERT_EQ(0, s.str().find("{[From: (localhost:8081) at:"));
  ASSERT_GT(200, s.str().find(", [From: (localhost:8088) at:"));
}

TEST(GossipFailureDetectorTests, addNeighbors) {
  std::shared_ptr<Server> server = MakeServer("host1.example.com", 8087),
      server2 = MakeServer("host2.test.net", 9099);

  auto detector = makeDetector();
  ASSERT_TRUE(detector->alive().empty());

  detector->AddNeighbor(*server);
  detector->AddNeighbor(*server2);

  ASSERT_EQ(2, detector->alive().size());

  std::shared_ptr<Server> server3 = MakeServer("another.example.com", 4456);
  detector->AddNeighbor(*server3);
  ASSERT_EQ(3, detector->alive().size());
}
