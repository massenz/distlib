// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/7/16.


#include <gtest/gtest.h>

#include "../include/ConsistentHash.hpp"
#include "../include/Bucket.hpp"

TEST(BucketTests, CanCreate) {
  Bucket b("test_bucket", 3);

  EXPECT_EQ(3, b.partitions());

  for (int i = 0; i < b.partitions(); ++i) {
    EXPECT_GT((b.partition_points())[i], 0.0);
    EXPECT_LT((b.partition_points())[i], 1.0);
  }
  EXPECT_EQ("test_bucket", b.name());
}


TEST(BucketTests, CanFindNearest) {
  Bucket b("abucket", 5);
  auto points = b.partition_points();

  float hv = consistent_hash("the test string");

  // we can take advantage of what we know about partition points:
  // they are sorted and there's only 5 of them.
  //
  // During a test run, this is what we have:
  //
  //  hv = 0.362957
  //  i  partition_point(i)
  //  0  0.0422193
  //  1  0.0592362
  //  2  0.119673
  //  3  0.215251
  //  4  0.90553
  int i;
  for (i = 0; i < 5; ++i) {
    if ((points)[i] > hv) break;
  }
  ASSERT_EQ(std::make_pair(i, b.partition_point(i)), b.nearest_partition_point(hv));

  Bucket b2("another_bucket", 4);
  points = b.partition_points();
  hv = consistent_hash("a different test string");

  for (i = 0; i < 5; ++i) {
    if ((points)[i] > hv) break;
  }
  ASSERT_EQ(std::make_pair(i, b.partition_point(i)), b.nearest_partition_point(hv));

}


TEST(BucketTests, CanPrint) {
  std::ostringstream os;
  Bucket b("fancy bucket", 6);
  os << b;
  ASSERT_EQ("'fancy bucket' [0.11136, 0.44829, 0.50863, 0.52950, 0.65781, 0.83783]", os.str());
}
