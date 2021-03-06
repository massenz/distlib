// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.

#pragma once


#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <glog/logging.h>

#include "json.hpp"

using json = nlohmann::json;

/**
 * A "bucket" abstracts the concept of a hashed partition, using consistent hashing.
 *
 * <p>Each Bucket, further, owns a number of `partitions` that are essentially points on the unity
 * circle and which determine which items will be allocated to this Bucket (respective to
 * other buckets, if any).
 *
 * <p>A Bucket holds no information about the items that are (nominally) assigned to it,
 * however, it has a `#name_` that can be used as a unique ID (or, e.g., a hostname) when
 * distributing items to a set of nodes; the only information the bucket holds is about the number
 * of its `#partitions_` and their position on the unit circle.
 *
 * <p>The `partition_points()` are assumed (but not guaranteed) to be roughly equally (and randomly,
 * but deterministically, according to a hashing function) spaced around the unit
 * circle, thus providing an approximately uniform and constant time-complexity for lookups (more
 * specifically, the lookup complexity has been demonstrated to be O(log(C)), where C is the
 * number of caches, or Buckets, used).
 *
 * <p>For more details, see the [Consistent Hashing](http://www.cs.princeton.edu/courses/archive/fall07/cos518/papers/chash.pdf)
 * paper.
 */
class Bucket {
private:
  std::string name_;
  std::vector<float> hash_points_;

public:
  Bucket(std::string name, std::vector<float> hash_points);

  Bucket(const Bucket&) = default;
  Bucket& operator=(const Bucket& other) = default;
  virtual ~Bucket() = default;

  void add_partition_point(float point);

  void remove_partition_point(unsigned int i);

  /**
 * Converts a `Bucket` to JSON.
 *
 * @param bp the `Bucket` to render in JSON
 * @return the JSON representation of the `Bucket` pointed to by `bp`
 */
  operator json() const;

  /**
   * Every `Bucket` should have a unique name that can be used to identify it.
   *
   * @return the bucket's name
   */
  [[nodiscard]] std::string name() const {
    return name_;
  }

  void set_name(std::string name) {
    name_ = std::move(name);
  }

  /**
   * The partition points for this bucket will determine which items will be "allocated" to it,
   * based on a "nearest point" from the item's hash.
   *
   * @return the set of {@link partitions()} points that define this bucket
   */
  std::vector<float> partition_points() const {
    return hash_points_;
  }

  float partition_point(int i) const {
    if (i < 0 || i >= partitions()) {
      std::ostringstream msg;
      msg << "Out of bound: requesting partition point #" << i << ", when only "
           << partitions() << " are available ('" << name_ << "')";
      throw std::out_of_range(msg.str());
    }
    return hash_points_[i];
  }

  /**
   * Given a point on the unit circle, we return the {@link #partition_point()} that is the
   * next-greatest in the ``Bucket``.
   *
   * This is consistent with the usage in Consistent Hashing, where we partition the unit
   * circle and then allocate each partition with all the points that are _smaller_ than
   * the partition point.
   *
   * @param x a point in the [0, 1] interval.
   * @return a pair of {index, point} values that determine which partition point is
   *    the immediately greater than `x`.
   */
  std::pair<int, float> partition_point(float x) const;

  int partitions() const { return hash_points_.size(); }
};


/**
 * Utility function to stream a string representation of a bucket.
 *
 * @return the passed in stream, to which the bucket has been streamed to.
 */
std::ostream& operator<<(std::ostream& out, const Bucket &bucket);
