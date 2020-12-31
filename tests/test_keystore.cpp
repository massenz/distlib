// Copyright (c) 2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com)

// Ignore CLion warning caused by GTest TEST() macro.
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <unordered_set>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tests.h"

#include "keystore/InMemoryKeyStore.hpp"

using namespace keystore;
using namespace std;

using KSsl = keystore::InMemoryKeyStore<std::string, long>;
using KSslPtr = std::unique_ptr<KSsl>;

class KeyStoreTests : public ::testing::Test {
 protected:
  KSslPtr store_;
  std::shared_ptr<View> pv_;

  void SetUp() override {
    pv_ = std::move(make_balanced_view(2, 3));
    store_ = std::make_unique<KSsl>("test",
        pv_, std::unordered_set<std::string>{"bucket-0", "bucket-1"});
  }

  void Insert(int start, int end, const std::function<int(int)> &mapper) {
    for (int i = start; i < end; ++i) {
      store_->Put(std::to_string(i), mapper(i));
    }
  }

  void Assert(int start, int end, const std::function<int(int)> &mapper) {
    for (int i = start; i < end; ++i) {
      auto val = store_->Get(std::to_string(i));
      ASSERT_TRUE(val) << "Missing value for i = " << i;
      ASSERT_EQ(mapper(i), *val);
    }
  }
};

TEST_F(KeyStoreTests, CanCreate) {
  ASSERT_NE(nullptr, pv_);
  ASSERT_NE(nullptr, store_);

  ASSERT_EQ(2, store_->num_buckets());
  ASSERT_THAT(store_->bucket_names(),
              ::testing::UnorderedElementsAre("bucket-0", "bucket-1"));
}

TEST_F(KeyStoreTests, CanInsertRetrieveOne) {
  ASSERT_TRUE(store_->Put("foo", 101));
  ASSERT_EQ(101, *(store_->Get("foo")));
}

TEST_F(KeyStoreTests, CanEraseOne) {
  ASSERT_TRUE(store_->Put("foo", 101));
  ASSERT_EQ(101, *(store_->Get("foo")));
  ASSERT_TRUE(store_->Remove("foo"));
  ASSERT_FALSE(store_->Get("foo"));
}

TEST_F(KeyStoreTests, CannotEraseNotExists) {
  ASSERT_FALSE(store_->Remove("bar"));
}

TEST_F(KeyStoreTests, CanInsertRetrieveMany) {
  auto mapper = [](int num) { return 100 + 2 * num; };
  Insert(1, 99, mapper);
  Assert(1, 99, mapper);
}

TEST_F(KeyStoreTests, CanEraseMany) {
  auto mapper = [](int num) { return 100 + 2 * num; };
  Insert(1, 99, mapper);
  for (int i = 20; i < 30; ++i) {
    ASSERT_TRUE(store_->Remove(std::to_string(i)));
  }
  Assert(1, 19, mapper);
  Assert(30, 99, mapper);
  for (int i = 20; i < 30; ++i) {
    ASSERT_FALSE(store_->Get(std::to_string(i)));
  }
}

TEST_F(KeyStoreTests, CanAddBucket) {
  auto mapper = [](int num) { return 99 - 5 * num; };
  Insert(556, 1023, mapper);

  BucketPtr bp = std::make_shared<Bucket>("bucket-2", std::vector<float>{0.35, 0.70});
  store_->AddBucket(bp);
  Assert(556, 1023, mapper);
}

using KSll = keystore::InMemoryKeyStore<long, long>;
using KSllPtr = std::shared_ptr<KSll>;

/**
 * Sink store, throws away any value stored, but counts the net additions (Puts less Removes).
 * Used to test rebalancing and ensuring no keys are lost.
 */
class SinkDelegate : public keystore::KeyStore<long, long> {
 public:
  SinkDelegate() : KeyStore("test") {};

  unsigned long num_items_ = 0;

  bool Put(const long &key, const long &value) override {
    num_items_++;
    return true;
  }
  [[nodiscard]] optional<long> Get(const long &key) const override {
    return {};
  }
  bool Remove(const long &key) override {
    num_items_--;
    return true;
  }
};


class MultiKeyStoreTests : public ::testing::Test {
 protected:
  std::vector<KSllPtr> stores_;
  std::shared_ptr<View> pv_;
  int numstores_;
  std::shared_ptr<SinkDelegate> sink_;
  std::unordered_map<std::string, KSllPtr > store_lookup_by_bkt_;

  void SetUp() override {
    numstores_ = 3;
    sink_ = std::make_shared<SinkDelegate>();
    pv_ = std::move(make_balanced_view(2 * numstores_, 3));
    for (int i = 0; i < numstores_; ++i) {

      std::string b1 { "bucket-" + std::to_string(2 * i) },
        b2 { "bucket-" + std::to_string(2 * i + 1)};

      auto store = std::make_shared<KSll>(
          "TestStore[" + std::to_string(i) + "]", pv_, std::unordered_set<std::string>{ b1, b2 }
      );
      stores_.push_back(store);
      store_lookup_by_bkt_[b1] = store;
      store_lookup_by_bkt_[b2] = store;
    }
  }

  void Insert(long numKeys) {
    for (int i = 0; i < numKeys; ++i) {
      bool inserted = false;
      for (int j = 0; j < numstores_; ++j) {
        if (stores_[j]->Put(i, 100 + 2 * i)) {
          inserted = true;
          break;
        }
      }
      ASSERT_TRUE(inserted) << "Failed to insert key/value for " << i;
    }
  }

  void AssertAllKeys(long numKeys) {
    for (int i = 0; i < numKeys; ++i) {
      std::optional<long> found;
      for (int j = 0; j < numstores_; ++j) {
        found = stores_[j]->Get(i);
        if (found) {
          ASSERT_EQ(100 + 2 * i, *found) << "Values don't match for i = " << i;
          break;
        }
      }
      ASSERT_TRUE(found) << "Failed to retrieve value for " << i;
    }
  }
};

TEST_F(MultiKeyStoreTests, CanStoreAndRetrieve) {

  long numKeys = 100;
  Insert(numKeys);
  AssertAllKeys(numKeys);
}

int GetTotalCount(const KSll &store) {
  auto stats = store.Stats();
  auto counts = stats["tot_elem_counts"];
  return counts;
}

TEST_F(MultiKeyStoreTests, AddsBucket) {

  const long kTot = 20000;
  Insert(kTot);

  unsigned long tot = 0;
  for (const auto& store : stores_) {
    tot += GetTotalCount(*store);
  }
  ASSERT_EQ(kTot, tot);

  // We now add a bucket to the view, the first KeyStore will be allocated to store the data.
  BucketPtr new_bkt = std::make_shared<Bucket>("bucket-20", std::vector<float>{0.05, 0.58});

  // Before adding to the view, we must find which buckets will need to be rebalanced.
  std::set<BucketPtr> rebalance_bkts;
  for (auto ppt : new_bkt->partition_points()) {
    rebalance_bkts.insert(pv_->FindBucket(ppt));
  }
  ASSERT_EQ(2, rebalance_bkts.size());

  // Given the partition points chosen, we know them to be bucket-0 and -4
  auto pos = rebalance_bkts.begin();
  ASSERT_EQ("bucket-0", (*pos++)->name());
  ASSERT_EQ("bucket-4", (*pos++)->name());
  ASSERT_TRUE(pos == rebalance_bkts.end());

  pv_->Add(new_bkt);
  stores_[0]->AddBucket(new_bkt);

  // If we don't re-balance, we will not be able to find data, as the new buckets' maps will
  // have no data stored (still stored in the ones that were "overlapping" with the new ones).
  for (const auto &source_bkt : rebalance_bkts) {
    auto source_store = store_lookup_by_bkt_[source_bkt->name()];
    bool success = source_store->Rebalance(source_bkt, stores_[0]);
    ASSERT_TRUE(success);
  }
  AssertAllKeys(kTot);
}

TEST_F(MultiKeyStoreTests, RemoveBucket) {

  const long kTot = 20000;
  Insert(kTot);
  auto br = pv_->FindBucket(0.666);
  pv_->Remove(br);

  // Once the bucket is removed from the view, we can find which buckets will take its place
  std::set<KeyStorePtr<long, long>> destinationStores;
  for (auto ppt : br->partition_points()) {
    auto bp = pv_->FindBucket(ppt);
    destinationStores.insert(store_lookup_by_bkt_[bp->name()]);
  }

  store_lookup_by_bkt_[br->name()]->RemoveBucket(br, destinationStores);
  AssertAllKeys(kTot);
}
