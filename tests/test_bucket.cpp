// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/7/16.

// Ignore CLion warning caused by GTest TEST() macro.
#pragma ide diagnostic ignored "cert-err58-cpp"


#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Bucket.hpp"
#include "ConsistentHash.hpp"


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

TEST(BucketTests, CanSetName) {
  Bucket b("bucket", {0.065193, 0.052362, 0.19673, 0.2551, 0.9553});
  ASSERT_EQ(b.name(), "bucket");

  b.set_name("another");
  ASSERT_EQ(b.name(), "another");
}


TEST(BucketTests, ThrowsOutOfRange) {
  Bucket b("bucket", {0.065193, 0.052362});
  ASSERT_EQ(b.partitions(), 2);
  ASSERT_THROW(b.partition_point(b.partitions() + 2), std::out_of_range);
}


TEST(BucketTests, Json) {

  json bj = Bucket {"my-bucket", {0.5f, 0.8f}};

  ASSERT_EQ("my-bucket", bj["name"]);
  ASSERT_TRUE(bj["partition_points"].is_array());

  ASSERT_EQ(0.5f, bj["partition_points"][0]);
  ASSERT_EQ(0.8f, bj["partition_points"][1]);
}


TEST(BucketTests, JsonArray) {

  std::vector<Bucket> buckets = {
      Bucket {"my-bucket", {0.5f, 0.8f}},
      Bucket {"another", {0.6f, 0.9f}},
      Bucket {"last", {0.7f, 0.1f}},
  };

  json myBuckets;
  myBuckets["buckets"] = buckets;

  ASSERT_TRUE(myBuckets["buckets"].is_array());
  ASSERT_EQ(3, myBuckets["buckets"].size());
  ASSERT_EQ("another", myBuckets["buckets"][1]["name"]);
}
