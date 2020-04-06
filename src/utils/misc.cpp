// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 8/6/17.

#include <utility>

#include "utils/utils.hpp"

namespace utils {

std::ostream& PrintVersion(const std::string& server_name,
                           const std::string& version,
                           std::ostream &out) {
  out << server_name << " Ver. " << version
      << " (libdist ver. " << RELEASE_STR << ")"
      << std::endl;

  return out;
}

} // namespace utils
