// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.


#include "Bucket.hpp"

#include <cmath>
#include <ios>

#include "brick.hpp"


std::ostream& operator<<(std::ostream& out, const Bucket &bucket) {
  out << "'" << bucket.name() << "' [";
  out.setf(std::ios_base::fixed);
  out.precision(5);

  for (int i = 0; i < bucket.replicas(); ++i) {
    if (i > 0) out << ", ";
    out << bucket.hash_point(i);
  }
  out << "]";

  return out;
}


Bucket::Bucket(const std::string &name, int replicas) :
    name_(name), replicas_(replicas), hash_points_(new float[replicas]) {
  for (int i = 0; i < replicas_; ++i) {
    hash_points_[i] = consistent_hash(name + "_" + std::to_string(i));
  }
}

std::pair<int, float> Bucket::nearest_hash_point(float x) const {
  double min = 1.0;
  int min_idx = -1;
  for (int j = 0; j < replicas(); ++j) {
    double dist = fabs(x - hash_point(j));
    if (x < hash_point(j) && dist < min) {
      min = dist;
      min_idx = j;
    }
  }
  return std::make_pair(min_idx, min);
}
