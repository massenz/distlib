// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 8/6/17.

#include "utils.hpp"

namespace utils {

std::ostream& PrintVersion(const std::string& server_name,
                           const std::string& version,
                           std::ostream &out) {
  int major, minor, patch;
  zmq::version(&major, &minor, &patch);

  out << server_name << " Ver. " << version
      << "\n- LibDist ver. " << RELEASE_STR
      << "\n- ZeroMQ ver. " << major << "." << minor << "." << patch
      << "\n- Google Protocol Buffer ver. "
      << ::google::protobuf::internal::VersionString(GOOGLE_PROTOBUF_VERSION) << std::endl;

  return out;
}
} // namespace utils
