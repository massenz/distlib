// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/7/16.


#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../include/ConsistentHash.hpp"
#include "../include/Bucket.hpp"

TEST(BucketTests, CanCreate) {
  Bucket b("test_bucket", {0.3, 0.6, 0.9});

  EXPECT_EQ(3, b.partitions());

  // We verify that all partition points are within the unit circle and sorted.
  float last = 0.0;
  for (auto current : b.partition_points()) {
    EXPECT_GT(current, 0.0);
    EXPECT_LT(current, 1.0);
    EXPECT_GT(current, last);
    last = current;
  }
  EXPECT_EQ("test_bucket", b.name());
}

TEST(BucketTests, CanCreateWithValues) {
  std::vector<float> points { 0.3, 0.6, 0.9 };
  Bucket b("with_points", points);

  ASSERT_EQ(points.size(), b.partitions());
  for (int i = 0; i < points.size(); ++i) {
    ASSERT_FLOAT_EQ(points[i], b.partition_point(i));
  }
}


TEST(BucketTests, CanAddPoint) {
  std::vector<float> points { 0.2, 0.4, 0.6, 0.8 };
  Bucket b("with_points", points);

  ASSERT_EQ(points.size(), b.partitions());
  for (int i = 0; i < points.size(); ++i) {
    ASSERT_FLOAT_EQ(points[i], b.partition_point(i));
  }

  b.add_partition_point(0.7);
  ASSERT_FLOAT_EQ(0.7, b.partition_point(3)) << "Point was not added properly " << b;
}


TEST(BucketTests, CanRemovePoint) {
  std::vector<float> points { 0.2, 0.4, 0.6, 0.8 };
  Bucket b("with_points", points);

  b.remove_partition_point(2);

  ASSERT_EQ(points.size() - 1, b.partitions());
  for (auto x : b.partition_points()) {
    ASSERT_NE(0.6, x);
  }
}


TEST(BucketTests, CanFindNearest) {
  Bucket b("abucket", {0.0422193,
                       0.0592362, 0.119673, 0.215251, 0.90553});
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
  ASSERT_EQ(std::make_pair(i, b.partition_point(i)), b.partition_point(hv));

  Bucket b2("another_bucket", {0.065193,
                       0.052362, 0.19673, 0.2551, 0.9553});
  points = b.partition_points();
  hv = consistent_hash("a different test string");

  for (i = 0; i < 5; ++i) {
    if ((points)[i] > hv) break;
  }
  ASSERT_EQ(std::make_pair(i, b.partition_point(i)), b.partition_point(hv));

}


TEST(BucketTests, CanPrint) {
  std::ostringstream os;
  Bucket b("fancy bucket", {0.065193, 0.052362, 0.19673, 0.2551, 0.9553});
  os << b;
  ASSERT_THAT(os.str(), ::testing::StartsWith("'fancy bucket' ["));
}
