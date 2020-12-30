// Copyright (c) 2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com)

#pragma once

#include <iomanip>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <glog/logging.h>

#include "json.hpp"

#include "ConsistentHash.hpp"
#include "View.hpp"


using json = nlohmann::json;

namespace keystore {

template<typename K>
class not_found : public utils::base_error {
 public:
  explicit not_found(const K &key) : base_error("") {
    std::ostringstream msg;
    msg << "Key not found: " << key;
    what_ = msg.str();
  }
};

// Forward declaration
template<typename Key, typename Value>
class KeyStore;

/**
 * Data store, uses ``unordered_map`` so that access to values is O(1)
 */
template<typename Key, typename Value>
using MapPtr = std::shared_ptr<std::unordered_map<Key, Value>>;

template<typename Key, typename Value>
using KeyStorePtr = std::shared_ptr<KeyStore<Key, Value>>;

inline const long kModulo = 33457;

/**
 * Abstract interface for a distributed KeyValue Store.
 *
 * @tparam K the type of the key, must be possible to hash it to a float in [0,1.0] space using
 *      one of the `HashKey` functions variant.
 * @tparam V the type of the data being stored, must provide default and copy constructors, so
 *      that it can be stored in an associative (unordered) container.
 */
template<typename K, typename V>
class KeyStore {
 protected:
  std::string self_;

  /**
   * Given a `key` it hashes it, finds the appropriate `Bucket` and returns the corresponding
   * associative container which may contain the data.
   *
   * @param key
   * @return a pointer to the map where the `data` *may* be stored; or `None` if the key hashes
   *        to a bucket that does not belong to this store
   */
  virtual std::optional<MapPtr<K, V>> FindMap(const K &key) const = 0;

 public:

  virtual ~KeyStore() = default;

  [[nodiscard]] std::string name() const { return self_; }
  void set_name(std::string name) { self_ = std::move(name); }

  /**
   * Stores the `value` in the bucket which the `key` maps to (assuming the bucket is one of
   * those which this store has been tasked with).
   *
   * @param key
   * @param value
   * @return `true` if successful; `false`otherwise (most notably, if the key maps to a bucket
   * that does not belong to the store)
   */
  virtual bool Put(const K &key, const V &value) = 0;

  /**
   * Retrieves the data mapped by the `key`
   *
   * @param key
   * @return an `optional` which may not contain the data, if the key maps to a bucket not
   * in this store; or if simply the key does not map to one of those already stored.
   */
  virtual std::optional<V> Get(const K &key) const = 0;

  /**
   * Removes the key/value from the store.
   *
   * @param key the key to remove, along with its associated value
   * @return `true` if the key was found and removed
   */
  virtual bool Remove(const K &key) = 0;

  [[nodiscard]] virtual json Stats() const {
    json stats;
    stats["name"] = name();
    return stats;
  }
};

template<typename Key, typename Value>
class PartitionedKeyStore : public KeyStore<Key, Value> {
 public:
  /**
   * Adds a bucket to this store: going forward this store will save data for this bucket too.
   *
   * <p>Generally, this also requires a
   * <a href="https://bitbucket.org/marco/distkvs/src/develop/README.md#rebalancing-nodes">
   * rebalance</a>, see the `Rebalance()` method.
   *
   * @param bucket that will be added to the set of buckets for this store
   * @return whether adding the bucket was successful
   * @see   Rebalance(BucketPtr source, KeyStorePtr<K, V> destination_store)
   */
  virtual void AddBucket(BucketPtr bucket) = 0;

  /**
   * Removes a bucket from this store.
   *
   * <p>This will require moving the data, if any, to another store: this is **not** a
   * <a href="https://bitbucket.org/marco/distkvs/src/develop/README.md#rebalancing-nodes">
   * rebalance</a>, but simply a data move.
   *
   * @param bucket that will be removed
   * @param destination_store the store where to move the data to
   * @return whether the data move was successful
   */
  virtual bool RemoveBucket(BucketPtr bucket,
      std::set<KeyStorePtr<Key, Value>> destination_stores) = 0;

  /**
   * Following a bucket addition, the data needs to be moved from the `source` bucket to the
   * `destination` one (owned, by the `destination_store`).
   *
   * @param source the bucket whose data will be "sequentially scanned" and moved (if
   * appropriate) to the destination store
   * @param destination_store the destination for the moved data (it either owns the new bucket
   * (s), or knows how to reach them)
   * @return `true` if the re-balancing was successful
   */
  virtual bool Rebalance(BucketPtr source, KeyStorePtr<Key, Value> destination_store) = 0;
};

template<typename K, typename V>
void PrintStats(const KeyStore<K, V> &store, std::ostream& out = std::cout) {
  auto stats = store.Stats();

  out << "============== Stats: " << store.name() << " ==================" << std::endl;
  out << std::setw(4) << stats << std::endl;
  out << "----------------------------------------------------------------" << std::endl;
}

} // namespace keystore
