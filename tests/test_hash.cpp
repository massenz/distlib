// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio, 2016-03-05

// Ignore CLion warning caused by GTest TEST() macro.
#pragma ide diagnostic ignored "cert-err58-cpp"


#include <gtest/gtest.h>

#include "utils/utils.hpp"

using namespace std;
using namespace utils;

TEST(HashTests, CanHashOneWord) {
  const string message = "test";

  const string hash = hash_str(message);
  ASSERT_EQ("098f6bcd4621d373cade4e832627b4f6", hash);
}


TEST(HashTests, CanHashComplexString) {
  const string msg = "A more complex ** /string +/- @55";
  const string result = hash_str(msg);

  ASSERT_EQ("c86748802a2ff2f09419e4625e70d1fd", result);
}


TEST(HashTests, UseCharArrays) {
  const string base = "simple string";
  const string result = hash_str(base);

  auto* buf = (unsigned char*)"simple string";
  EXPECT_EQ(result, hash_str((char *)buf));
}
