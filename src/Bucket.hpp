// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.



#ifndef BRICK_BUCKET_HPP
#define BRICK_BUCKET_HPP


#include <memory>
#include <string>
#include <vector>

#include <glog/logging.h>

/**
 * A "bucket" abstracts the concept of a hashed partition, using consistent hashing.
 *
 * Each Bucket, further, owns a number of `partitions` that are essentially points on the unity
 * circle and which determine which items will be allocated to this Bucket (respective to
 * other buckets, if any).
 *
 * A Bucket holds no information about the items that (nominally) assigned to it, however, it has
 * a `name` that can be used as a unique ID (or, e.g., a hostname) when distributing items to a
 * set of nodes; the only information the bucket holds is around the number of its `partitions`
 * and their position on the unit circle.
 *
 * The `partition_points` are assumed (but not guaranteed) to be roughly equally (and randomly,
 * but deterministically - once the {@link hashing_function()} is given) spaced around the unit
 * circle, thus providing an approximately uniform and constant time-complexity for lookups (more
 * specifically, the lookup complexity has been demonstrated to be O(log(C)), where C is the
 * number of caches, or Buckets, used).
 *
 * For more details, see the `Consistent Hashing` paper.
 */
class Bucket {
private:
  const std::string name_;
  std::vector<float> hash_points_;
  int partitions_;

public:
  Bucket(const std::string &name, unsigned int replicas);

  virtual ~Bucket() {}

  // Explicitly disallow copy & assignment
  Bucket(const Bucket&) = delete;
  Bucket operator=(const Bucket&) = delete;


  const std::string name() const {
    return name_;
  }


  /**
   * The partition points for this bucket will determine which items will be "allocated" to it,
   * based on a "nearest point" from the item's hash.
   *
   * @return the set of {@link partitions()} points that define this bucket
   */
  const std::shared_ptr<std::vector<float>> partition_points() const {
    // Make a copy, do not return the original vector.
    std::shared_ptr<std::vector<float>> ret(new std::vector<float>());

    for (auto x : hash_points_) {
      ret->push_back(x);
    }
    return ret;
  }

  float partition_point(int i) const {
    if (i >= partitions_) {
      LOG(FATAL) << "Out of bound: requesting partition point #" << i << ", when only "
                 << partitions_ << " are available ('" << name_ << "')";
    }
    return hash_points_[i];
  }

  /**
   * Given a point on the unit circle, we return the closest {@link partition_point(int)} as well
   * as its index.
   *
   * @param x a point in the [0, 1] interval.
   * @return a pair of {index, point} values that determine which partition point is nearest to `x`.
   */
  std::pair<int, float> nearest_partition_point(float x) const;

  int partitions() const { return partitions_; }
};


/**
 * Utility function to stream a string representation of a bucket.
 *
 * @return the passed in stream, to which the bucket has been streamed to.
 */
std::ostream& operator<<(std::ostream& out, const Bucket &bucket);

#endif //BRICK_BUCKET_HPP
