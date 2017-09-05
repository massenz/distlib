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

void GossipFailureDetector::InitAllBackgroundThreads() {

  if (!gossip_server().isRunning()) {
    LOG(ERROR) << "SWIM Gossip Server is not running, please start() it before running the "
                  "detector's background threads";
    return;
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
//  const ServerRecordsSet& neighbors = gossip_server_->alive();
  if (!gossip_server_->alive_empty()) {
    const Server server = gossip_server_->GetRandomNeighbor();
    VLOG(2) << "Pinging " << server;

    SwimClient client(server, gossip_server().port(), ping_timeout().count());
    auto response = client.Ping();
    if (!response) {
      // TODO: forward request to ping to a set of kForwarderCount neighbors.

      LOG(WARNING) << server << " is not responding to ping";
      if (gossip_server_->ReportSuspect(server)) {
        VLOG(2) << "Server " << server << " added to the suspected set";
      } else {
        LOG(WARNING) << "Could not add " << server << " to the suspected set";
      }
    }
  } else {
    VLOG(2) << "No servers in group, skipping ping";
  }
}

void GossipFailureDetector::SendReport() const {
  if (gossip_server_->alive_empty()) {
    VLOG(2) << "No neighbors, skip sending report";
    return;
  }

  auto report = gossip_server_->PrepareReport();
  VLOG(2) << "Sending report, alive: " << report.alive_size() << "; suspected: "
          << report.suspected_size();

  // TODO: make the number of reports sent out configurable; only sending one at a time now.
  const Server other = gossip_server_->GetRandomNeighbor();
  auto client = SwimClient(other);
  VLOG(2) << "Sending report to " << other;

  if (!client.Send(report)) {
    // TODO: ask m forwarders to reach this server.
    // We managed to pick an unresponsive server; let's add to suspects.
    LOG(WARNING) << "Report sending failed; adding " << other << " to suspects";
    gossip_server_->ReportSuspect(other);
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



