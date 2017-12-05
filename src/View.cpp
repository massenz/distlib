// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.

#include "../include/View.hpp"
#include "../include/ConsistentHash.hpp"

#include <algorithm>
#include <cmath>


void View::add(const Bucket *bucket) {
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

void View::remove(const Bucket *bucket) {
  bool found = false;

  for (auto item : partition_to_bucket_) {
    if (item.second == bucket) {
      partition_to_bucket_.erase(item.first);
      found = true;
    }
  }
  // It is possible we were asked to remove a non-existent bucket;
  // in this case, we should not decrement the count.
  if (found) {
    num_buckets_--;
  }
}

const Bucket *const View::bucket(const std::string &key) {

  if (partition_to_bucket_.empty()) {
    return nullptr;
  }

  float hash_value = consistent_hash(key);

  auto pos = partition_to_bucket_.upper_bound(hash_value);

  if (pos == partition_to_bucket_.end()) {
    return partition_to_bucket_.begin()->second;
  } else {
    return pos->second;
  }
}



std::ostream& operator<<(std::ostream& out, const View& view) {
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
