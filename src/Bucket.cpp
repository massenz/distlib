// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.


#include "Bucket.hpp"

#include <algorithm>
#include <ios>
#include <utility>


std::ostream& operator<<(std::ostream& out, const Bucket &bucket) {
  out << "'" << bucket.name() << "' [";
  out.setf(std::ios_base::fixed);
  out.precision(5);

  for (int i = 0; i < bucket.partitions(); ++i) {
    if (i > 0) out << ", ";
    out << bucket.partition_point(i);
  }
  out << "]";

  return out;
}

Bucket::Bucket(std::string name, std::vector<float> hash_points) :
  name_(std::move(name)), hash_points_(std::move(hash_points)) {
    std::sort(hash_points_.begin(), hash_points_.end());
}

std::pair<int, float> Bucket::partition_point(float x) const {
  auto pos = std::upper_bound(hash_points_.begin(), hash_points_.end(), x);
  if (pos == hash_points_.end()) {
    return std::make_pair(0, hash_points_[0]);
  }
  return std::make_pair(std::distance(hash_points_.cbegin(), pos), *pos);
}

void Bucket::add_partition_point(float point) {
  auto pos = hash_points_.begin();
  for(auto x : hash_points_) {
    if (x > point) {
      break;
    }
    pos++;
  }
  if (pos != hash_points_.end()) {
    hash_points_.insert(pos, point);
  } else {
    hash_points_.push_back(point);
  }
}
void Bucket::remove_partition_point(unsigned int i) {
  if (i < partitions()) {
    hash_points_.erase(hash_points_.cbegin() + i);
  }
}
