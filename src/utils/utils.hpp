// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 9/17/17.

#pragma once

#include <iostream>
#include <regex>
#include <vector>

#include <google/protobuf/stubs/common.h>

#include <zmq.hpp>

#include "version.h"


namespace utils {

/**
 * Marker exception for unimplemented methods.
 */
class not_implemented : public std::exception {
  std::string what_;

public:
  explicit not_implemented(const std::string &method_or_class) {
    what_ = method_or_class + " not implemented";
  }

  const char *what() const throw() override {
    return what_.c_str();
  }
};

/**
 * Prints the current project's version, along with those of supporting frameworks (ZMQ and
 * Protobuf).
 *
 * @param out the stream to write out the version information
 * @return the same stream that was passed in, for ease of chaining
 */
std::ostream& PrintVersion(std::ostream &out = std::cout);

/**
 * Converts a vector to a string, the concatenation of its element, separated by `sep`.
 * Only really useful for debugging purposes.
 *
 * @tparam T the type of the elements, MUST support emitting to an `ostream` via the `<<` operator
 * @param vec the vector whose element we are emitting
 * @param sep a separator string, by default a newline
 * @return the string resulting from the concatenation of all elements
 */
template<typename T>
std::string Vec2Str(const std::vector<T> &vec, const std::string &sep = "\n") {
  bool first = true;
  std::ostringstream out;

  for (auto t : vec) {
    if (!first) out << sep; else first = false;
    out << t;
  }
  return out.str();
}
} // namespace utils
