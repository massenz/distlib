// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.




#include <memory>
#include <thread>

#include <gtest/gtest.h>

#include "swim/GossipFailureDetector.hpp"

using namespace swim;


TEST(GossipFailureDetector, create) {
  GossipFailureDetector detector(10, 20, 30, 40);

  Server h1;
  h1.set_hostname("h1");
  h1.set_ip_addr("10.10.1.5");
  h1.set_port(8080);

  detector.AddNeighbor(h1);
  // Adding twice the same server will have no effect.
  detector.AddNeighbor(h1);
  ASSERT_EQ(1, detector.alive().size());

  // However, a different port is regarded as a different server.
  h1.set_port(8090);
  detector.AddNeighbor(h1);

  ASSERT_EQ(2, detector.alive().size());
};
