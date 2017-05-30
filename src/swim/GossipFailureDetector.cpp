// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.

#include "GossipFailureDetector.hpp"

namespace swim {

bool operator<(const ServerRecord &lhs, const ServerRecord& rhs) {
  if (lhs.server().hostname() == rhs.server().hostname()) {
    return lhs.server().port() < rhs.server().port();
  }
  return lhs.server().hostname() < rhs.server().hostname();
}

bool operator==(const Server &lhs, const Server &rhs) {
  return lhs.hostname() == rhs.hostname()
      && lhs.port() == rhs.port();
}

SwimReport GossipFailureDetector::PrepareReport() {
  SwimReport report;

  report.mutable_sender()->CopyFrom(gossip_server().self());

  for (auto item : gossip_server().alive()) {
    if (!item->didgossip()) {
      ServerRecord *prec = report.mutable_alive()->Add();
      prec->CopyFrom(*item);
    }
  }

  for (auto item : gossip_server().suspected()) {
    if (!item->didgossip()) {
      ServerRecord *prec = report.mutable_suspected()->Add();
      prec->CopyFrom(*item);
    }
  }

  return report;
}
} // namespace swim



