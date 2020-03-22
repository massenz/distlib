// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.


#include <gtest/gtest.h>
#include <ConsistentHash.hpp>
#include "../include/View.hpp"

using namespace std;

TEST(ViewTests, CanCreate) {
  View v;
}

TEST(ViewTests, CanAddBucket) {
  Bucket b("test_bucket", {0.2, 0.4, 0.6, 0.8, 0.9});
  View v;
  ASSERT_NO_FATAL_FAILURE(v.add(&b));

  ASSERT_EQ(1, v.num_buckets());
}


TEST(ViewTests, CanFindBucket)
{
  Bucket b("test_bucket", {0.1, 0.3, 0.5, 0.7, 0.9});
  View v;
  ASSERT_NO_FATAL_FAILURE(v.add(&b));

  // As this is the only bucket, anything will be assigned to it.
  auto found = v.bucket(0.33);
  ASSERT_EQ(&b, found);
}

TEST(ViewTests, CanEmitToStdout)
{
  View v;
  Bucket b1("test-1", {0.2, 0.6, 0.9}),
         b2("test-2", {0.4, 0.8, 0.7}),
         b3("test-3", {0.3, 0.5, 0.8, 0.95});

  v.add(&b1);
  v.add(&b2);
  v.add(&b3);
  ASSERT_EQ(3, v.num_buckets());

  ASSERT_NO_FATAL_FAILURE(std::cout << v << std::endl);
}

TEST(ViewTests, CanRemoveBucket) {
  View v;
  Bucket b1("test-1", {0.1, 0.4, 0.7}),
         b2("test-2", {0.2, 0.5, 0.8}),
         b3("test-3", {0.3, 0.6, 0.9});

  v.add(&b1);
  v.add(&b2);
  v.add(&b3);
  EXPECT_EQ(3, v.num_buckets());

  EXPECT_TRUE(v.remove(&b2));
  EXPECT_EQ(2, v.num_buckets());

  EXPECT_TRUE(v.remove(&b1));
  EXPECT_FALSE(v.remove(&b2));
  EXPECT_EQ(1, v.num_buckets());

  EXPECT_FALSE(v.remove(&b1));
  EXPECT_FALSE(v.remove(&b1));
  EXPECT_EQ(1, v.num_buckets());

  EXPECT_TRUE(v.remove(&b3));
  EXPECT_EQ(0, v.num_buckets());
}


// Using consistent hash, adding a node should result in a
// very small number of items in need of being moved around.
TEST(ViewTests, RebalanceLoad) {
  const int NUM_SAMPLES = 1000;
  const int NUM_BUCKETS = 10;
  const int NUM_PARTS = 3;

  View v;
  std::map<std::string, const Bucket*> map_items_to_hosts;
  Bucket* buckets[NUM_BUCKETS];

  float delta = 1.0 / (NUM_BUCKETS * NUM_PARTS);

  for (int i = 0; i < NUM_BUCKETS; ++i) {
    buckets[i] = new Bucket("host-" + std::to_string(i) + ".example.com",
        {static_cast<float>(i * delta),
         static_cast<float>(0.34 + i * delta),
         static_cast<float>(0.67 + i * delta)});
    v.add(buckets[i]);
  }

  for (int i = 0; i < NUM_SAMPLES; ++i) {
    std::string s("random " + std::to_string(i));
    map_items_to_hosts[s] = v.bucket(consistent_hash(s));
  }

  Bucket new_bucket("new-host.example.com", {0.3, 0.6, 0.9});
  v.add(&new_bucket);

  int rebalance_counts = 0;
  for (const auto& item : map_items_to_hosts) {
    auto bucket = v.bucket(consistent_hash(item.first));
    if (bucket != item.second) {
      rebalance_counts++;
      map_items_to_hosts[item.first] = bucket;
    }
  }
  LOG(INFO) << "ADD: " << rebalance_counts;

  // Adding a node should cause around one C-th of items to be re-shuffled,
  // if C is the number of caches (in this case, 1/10-th).
  auto expected_reshuffles = 1.1 * float(NUM_SAMPLES) / NUM_BUCKETS;
  ASSERT_LT(rebalance_counts, expected_reshuffles) << "ADD: Too many reshuffles " << rebalance_counts;

  v.remove(buckets[8]);
  rebalance_counts = 0;
  for (const auto& item : map_items_to_hosts) {
    if (v.bucket(consistent_hash(item.first)) != item.second) rebalance_counts++;
  }
  LOG(INFO) << "REMOVE: " << rebalance_counts;

  // Similarly, removing, should only cause the items with the removed node to be rebalanced.
  ASSERT_LT(rebalance_counts, float(NUM_SAMPLES) / NUM_BUCKETS) << "REMOVE: Too many reshuffles " << rebalance_counts;
}

