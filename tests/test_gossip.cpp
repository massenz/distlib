// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.




#include <memory>
#include <thread>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "swim/GossipFailureDetector.hpp"

using namespace swim;


TEST(GossipFailureDetector, create) {

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
