// Copyright (c) 2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com)

#pragma once

#include "KeyStore.hpp"

namespace keystore {

template<typename T>
inline float HashKey(const T &key) {
  std::string key_str = std::string{key};
  return consistent_hash(key_str);
}

inline float HashKey(const char *key) {
  unsigned long iter = strlen(key) / sizeof(long);
  unsigned long remainder = strlen(key) % sizeof(long);

  long n = 0;
  long accum = 0;
  for (int i = 0; i < iter; ++i) {
    memcpy(&n, key + sizeof(long) * i, sizeof(long));
    accum += n;
  }
  if (remainder > 0) {
    memcpy(&n, key + sizeof(long) * iter, remainder);
    accum += n;
  }
  float f = static_cast<float >(accum % kModulo) / kModulo;
  VLOG(3) << key << " hashes to " << f;

  return f;
}

template<>
inline float HashKey(const long &key) {
  return static_cast<float >(key % kModulo) / kModulo;
}

template<>
inline float HashKey<int>(const int &key) {
  return static_cast<float >(key % kModulo) / kModulo;
}

template<>
inline float HashKey(const std::string &key) {
  return consistent_hash(key);
}

/**
 * Implements a distributed KeyValue Store.
 *
 * <p>Using Consistent Hashes, it distributes the data according to a hash of the key (`K`)
 * using `HashKey(key)`, across a set of `Bucket`s; the data is kept in unordered associative
 * containers, so that access is O(1).
 *
 * <p>Each `InMemoryKeyStore` retains a full "global" `View` of the system, as well as its own set of
 * `Bucket`s (`buckets_`) which map the stored data.
 *
 * <p>The store's API is extremely simple, implementing essentially the CRUD primitives (`Get`,
 * `Put` and `Remove`); however, due to each `InMemoryKeyStore` only being responsible for a portion of
 * the data, we return an `optional<V>` instead of the actual value, as there may actually be no
 * corresponding data, even though the key may be valid (but hashing to a bucket that is not one
 * of those the store is storing data for).
 *
 * @tparam K the type of the key, must be possible to hash it to a float in [0,1.0] space using
 *      one of the `HashKey` functions variant.
 * @tparam V the type of the data being stored, must provide default and copy constructors, so
 *      that it can be stored in an associative (unordered) container.
 */
template<typename K, typename V>
class InMemoryKeyStore : public PartitionedKeyStore<K, V> {

  std::shared_ptr<View> view_ptr_;
  std::unordered_map<BucketPtr, MapPtr<K, V>> maps_;
  std::unordered_set<BucketPtr> buckets_;

 protected:
  std::optional<MapPtr<K, V>> FindMap(const K &key) const override;

 public:

  /**
   * Creates a new store, which maps the keys to the buckets as defined in the `View`, and stores
   * the data only for the subset of `buckets` (in the `View` whose names match).
   *
   * @param view describes how the data, globally, is distributed across buckets; this store may
   * only store a portion of this data (that relative to the `buckets` assigned to it) but it
   * needs to know how the rest of the data is distributed.
   *
   * @param buckets the subset (or, possibly, the entirety) of the data that this store is
   * responsible for storing: this is described by the name of the buckets (in the `view`) that
   * are allocated to this store, matched by `name`.
   */
  InMemoryKeyStore(const std::string& name, const std::shared_ptr<View> &view,
                   const std::unordered_set<std::string> &buckets);

  virtual ~InMemoryKeyStore() = default;

  // ============= The Key/Value Store interface ===================
  bool Put(const K &key, const V &value) override;
  std::optional<V> Get(const K &key) const override;
  bool Remove(const K &key) override;

  // ============= Getters and Setters =============================
  const View *view() const { return view_ptr_.get(); }

  const std::unordered_set<BucketPtr> &buckets() const { return buckets_; }
  int num_buckets() const { return buckets_.size(); }
  std::vector<std::string> bucket_names() const;

  // ============= Class methods & Utilities =======================

  void AddBucket(BucketPtr bucket) override {
    VLOG(2) << "Adding bucket " << bucket << ", to KeyStore " << this->name();
    buckets_.insert(bucket);
    VLOG(2) << "Adding data store for bucket " << bucket;
    maps_[bucket] = std::make_shared<std::unordered_map<K, V>>();
  }

  bool RemoveBucket(BucketPtr bucket,
                    std::set<KeyStorePtr<K, V>> destination_stores) override;

  bool Rebalance(BucketPtr source, KeyStorePtr<K, V> destination_store) override;

  /**
   * Provides metrics for this KVS
   *
   * @return a map of metrics
   */
  json Stats() const override ;

};

template<typename K, typename V>
InMemoryKeyStore<K, V>::InMemoryKeyStore(
    const std::string& name,
    const std::shared_ptr<View> &view,
    const std::unordered_set<std::string> &buckets
) : PartitionedKeyStore<K, V>(name) {
  VLOG(2) << "Creating InMemoryKeyStore with "
          << buckets.size() << " buckets (of " << view->num_buckets() << ")";

  view_ptr_ = view;
  for (auto &b : view_ptr_->buckets()) {
    if (buckets.count(b->name()) > 0) {
      buckets_.insert(b);
      VLOG(2) << "Adding data store for bucket " << b;
      maps_[b] = std::make_shared<std::unordered_map<K, V>>();
    }
  }
}

template<typename K, typename V>
bool InMemoryKeyStore<K, V>::Put(const K &key, const V &value) {
  auto map_maybe = FindMap(key);
  if (map_maybe) {
    auto map = *map_maybe;
    (*map)[key] = value;
    return true;
  }
  return false;
}

template<typename K, typename V>
std::optional<V> InMemoryKeyStore<K, V>::Get(const K &key) const {
  auto map_maybe = FindMap(key);
  if (map_maybe) {
    auto map = *map_maybe;
    if (map->count(key) > 0) {
      return map->at(key);
    }
  }
  return {};
}

template<typename K, typename V>
std::optional<MapPtr<K, V>> InMemoryKeyStore<K, V>::FindMap(const K &key) const {
  float hash = HashKey(key);
  auto bp = view_ptr_->FindBucket(hash);
  if (maps_.count(bp) > 0) {
    return maps_.at(bp);
  }
  return {};
}

template<typename K, typename V>
bool InMemoryKeyStore<K, V>::Remove(const K &key) {
  auto map_maybe = FindMap(key);
  if (map_maybe) {
    return (*map_maybe)->erase(key) > 0;
  }
  return false;
}

template<typename K, typename V>
std::vector<std::string> InMemoryKeyStore<K, V>::bucket_names() const {
  std::vector<std::string> names;
  for (const auto &b : buckets_) {
    names.push_back(b->name());
  }
  std::sort(names.begin(), names.end());
  return names;
}


template<typename K, typename V>
json InMemoryKeyStore<K, V>::Stats() const {

  auto stats = KeyStore<K,V>::Stats();

  unsigned long tot_keys = 0;
  std::vector<json> bj;
  for (const auto& bp : buckets()) {
    json j = *bp;
    long size = maps_.find(bp)->second->size();
    tot_keys += size;
    j["size"] = size;
    bj.push_back(j);
  }
  stats["buckets"] = bj;
  stats["num_buckets"] = num_buckets();
  stats["tot_elem_counts"] = tot_keys;

  return stats;
}

template<typename K, typename V>
bool InMemoryKeyStore<K, V>::Rebalance(BucketPtr source, KeyStorePtr<K, V> destination_store) {
  // This method is typically called after one (or more) bucket(s) have been added to the View,
  // and the data needs to moved out (via a full data scan) from the "old" bucket(s) and into the
  // new bucket(s), according to where the hash points to.
  //
  // This obviously assumes the View has already been updated, and the `destination_store` either
  // owns the destination buckets, or knows how to redirect to the respective store.
  if (buckets_.count(source) == 0) {
    LOG(ERROR) << "Rebalance request for source bucket " << source->name()
               << " cannot be executed by this KeyStore, as it does not own the data";
    return false;
  }

  // We want this to fail noisily if the data cannot be found, so we use at()
  auto data = maps_.at(source);
  std::vector<K> to_be_erased;
  for (const auto &[key, value] : *data) {
    // First find out whether it should be moved at all:
    if (source != view_ptr_->FindBucket(HashKey(key))) {
      if (!destination_store->Put(key, value)) {
        LOG(ERROR) << "Key " << key << " cannot be stored to destination KeyStore ["
                   << destination_store->name() << "]";
        return false;
      }
      to_be_erased.push_back(key);
    }
  }
  // Data cannot be removed from a collection while iterating on it
  // so we do it once the iteration is completed.
  for (const auto &key : to_be_erased) {
    VLOG(3) << "Removing data for key: " << key;
    data->erase(key);
  }
  VLOG(2) << "Done re-balancing from Bucket [" << source->name() << "] to KeyStore ["
          << destination_store->name() << "]";

  return true;
}

template<typename K, typename V>
bool InMemoryKeyStore<K, V>::RemoveBucket(
    BucketPtr bucket,
    std::set<KeyStorePtr<K, V>> destination_stores) {
  VLOG(2) << "Scanning data for bucket " << bucket->name();
  // We want this to fail noisily if the data cannot be found, so we use at()
  auto data = maps_.at(bucket);
  for (const auto &[key, value] : *data) {
    for (const auto &store : destination_stores) {
      if (store->Put(key, value)) {
        break;
      }
      LOG(ERROR) << "Key " << key << " cannot be moved to any of the destinations";
      return false;
    }
  }
  maps_.erase(bucket);
  buckets_.erase(bucket);
  VLOG(2) << "Done moving data from Bucket " << bucket->name();
  return true;
}

} // namespace keystore
