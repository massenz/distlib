// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.

#include "View.hpp"
#include "brick.hpp"

#include <algorithm>
#include <cmath>
#include <set>

#include <glog/logging.h>

View::View(int intervals) : num_intervals_(intervals), num_buckets_(0),
                            buckets_(new std::set<const Bucket*>[intervals]) {
  for (int i = 0; i < num_intervals_; ++i) {
    buckets_[i] = std::set<const Bucket*>();
  }
}

void View::add(const Bucket *bucket) {
  if (bucket == nullptr) {
    LOG(FATAL) << "Cannot add a null Bucket to a View";
  }
  for (int i = 0; i < bucket->replicas(); i++) {
    float point = bucket->hash_point(i);
    buckets_[interval(point)].insert(bucket);
  }
  num_buckets_++;
}

void View::remove(const Bucket *bucket) {
  int count = 0;
  for(int i = 0; i < num_intervals_; ++i) {
    count += buckets_[i].erase(bucket);
  }
  // It is possible we were asked to remove a non-existent bucket;
  // in this case, we should not decrement the count.
  if (count > 0)
    num_buckets_--;
}

const Bucket *const View::bucket(const std::string &key) {
  float hash_value = consistent_hash(key);
  int start_from = interval(hash_value);

  // Comparison operator for nearest bucket.
  auto op = [hash_value](const Bucket* b1, const Bucket* b2) -> bool {
    return b1->nearest_hash_point(hash_value).second <
           b2->nearest_hash_point(hash_value).second;
  };

  for (int i = start_from; i < num_intervals_; ++i) {
    auto buckets = buckets_[i];
    if (!buckets.empty()) {
      return *std::min_element(buckets.begin(), buckets.end(), op);
    }
  }
  // If we got here, we need to "eat our tail" as consistent hashing is implemented as a ring:
  for (int i = 0; i < start_from; ++i) {
    auto buckets = buckets_[i];
    if (!buckets.empty()) {
      return *std::min_element(buckets.begin(), buckets.end(), op);
    }
  }

  LOG(ERROR) << "No bucket found: is this an empty view?";
  return nullptr;
}


int View::interval(float x) {
  if (x < 0.0 || x > 1.0) {
    LOG(FATAL) << "x must be in the [0, 1] interval: " << x;
  }
  return int(floor(x * num_intervals_));
}


std::ostream& operator<<(std::ostream& out, const View& view) {
  std::set<const Bucket*> buckets;

  out.setf(std::ios_base::fixed);
  out.precision(6);

  float lowerBound = 0.0f;
  float upperBound = 0.0f;
  float intervalSize = 1.0f / view.num_intervals_;
  for (int i = 0; i < view.num_intervals_; ++i) {
    upperBound += intervalSize;
    if (view.buckets_[i].empty()) continue;
    out << lowerBound << " .. " << upperBound << "\t";
    std::for_each(view.buckets_[i].begin(), view.buckets_[i].end(),
                  [&out, &buckets](const Bucket* b) {
                    out << b->name() << "  ";
                    buckets.insert(b);
                  });
    out << std::endl;
    lowerBound = upperBound;
  }
  out << std::endl;

  for (auto b : buckets) {
    out << *b << std::endl;
  }
  return out;
}
