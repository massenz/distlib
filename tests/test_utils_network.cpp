// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/7/16.


#include <gtest/gtest.h>

#include "utils/utils.hpp"

using namespace utils;
using namespace std;


TEST(NetworkTests, CanGetHostname) {
  string host = Hostname();
  EXPECT_FALSE(host.empty());
}

TEST(NetworkTests, CanGetSocket) {
  string socket = SocketAddress(9909);
  EXPECT_FALSE(socket.empty());
  ASSERT_NE(std::string::npos, socket.find("tcp://"));
}

TEST(NetworkTests, CanGetIPAddr) {
  string ip_addr = InetAddress("localhost");
  ASSERT_EQ("127.0.0.1", ip_addr);

  ip_addr = InetAddress("google.com");
  ASSERT_FALSE(ip_addr.empty());

  ASSERT_FALSE(InetAddress().empty());
}
