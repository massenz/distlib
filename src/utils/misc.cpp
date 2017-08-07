// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 8/6/17.


#include <zmq.hpp>
#include <iostream>
#include <config.h>
#include <google/protobuf/stubs/common.h>


namespace utils {

// TODO: replace with a function that returns the version string; and one that emits to an ostream.
void printVersion() {
  int major, minor, patch;

  zmq::version(&major, &minor, &patch);
  char zmq_ver[32];
  sprintf(zmq_ver, "%d.%d.%d", major, minor, patch);

  std::cout << "Demo SWIM server Ver. " << RELEASE_STR
            << "\n- ZeroMQ ver. " << zmq_ver << "\n- Google Protocol Buffer ver. "
            << ::google::protobuf::internal::VersionString(GOOGLE_PROTOBUF_VERSION) << std::endl;
}

} // namespace utils
