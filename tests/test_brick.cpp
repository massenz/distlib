// This file is (c) 2016 AlertAvert.com.  All rights reserved.
// Created by M. Massenzio, 2016-03-05


#include <gtest/gtest.h>

#include "brick.hpp"


using namespace std;

TEST(HashTests, CanHashOneWord) {
  const string mesg = "test";

  const string hash = hash_str(mesg);
  ASSERT_EQ("098f6bcd4621d373cade4e832627b4f6", hash);

}


TEST(Hash, CanHashComplexString) {
  const string mesg = "A more complex ** /string +/- @55";
  const string result = hash_str(mesg);

  ASSERT_EQ("c86748802a2ff2f09419e4625e70d1fd", result);
}


TEST(Hash, UseCharArrays) {
  const string base = "simple string";
  const string result = hash_str(base);

  unsigned char* buf = (unsigned char*)"simple string";
  EXPECT_EQ(result, hash_str((char *)buf));
}
