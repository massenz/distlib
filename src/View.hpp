// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.

#pragma once

#include <set>

#include <glog/logging.h>
#include <map>

#include "Bucket.hpp"

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

private:
  int num_buckets_;

  // Maps each bucket's partition point to the respective bucket.
  std::map<float, const Bucket*> partition_to_bucket_;

  friend std::ostream& operator<<(std::ostream&, const View&);
public:
  View() : num_buckets_(0) {};

  virtual ~View() {}

  /** Adds a bucket to this `View` */
  void add(const Bucket *bucket);

  /** Removes the given bucket from this view. */
  void remove(const Bucket *bucket);

  /**
   * @return the total number of buckets available in this view.
   */
  int num_buckets() const { return num_buckets_; }

  /**
   * Retrieves the `Bucket` which is the nearest match for the given `key`.
   *
   * The `key` will be hashed and reduced to the [0, 1] interval (consistent hashing) and then
   * the `Bucket` whose hash-point is the smallest partition point larger than the hash
   * will be found and returned.
   *
   * See the "Consistent Hash" paper for a summary of why this operation is a O(1) and for
   * implementation details.
   */
  const Bucket* const bucket(const std::string& key);

  std::set<const Bucket *> buckets() const;
};

/**
 * Streams a view, listing all the intervals and associated buckets; then emits a list of all
 * the buckets.
 */
std::ostream& operator<<(std::ostream&, const View &);
