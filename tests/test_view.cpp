// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.


#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "ConsistentHash.hpp"
#include "View.hpp"

using namespace std;

TEST(ViewTests, CanCreate) {
  View v;
}

TEST(ViewTests, CanAddBucket) {
  auto pb = std::make_shared<Bucket>("test_bucket",
                                     std::vector<float>{0.2, 0.4, 0.6, 0.8, 0.9});
  View v;
  ASSERT_NO_FATAL_FAILURE(v.Add(pb));
  ASSERT_EQ(1, v.num_buckets());
}

TEST(ViewTests, CanAddBuckets) {
  auto pb = std::make_shared<Bucket>("test_bucket",
                                     std::vector<float>{0.2, 0.4, 0.6, 0.8, 0.9});
  View v;
  ASSERT_NO_FATAL_FAILURE(v.Add(pb));
  ASSERT_EQ(1, v.num_buckets());

  auto pb2 = std::make_shared<Bucket>("test_bucket-2",
                                std::vector<float>{0.3, 0.5, 0.7});

  ASSERT_NO_FATAL_FAILURE(v.Add(pb2));
  ASSERT_EQ(2, v.num_buckets());

  ASSERT_EQ(pb, v.FindBucket(0.35));
  ASSERT_EQ(pb, v.FindBucket(0.77));
  ASSERT_EQ(pb2, v.FindBucket(0.4856));
}

TEST(ViewTests, CanFindBucket) {
  auto pb = std::make_shared<Bucket>("test_bucket",
                                     std::vector<float>{0.1, 0.3, 0.5, 0.7, 0.9});
  View v;
  ASSERT_NO_FATAL_FAILURE(v.Add(pb));

  // As this is the only bucket, anything will be assigned to it.
  auto found = v.FindBucket(0.33);
  ASSERT_EQ(pb, found);
}

TEST(ViewTests, CanEmitToStdout) {
  View v;
  auto pb1 = std::make_shared<Bucket>("test-1", std::vector<float>{0.2, 0.6, 0.9});
  auto pb2 = std::make_shared<Bucket>("test-2", std::vector<float>{0.4, 0.8, 0.7});
  auto pb3 = std::make_shared<Bucket>("test-3", std::vector<float>{0.3, 0.5, 0.8, 0.95});

  v.Add(pb1);
  v.Add(pb2);
  v.Add(pb3);
  ASSERT_EQ(3, v.num_buckets());

  ASSERT_NO_FATAL_FAILURE(std::cout << v << std::endl);
}

TEST(ViewTests, CanRemoveBucket) {
  View v;
  auto pb1 = std::make_shared<Bucket>("test-1", std::vector<float>{0.2, 0.6, 0.9});
  auto pb2 = std::make_shared<Bucket>("test-2", std::vector<float>{0.4, 0.8, 0.7});
  auto pb3 = std::make_shared<Bucket>("test-3", std::vector<float>{0.3, 0.5, 0.8, 0.95});

  v.Add(pb1);
  v.Add(pb2);
  v.Add(pb3);
  EXPECT_EQ(3, v.num_buckets());

  EXPECT_TRUE(v.Remove(pb2));
  EXPECT_EQ(2, v.num_buckets());

  EXPECT_TRUE(v.Remove(pb1));
  EXPECT_FALSE(v.Remove(pb2));
  EXPECT_EQ(1, v.num_buckets());

  EXPECT_FALSE(v.Remove(pb1));
  EXPECT_FALSE(v.Remove(pb1));
  EXPECT_EQ(1, v.num_buckets());

  EXPECT_TRUE(v.Remove(pb3));
  EXPECT_EQ(0, v.num_buckets());
}


// Using consistent hash, adding a node should result in a
// very small number of items in need of being moved around.
TEST(ViewTests, RebalanceLoad) {
  const int NUM_SAMPLES = 1000;
  const int NUM_BUCKETS = 10;
  const int NUM_PARTS = 3;

  View v;
  std::map<std::string, BucketPtr> map_items_to_hosts;
  BucketPtr buckets[NUM_BUCKETS];

  float delta = 1.0 / (NUM_BUCKETS * NUM_PARTS);

  for (int i = 0; i < NUM_BUCKETS; ++i) {
    buckets[i] = std::make_shared<Bucket>(
        "host-" + std::to_string(i) + ".example.com",
        std::vector<float>{static_cast<float>(i * delta),
                           static_cast<float>(0.34 + i * delta),
                           static_cast<float>(0.67 + i * delta)}
    );
    v.Add(buckets[i]);
  }

  for (int i = 0; i < NUM_SAMPLES; ++i) {
    std::string s("random " + std::to_string(i));
    map_items_to_hosts[s] = v.FindBucket(consistent_hash(s));
  }

  auto new_bucket = std::make_shared<Bucket>("new-host.example.com",
                                             std::vector<float>{0.3, 0.6, 0.9});
  v.Add(new_bucket);

  int rebalance_counts = 0;
  for (const auto &item : map_items_to_hosts) {
    auto bucket = v.FindBucket(consistent_hash(item.first));
    if (bucket != item.second) {
      rebalance_counts++;
      map_items_to_hosts[item.first] = bucket;
    }
  }
  VLOG(2) << "ADD: " << rebalance_counts;

  // Adding a node should cause around one C-th of items to be re-shuffled,
  // if C is the number of caches (in this case, 1/10-th).
  auto expected_reshuffles = 1.1 * float(NUM_SAMPLES) / NUM_BUCKETS;
  ASSERT_LT(rebalance_counts, expected_reshuffles)
                << "ADD: Too many reshuffles " << rebalance_counts;

  v.Remove(buckets[8]);
  rebalance_counts = 0;
  for (const auto &item : map_items_to_hosts) {
    if (v.FindBucket(consistent_hash(item.first)) != item.second) rebalance_counts++;
  }
  VLOG(2) << "REMOVE: " << rebalance_counts;

  // Similarly, removing, should only cause the items with the removed node to be rebalanced.
  ASSERT_LT(rebalance_counts, float(NUM_SAMPLES) / NUM_BUCKETS)
                << "REMOVE: Too many reshuffles " << rebalance_counts;
}

TEST(ViewTests, CreateBalancedViewThrows) {
  try {
    auto pv = make_balanced_view(-1, 10);
    FAIL() << "It should have thrown here";
  } catch (const std::invalid_argument &ex) {
    SUCCEED();
  }
}

TEST(ViewTests, CreateBalancedView) {
  try {
    auto pv = make_balanced_view(3, 10);
    ASSERT_EQ(3, pv->num_buckets());

    auto b = *pv->buckets().cbegin();
    ASSERT_EQ(10, b->partitions());

    pv->Clear();

  } catch (const std::invalid_argument &ex) {
    FAIL() << "Creating the view throws: " << ex.what();
  }
}

TEST(ViewTests, CanGetBucketsAndUse) {
  auto pv = make_balanced_view(5, 15);
  auto buckets = pv->buckets();
  ASSERT_EQ(5, buckets.size());

  auto pos = buckets.begin();
  (*pos)->set_name("new bucket");
  pos++;
  pos++;
  (*pos)->set_name("new bucket");

  int count = 0;
  auto new_buckets = pv->buckets();
  std::for_each(new_buckets.begin(), new_buckets.end(),
                [&count](const BucketPtr &bucket_ptr) {
                  if (bucket_ptr->name() == "new bucket") {
                    count++;
                  }
                });
  ASSERT_EQ(2, count);

}

TEST(ViewTests, RenameBuckets) {
  auto pv = make_balanced_view(3, 10);
  ASSERT_EQ(3, pv->num_buckets());

  std::vector<std::string> names{"pippo", "pluto", "paperino"};
  pv->RenameBuckets(names.cbegin(), names.cend());

  auto buckets = pv->buckets();
  std::vector<string> bucket_names;
  bucket_names.reserve(buckets.size());
  for (auto b : buckets) {
    bucket_names.push_back(b->name());
  }
  ASSERT_THAT(bucket_names, ::testing::UnorderedElementsAreArray(names));
}

TEST(ViewTests, RenameBucketsNotEnoughNames) {
  auto pv = make_balanced_view(5, 10);
  ASSERT_EQ(5, pv->num_buckets());

  std::vector<std::string> names{"Qui", "Quo", "Qua"};
  pv->RenameBuckets(names.cbegin(), names.cend());

  std::vector<string> buckets_names;
  auto buckets = pv->buckets();
  ASSERT_EQ(buckets.size(), pv->num_buckets());

  for (const auto &b : buckets) {
    buckets_names.push_back(b->name());
  }

  for (const auto& name : names) {
    auto pos = std::find(buckets_names.cbegin(), buckets_names.cend(), name);
    ASSERT_NE(pos, buckets_names.cend());
  }
}
