// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.

#pragma once

#include <cmath>
#include <functional>
#include <string>
#include <utility>

#include <openssl/md5.h>

#include "utils/utils.hpp"


/**
 * Computes a "consistent hash" of the given string.
 *
 * For the definition of "consistent hash" see:
 * Karger et al., "Consistent Hashing and Random Trees"
 * https://goo.gl/NBY9yN
 *
 * @param msg the string to hash
 * @return a value in the [0, 1.0) range that can be used as `msg`'s
 *    consistent hash.
 */
float consistent_hash(const std::string &msg);


/**
 * Comparator function object, compares two floats, assuming
 * they are equal if their values are within `eps` distance.
 *
 * See Item 40 of Effective STL.
 */
template <int Tolerance = 5>
class FloatLessWithTolerance :
    public std::binary_function<float, float, bool> {

  double epsilon_;
 public:
  FloatLessWithTolerance() {
    epsilon_ = pow(10, -Tolerance);
  }

  bool operator()( const float &left, const float &right  ) const {
    return (std::abs(left - right) > epsilon_) && (left < right);
  }
};
