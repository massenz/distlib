// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.

#pragma once

#include <set>

#include <glog/logging.h>
#include <map>

#include "ConsistentHash.hpp"
#include "Bucket.hpp"


/**
 * A `map` which compares its `float` keys with a given `Tolerance`.
 *
 * @see FloatLessWithTolerance
 */
typedef std::map<float, const Bucket*, FloatLessWithTolerance<>> MapWithTolerance;

/**
 * A `View` is a mapping of the whole space of hashes onto a set of `Bucket`s, using
 * consistent hashing.
 *
 * The purpose of a `View` is to retrieve the `Bucket` which is closest to the given item's
 * hash: we do this in an efficient way by splitting the unity interval into a set of equal-sized
 * N intervals, where N is the smallest 2^x such that N > C log(C) and C is the number of expected
 * caches.
 *
 * To each `interval` we assign the buckets where at least one of their `hash_points` falls into
 * the interval: when finding the closest (ascending) bucket we only need to look into this
 * smaller subset of buckets (see the {@link bucket(const std::string&)} method).
 *
 * If no buckets have been allocated yet to that interval, we continue navigating the unit circle
 * until we find one.
 *
 * For more details, see the paper on Consistent Hashing, referred to in the documentation for
 * the `consistent_hash()` method.
 */
class View {

  int num_buckets_ {0};

  /**
   * Maps each bucket's partition point to the respective bucket.
   */
   MapWithTolerance partition_to_bucket_;

  /**
   * Streams a view, listing all the intervals and associated buckets; then emits a list of all
   * the buckets.
   */
  friend std::ostream& operator<<(std::ostream&, const View&);

public:
  View() = default;
  virtual ~View() = default;

  View(const View&) = delete;
  View(View&&) = delete;

  /** Adds a bucket to this `View` */
  void Add(const Bucket *bucket);

  /**
   * Removes the given bucket from this view.
   *
   * Note that this also deletes the associated pointer: if you need to access
   * the `bucket` after it has been removed, make a copy.
   *
   * @param bucket the bucket to remove from this view and delete
   */
  bool Remove(const Bucket *bucket);

  /**
   * @return the total number of buckets available in this view.
   */
  int num_buckets() const { return num_buckets_; }

  /**
   * Removes all buckets and deallocates the associated pointers.
   *
   * The `~View` destructor does not delete the buckets' pointers (as the caller may
   * still hold references to them); if you want to clear all the memory, use this method.
   */
  void Clear();

  /**
   * Retrieves the `Bucket` which the `hash` belongs to.
   *
   * The `hash` must be in the [0, 1] interval (consistent hashing) and then
   * the `Bucket` whose hash-point is the smallest partition point larger than the `hash`
   * will be found and returned.
   *
   * See the "Consistent Hash" paper for a summary of why this operation is a O(1) and for
   * implementation details.
   *
   * @param hash the hash value for a key, whose `Bucket` we wish to lookup
   * @return a pointer to the `Bucket` which contains the value associated with the `hash`
   */
  const Bucket* FindBucket(float hash) const;

  std::set<const Bucket *> buckets() const;
};

std::unique_ptr<View> make_balanced_view(int num_buckets, int partitions_per_bucket = 5);
