// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.

#pragma once

#include <set>

#include <glog/logging.h>

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
 * For more details, see the paper on Consistent Hashing, referred to in the documentation for
 * the `consistent_hash()` method.
 */
class View {

private:
  int num_intervals_;
  int num_buckets_;

  std::unique_ptr<std::set<const Bucket*> []> buckets_;

  /**
   * The interval number corresponding to x (which must be in the [0, 1] range).
   *
   * @param the point in the unity range whose interval we are seeking.
   * @return the value between [0, num_intervals-1] for the corresponding interval.
   */
  int interval(float x);

  friend std::ostream& operator<<(std::ostream&, const View&);
public:
  View(int intervals);

  virtual ~View() {}

  /** Adds a bucket to this `View` */
  void add(const Bucket *bucket);

  /** Removes the given bucket from this view. */
  void remove(const Bucket *bucket);
  /**
   * @return the total number of buckets available in this view.
   */
  int num_buckets() const { return num_buckets_; }

  /** Used for testing only, it's an implementation detail and should not be used externally */
  int intervals() const { return num_intervals_; }

  /**
   * Retrieves the `Bucket` which is the nearest match for the given `key`.
   *
   * The `key` will be hashed and reduced to the [0, 1] interval (consistent hashing) and then
   * the `Bucket` whose hash-point is nearest (largest) in the interval will be found and returned.
   *
   * See the "Consistent Hash" paper for a summary of why this operation is a O(1) and for
   * implementation details.
   */
  const Bucket* const bucket(const std::string& key);
};

/**
 * Streams a view, listing all the intervals and associated buckets; then emits a list of all
 * the buckets.
 */
std::ostream& operator<<(std::ostream&, const View &);
