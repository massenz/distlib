// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.




#include <memory>
#include <thread>

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <swim/SwimClient.hpp>

#include "swim/GossipFailureDetector.hpp"

using namespace swim;


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
  records.insert(MakeRecord(*sameServer));

  ASSERT_EQ(2, records.size());

}
