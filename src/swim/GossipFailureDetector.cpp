// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.

#include "GossipFailureDetector.hpp"

namespace swim {

bool operator<(const ServerRecord &lhs, const ServerRecord& rhs) {
  if (lhs.server().ip_addr() == rhs.server().ip_addr()) {
    return lhs.server().port() < rhs.server().port();
  }
  return lhs.server().ip_addr() < rhs.server().ip_addr();
}

} // namespace swim



