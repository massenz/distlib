// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.

#include "../include/View.hpp"
#include "../include/ConsistentHash.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

void View::Add(const Bucket *bucket) {
  if (bucket == nullptr) {
    LOG(FATAL) << "Cannot add a null Bucket to a View";
    return;
  }

  for (int i = 0; i < bucket->partitions(); i++) {
    float point = bucket->partition_point(i);
    partition_to_bucket_[point] = bucket;
  }
  num_buckets_++;
}

bool View::Remove(const Bucket *bucket) {
  bool found = false;

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
  // It is possible we were asked to remove a non-existent bucket;
  // in this case, we should not decrement the count.
  if (found) {
    num_buckets_--;
    VLOG(2) << "Removed bucket from View, left " << num_buckets_;
  } else {
    VLOG(2) << "Bucket " << *bucket << " not found, not removed";
  }
  return found;
}

const Bucket *View::FindBucket(float hash) const {

  if (partition_to_bucket_.empty()) {
    return nullptr;
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

  for (auto b : view.buckets()) {
    out << *b << std::endl;
  }
  return out;
}
std::set<const Bucket *> View::buckets() const {
  std::set<const Bucket *> buckets;

  for (auto item : partition_to_bucket_) {
    buckets.insert(item.second);
  }

  return buckets;
}

void View::Clear() {
  for (auto bp : buckets()) {
    delete (bp);
  }
}

/**
 * Creates a new `View` (and associated `num_buckets` `Bucket`s) with each bucket having
 * `partitions_per_bucket` partitions points, equidistant on the unit circle and each bucket
 * overlaps, so that the partition points are uniformly distributed across buckets.
 *
 * **NOTE**: it is the caller's responsibility to deallocate the `Bucket`s created by this method.
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
    auto *pb = new Bucket("bucket-" + std::to_string(i), hash_points[i]);
    pv->Add(pb);
  }

  return pv;
}

