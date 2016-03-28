// This file is (c) 2016 AlertAvert.com.  All rights reserved.
// Created by M. Massenzio, 2016-03-05


#include "brick.hpp"

#include <gtest/gtest.h>
#include <Bucket.hpp>

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


TEST(BucketTests, CanCreate) {
  Bucket b("test_bucket", 3);
  EXPECT_EQ(3, b.replicas());
  for (int i = 0; i < b.replicas(); ++i) {
    EXPECT_GT(b.hash_points()[i], 0.0);
    EXPECT_LT(b.hash_points()[i], 1.0);
  }
  EXPECT_EQ("test_bucket", b.name());
}

TEST(BucketTests, CanFindNearest) {
  Bucket b("abucket", 5);
  float values[5];
  for (int i = 0; i < 5; ++i) {
    values[i] = b.hash_point(i);
  }
  string s = "the test string";

  float hv = consistent_hash(s);
  float minDist = 1.0;
  int minIdx = -1;
  for (int i = 0; i < 5; ++i) {
    if (hv < values[i] && (values[i] - hv) < minDist) {
      minDist = values[i] - hv;
      minIdx = i;
    }
  }
  ASSERT_EQ(std::make_pair(minIdx, minDist), b.nearest_hash_point(hv));
}


TEST(BucketTests, CanPrint) {
  std::ostringstream os;
  Bucket b("fancy bucket", 6);
  os << b;
  ASSERT_EQ("'fancy bucket' [0.44829, 0.50863, 0.11136, 0.52950, 0.83783, 0.65781]", os.str());
}
