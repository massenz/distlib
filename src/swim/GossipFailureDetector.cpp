// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.

#include "GossipFailureDetector.hpp"

namespace swim {

bool operator<(const ServerRecord& lhs, const ServerRecord& rhs) {
  return lhs.server() < rhs.server();
}

bool operator<(const Server& lhs, const Server& rhs) {
  if (lhs.hostname() == rhs.hostname()) {
    return lhs.port() < rhs.port();
  }
  return lhs.hostname() < rhs.hostname();
}

bool operator==(const Server& lhs, const Server& rhs) {
  return !(lhs < rhs) && !(rhs < lhs);
}

void GossipFailureDetector::InitAllBackgroundThreads() {

  if (!gossip_server().isRunning()) {
    LOG(ERROR) << "SWIM Gossip Server is not running, please start() it before running the "
                  "detector's background threads";
    return;
  }

  threads_.push_back(std::unique_ptr<std::thread>(
      new std::thread([this]() {
        while (gossip_server().isRunning()) {
          SendReport();
          std::this_thread::sleep_for(update_round_interval_);
        }
      }))
  );

  threads_.push_back(std::unique_ptr<std::thread>(
      new std::thread([this]() {
        while (gossip_server().isRunning()) {
          GarbageCollectSuspected();
          std::this_thread::sleep_for(update_round_interval_);
        }
      }))
  );

  LOG(INFO) << "All Gossiping threads for the SWIM Detector started";
}


std::set<Server> GossipFailureDetector::GetUniqueNeighbors(int k) const {
  std::set<Server> others;
  unsigned int collisions = 0;
  const unsigned int kMaxCollisions = 3;

  for (int i = 0; i < std::min(num_reports_,
                               static_cast<const unsigned int>(gossip_server_->alive_size())); ++i) {
    const Server other = gossip_server_->GetRandomNeighbor();
    auto inserted = others.insert(other);
    if (!inserted.second && ++collisions > kMaxCollisions) {
      // We are hitting too many already randomly-picked neighbors, clearly the set is exhausted.
      break;
    }
  }
  return others;
}

void GossipFailureDetector::SendReport() const {
  if (gossip_server_->alive_empty()) {
    VLOG(2) << "No neighbors, skip sending report";
    return;
  }

  auto report = gossip_server_->PrepareReport();
  VLOG(2) << "Sending report, alive: " << report.alive_size() << "; suspected: "
          << report.suspected_size();

  for (auto other : GetUniqueNeighbors(num_reports_)) {
    auto client = SwimClient(other);
    VLOG(2) << "Sending report to " << other;

    if (!client.Send(report)) {
      // We managed to pick an unresponsive server; let's add to suspects.
      LOG(WARNING) << "Report sending failed; adding " << other << " to suspects";
      // TODO: ask m forwarders to reach this server (see #150911899).
      gossip_server_->ReportSuspected(other);
    } else {
      // All is well, simply update the timestamp of when we last "saw" this healthy server.
      gossip_server_->AddAlive(other);
    }
  }
}


void GossipFailureDetector::GarbageCollectSuspected() const {
  SwimReport report = gossip_server_->PrepareReport();

  auto expiredTime = ::utils::CurrentTime() - grace_period().count();
  VLOG(2) << "Evicting suspects last seen before " << expiredTime;

  for (const auto &suspectRecord : report.suspected()) {
    if (suspectRecord.timestamp() < expiredTime) {
      VLOG(2) << "Server " << suspectRecord.server() << " last seen at: "
              << suspectRecord.timestamp() << " exceeded grace period, presumed dead";
      gossip_server_->RemoveSuspected(suspectRecord.server());
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
} // namespace swim



