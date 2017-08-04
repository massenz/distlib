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

  threads_.push_back(std::unique_ptr<std::thread>(
      new std::thread([this]() {
        while (gossip_server().isRunning()) {
          PingNeighbor();
          std::this_thread::sleep_for(ping_interval());
        }
      }))
  );

  threads_.push_back(std::unique_ptr<std::thread>(
      new std::thread([this]() {
        while (gossip_server().isRunning()) {
          SendReport();
          std::this_thread::sleep_for(update_round_interval());
        }
      }))
  );

  threads_.push_back(std::unique_ptr<std::thread>(
      new std::thread([this]() {
        while (gossip_server().isRunning()) {
          GarbageCollectSuspected();
          std::this_thread::sleep_for(ping_interval());
        }
      }))
  );

  LOG(INFO) << "All Gossiping threads for the SWIM Detector started";
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
      // TODO: forward request to ping to a set of kForwarderCount neighbors.

      // TODO: remove unnecessary trace once confirmed all tests pass.
      VLOG(3) << "Before: " << gossip_server().alive().size() << " alive; "
              << gossip_server().suspected().size() << " suspected";

      LOG(WARNING) << server << " is not responding to ping";

      std::shared_ptr<ServerRecord> suspectRecord(new ServerRecord());
      suspectRecord->set_allocated_server(new Server(server));
      suspectRecord->set_timestamp(utils::CurrentTime());
      suspectRecord->set_didgossip(false);

      // TODO: we are mutating the alive/suspected containers, there is a race with PrepareReport.
      //     Must add a mutex_lock around these two statements.
      gossip_server().mutable_suspected()->insert(suspectRecord);
      gossip_server().mutable_alive()->erase(*iterator);

      VLOG(3) << "After: " << gossip_server().alive().size() << " alive; "
              << gossip_server().suspected().size() << " suspected";
    }
  } else {
    VLOG(2) << "No servers in group, skipping ping";
  }
}

void GossipFailureDetector::SendReport() const {
  if (gossip_server().alive().empty()) {
    VLOG(2) << "No neighbors, skip sending report";
    return;
  }

  auto report = PrepareReport();
  VLOG(2) << "Sending report, alive: " << report.alive_size() << "; suspected: "
          << report.suspected_size();

  auto neighbors = gossip_server().mutable_alive();
  for (auto server : *neighbors) {
    if (!server->didgossip()) {
      auto client = SwimClient(server->server());
      VLOG(2) << "Sending report to " << server->server();

      // Either way, we want to mark the time we connected (or tried to connect) to this server.
      server->set_timestamp(::utils::CurrentTime());

      if (client.Send(report)) {
        server->set_didgossip(true);
        return;
      }

      // We managed to pick an unresponsive server; let's add to suspects, then let the loop
      // continue.
      gossip_server().mutable_suspected()->insert(server);
      gossip_server().mutable_alive()->erase(server);
    }
  }

  // If we got here, it means we have sent updates to all our neighbors in the past; we can then
  // select just one at random and send an update.
  std::uniform_int_distribution<unsigned long> distribution(0, alive().size() - 1);
  auto num = distribution(random_engine_);

  auto iterator = alive().begin();
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

void GossipFailureDetector::StopAllBackgroundThreads() {

  LOG(WARNING) << "Stopping background threads for SWIM protocol; the server will be "
               << "briefly stopped, then restarted, so that it continues to respond to "
               << "pings and forwarding requests; and receiving SWIM reports.";
  bool server_was_stopped = false;

  if (gossip_server_->isRunning()) {
    VLOG(2) << "Temporarily stopping server to allow threads to drain gracefully";
    gossip_server_->stop();
    server_was_stopped = true;
  }

  // A brief wait to allow the news that the server is stopped to spread.
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  VLOG(2) << "Waiting for threads to stop";
  for (auto const &thread : threads_) {
    if (thread->joinable()) {
      thread->join();
    }
  }

  if (server_was_stopped) {
    VLOG(2) << "Restarting server " << gossip_server().self();
    std::thread t([this] { gossip_server_->start(); });
    t.detach();
    // A brief wait to allow the news that the server is starting to spread.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!gossip_server_->isRunning()) {
      LOG(FATAL) << "Failed to restart the server, terminating";
    }
  }
  LOG(WARNING) << "All Gossiping threads for the SWIM Detector terminated; this detector is "
               << "no longer participating in Gossip.";
}

void GossipFailureDetector::set_update_round_interval(const seconds &update_round_interval_) {
  GossipFailureDetector::update_round_interval_ = update_round_interval_;
}

void GossipFailureDetector::set_grace_period(const seconds &grace_period_) {
  GossipFailureDetector::grace_period_ = grace_period_;
}

void GossipFailureDetector::set_ping_timeout(const milliseconds &ping_timeout_) {
  GossipFailureDetector::ping_timeout_ = ping_timeout_;
}

void GossipFailureDetector::set_ping_interval(const seconds &ping_interval_) {
  GossipFailureDetector::ping_interval_ = ping_interval_;
}
} // namespace swim



