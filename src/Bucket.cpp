// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.


#include "Bucket.hpp"

#include <algorithm>

#include <cmath>
#include <ios>

#include "brick.hpp"


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


Bucket::Bucket(const std::string &name, unsigned int replicas) :
    name_(name), partitions_(replicas), hash_points_(replicas) {
  for (int i = 0; i < partitions_; ++i) {
    hash_points_[i] = consistent_hash(name + "_" + std::to_string(i));
  }
  std::sort(hash_points_.begin(), hash_points_.end());
}


std::pair<int, float> Bucket::nearest_partition_point(float x) const {
  auto pos = std::upper_bound(hash_points_.begin(), hash_points_.end(), x);
  if (pos == hash_points_.end()) {
    return std::make_pair(0, hash_points_[0]);
  }
  return std::make_pair(std::distance(hash_points_.cbegin(), pos), *pos);
}
