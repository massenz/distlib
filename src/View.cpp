// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.

#include "../include/View.hpp"
#include "../include/ConsistentHash.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

void View::Add(const BucketPtr& bucket) {
  if (!bucket) {
    LOG(FATAL) << "Cannot add a null Bucket to a View";
    return;
  }

  {
    UniqueLock lk(buckets_mx_);
    buckets_.insert(bucket);
  }
  UniqueLock lk(partition_map_mx_);
  for (int i = 0; i < bucket->partitions(); i++) {
    float point = bucket->partition_point(i);
    partition_to_bucket_[point] = bucket;
  }
}

bool View::Remove(const BucketPtr& bucket) {
  bool found = false;

  {
    UniqueLock lk(partition_map_mx_);
    for (auto item : bucket->partition_points()) {
      if (partition_to_bucket_[item] == bucket) {
        auto res = partition_to_bucket_.erase(item);
        if (res > 0) {
          found = true;
          VLOG(2) << "Found matching partition point: " << item
                  << ", removed bucket: " << *bucket;
        }
      }
    }
  }
  // It is possible we were asked to remove a non-existent bucket;
  // in this case, we should not decrement the count.
  if (found) {
    UniqueLock lk(buckets_mx_);
    buckets_.erase(bucket);
    VLOG(2) << "Removed bucket from View: " << *bucket;
  } else {
    VLOG(2) << "Bucket " << *bucket << " not found, not removed";
  }
  return found;
}

BucketPtr View::FindBucket(float hash) const {
  if (hash < 0.0f || hash > 1.10000001f) {
    throw std::invalid_argument(
        "Hash should always be in the [0, 1] interval, was: " + std::to_string(hash));
  }

  SharedLock lk(partition_map_mx_);
  if (partition_to_bucket_.empty()) {
    throw std::invalid_argument("No buckets in this View");
  }
  auto pos = partition_to_bucket_.upper_bound(hash);

  if (pos == partition_to_bucket_.end()) {
    return partition_to_bucket_.begin()->second;
  } else {
    return pos->second;
  }
}

std::ostream &operator<<(std::ostream &out, const View &view) {
  out.setf(std::ios_base::fixed);
  out.precision(6);

  for (const auto& b : view.buckets()) {
    out << *b << std::endl;
  }
  return out;
}

std::set<BucketPtr> View::buckets() const {
  SharedLock lk(buckets_mx_);
  return buckets_;
}

void View::Clear() {
  UniqueLock lk(buckets_mx_);
  partition_to_bucket_.clear();
}

View::operator json() const {
  std::vector<Bucket> buckets;
  buckets.reserve(num_buckets());

  {
    UniqueLock lk(buckets_mx_);
    for (const auto& bpt : buckets_) {
      buckets.push_back(*bpt);
    }
  }

  return json{
      {
        "view", {
          {"buckets", buckets}
        }
      }
  };
}

/**
 * Creates a new `View` (and associated `num_buckets` `Bucket`s) with each bucket having
 * `partitions_per_bucket` partitions points, equidistant on the unit circle and each bucket
 * overlaps, so that the partition points are uniformly distributed across buckets.
 *
 * @param num_buckets how many buckets to create and associate to the view
 * @param partitions_per_bucket how many partition points in each bucket
 * @return a `View` fully formed, with the given partition points, buckets
 */
std::unique_ptr<View> make_balanced_view(int num_buckets,
                                         int partitions_per_bucket) {
  if (num_buckets <= 0 || partitions_per_bucket <= 0) {
    throw std::invalid_argument("num_buckets and partitions_per_bucket must both be non-zero");
  }
  auto pv = std::make_unique<View>();

  std::vector<std::vector<float>> hash_points{static_cast<unsigned long>(num_buckets)};
  for (int i = 0; i < num_buckets; ++i) {
    hash_points[i] = std::vector<float>(partitions_per_bucket);
  }

  float delta = 1.0f / (num_buckets * partitions_per_bucket);
  float x = delta;

  for (int j = 0; j < partitions_per_bucket; ++j) {
    for (int i = 0; i < num_buckets; ++i) {
      hash_points[i][j] = x;
      x += delta;
    }
  }

  for (int i = 0; i < num_buckets; ++i) {
    auto pb = std::make_shared<Bucket>("bucket-" + std::to_string(i), hash_points[i]);
    pv->Add(pb);
  }

  return pv;
}

