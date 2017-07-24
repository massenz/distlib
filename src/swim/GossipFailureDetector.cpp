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

SwimReport GossipFailureDetector::PrepareReport() const {
  SwimReport report;

  report.mutable_sender()->CopyFrom(gossip_server().self());

  for (const auto &item : gossip_server().alive()) {
    if (!item->didgossip()) {
      ServerRecord *prec = report.mutable_alive()->Add();
      prec->CopyFrom(*item);
    }
  }

  for (const auto &item : gossip_server().suspected()) {
    if (!item->didgossip()) {
      ServerRecord *prec = report.mutable_suspected()->Add();
      prec->CopyFrom(*item);
    }
  }

  return report;
}

std::default_random_engine GossipFailureDetector::random_engine_{};

void GossipFailureDetector::InitAllBackgroundThreads() {

  if (!gossip_server().isRunning()) {
    LOG(ERROR) << "SWIM Gossip Server is not running, please start() it before running the "
                  "detector's background threads";
  }

  std::thread pings([this]() {
    while (gossip_server().isRunning()) {
      PingNeighbor();
      std::this_thread::sleep_for(ping_interval());
    }
  });
  pings.detach();

  std::thread report([this]() {
    while (gossip_server().isRunning()) {
      SendReport();
      std::this_thread::sleep_for(update_round_interval());
    }
  });
  report.detach();

  std::thread gc([this]() {
    while (gossip_server().isRunning()) {
      GarbageCollectSuspected();
      std::this_thread::sleep_for(ping_interval());
    }
  });
  gc.detach();

  LOG(INFO) << "All background threads for the SWIM Detector started";
}

void GossipFailureDetector::PingNeighbor() const {
  const ServerRecordsSet& neighbors = alive();
  if (!neighbors.empty()) {
    VLOG(2) << "Selecting from " << neighbors.size() << " neighbors";
    std::uniform_int_distribution<unsigned long> distribution(0, neighbors.size() - 1);
    auto num = distribution(random_engine_);

    VLOG(2) << "Picked " << num << "-th server";
    auto iterator = neighbors.begin();
    advance(iterator, num);

    const Server& server = (*iterator)->server();
    VLOG(2) << "Pinging " << server;

    SwimClient client(server, gossip_server().port(), ping_timeout().count());
    auto response = client.Ping();
    if (!response) {
      VLOG(2) << "Server " << server << " is not responding to ping";
      (*iterator)->set_timestamp(utils::CurrentTime());
      gossip_server().mutable_suspected()->insert(*iterator);
      gossip_server().mutable_alive()->erase(iterator);
    }
  } else {
    VLOG(2) << "No neighbor servers added, skipping ping";
  }
}

void GossipFailureDetector::SendReport() const {
  VLOG(2) << "Sending report";
  auto report = PrepareReport();

  auto neighbors = gossip_server().mutable_alive();
  for (auto server : *neighbors) {
    if (!server->didgossip()) {
      auto client = SwimClient(server->server());
      VLOG(2) << "Sending report to " << server->server();
      client.Send(report);
      server->set_didgossip(true);
      return;
    }
  }

  // If we got here, it means we have sent updates to all our neighbors in the past; we can then
  // select just one at random and send an update.
  std::uniform_int_distribution<unsigned long> distribution(0, alive().size() - 1);
  auto iterator = alive().begin();
  auto num = distribution(random_engine_);
  advance(iterator, num);
  auto client = SwimClient((*iterator)->server());
  client.Send(report);
  VLOG(2) << "Sent report to " << (*iterator)->server();
}

void GossipFailureDetector::GarbageCollectSuspected() const {
  auto suspects = gossip_server().mutable_suspected();
  auto expiredTime = ::utils::CurrentTime() - grace_period().count();
  VLOG(2) << "Evicting suspects last seen before " << expiredTime;
  for (const auto &suspect : *suspects) {
    if (suspect->timestamp() < expiredTime) {
      VLOG(2) << "Server " << suspect->server() << " last seen at: "
              << suspect->timestamp() << " exceeded grace period, presumed dead";
      suspects->erase(suspect);
    }
  }
}
} // namespace swim



