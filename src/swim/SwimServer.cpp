// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.


#include "utils/utils.hpp"
#include "swim/SwimServer.hpp"


using namespace zmq;

namespace swim {

void SwimServer::start() {

  context_t ctx(kNumThreads);
  socket_t socket(ctx, ZMQ_REP);
  if (!socket) {
    LOG(ERROR) << "Could not initialize the socket, this is a critical error, aborting.";
    return;
  }

  // No point in keeping the socket around when we exit.
  socket.setsockopt(ZMQ_LINGER, &kDefaultSocketLingerMsec, sizeof (unsigned int));
  auto address = utils::SocketAddress(port_);
  VLOG(2) << "Binding Socket to: " << address << ":" << port_;

  LOG(INFO) << "Server listening on: " << address;
  socket.bind(address.c_str());


  // Polling from socket, so stopping the server does not hang indefinitely in the absence of
  // incoming messages.
  zmq_pollitem_t items[] = {
      {socket, 0, ZMQ_POLLIN, 0}
  };
  stopped_ = false;
  while (!stopped_) {
    int rc = zmq_poll(items, 1, polling_interval_);
    if (rc > 0 && items[0].revents && ZMQ_POLLIN) {
      message_t msg;
      if (!socket.recv(&msg)) {
        LOG(FATAL) << "Error receiving from socket";
      }

      SwimEnvelope message;
      if (message.ParseFromArray(msg.data(), msg.size())) {

        // TODO: these should be invoked asynchronously.
        switch (message.type()) {
          case SwimEnvelope_Type_STATUS_UPDATE:
            VLOG(2) << "Received a STATUS_UPDATE message";
            OnUpdate(message.release_sender());
            break;
          case SwimEnvelope_Type_STATUS_REPORT:
            VLOG(2) << "Received a STATUS_REPORT message";
            OnReport(message.release_sender(), message.release_report());
            break;
          case SwimEnvelope_Type_STATUS_REQUEST:
            VLOG(2) << "Received a STATUS_REQUEST message";
            OnForwardRequest(message.release_sender(), message.release_destination_server());
            break;
          default:
            LOG(ERROR) << "Unexpected message type: '" << message.type();
        }

        message_t reply(2);
        memcpy(reply.data(), "OK", 2);
        socket.send(reply);

      } else {
        LOG(ERROR) << "Cannot serialize data to `SwimEnvelope` protocol buffer";
        message_t reply(4);
        memcpy(reply.data(), "FAIL", 4);
        socket.send(reply);
      }
    }
  }
  LOG(WARNING) << "SERVER STOPPED: " << self();
}

void SwimServer::OnForwardRequest(Server *sender, Server *destination) {

  // First off, the sender is alive and well.
  AddAlive(*sender);

  // We do the ping forwarding in a background thread, or we may cause a timeout for the waiting
  // requestor, and this server would be incorrectly reported as unresponsive (while, in fact, it
  // may be `destination` that is not responding).
  //
  std::thread ping_t{[=] {
    // As per the SWIM protocol, this server will attempt to communicate with the "suspected"
    // server, and will update *its own* records accordingly; but it will *not* report back to
    // the original requestor; instead, it will send a report to `destination` reporting it as
    // "suspected", but with the `sender` as the original requestor, not itself.
    //
    // Upon receiving a report of being "suspected" a SwimServer attempts to contact back the
    // `sender` so that the latter can update its records.
    SwimReport report;

    // By using the `allocated` versions of the setters we also ensure memory will be freed upon
    // destruction of the PB.
    report.set_allocated_sender(sender);
    auto record = ::swim::MakeRecord(*destination);
    report.mutable_suspected()->AddAllocated(record.release());

    // TODO: the timeout should be a property that we could set; for now using the default value.
    SwimClient client(*destination, port_);
    if (!client.Send(report)) {
      VLOG(2) << self() << ": Forwarded request to " << *destination
              << " failed; reporting SUSPECTED";
      ReportSuspected(*destination);
    }
  }};
  ping_t.detach();
}

SwimReport SwimServer::PrepareReport() const {
  SwimReport report;
  std::vector<ServerRecord> records;

  report.mutable_sender()->CopyFrom(self());
  {
    mutex_guard lock(alive_mutex_);

    // Defensive copy.
    // Keep hold of the lock for the least amount of time necessary to read data.
    for (const auto &record : alive_) {
      records.push_back(*record);
    }
  }
  AddRecordsToBudget(report, records, kAlive);
  records.clear();
  {
    mutex_guard lock(suspected_mutex_);

    for (const auto &item : suspected_) {
      records.push_back(*item);
    }
  }
  AddRecordsToBudget(report, records, kSuspected);

  return report;
}

void SwimServer::AddRecordsToBudget(SwimReport &report,
                                    std::vector<ServerRecord> &records,
                                    const ReportSelector &which) const {
  google::uint64 now = ::utils::CurrentTime();
  double running_cost = 0.0;

  // NOTE we are sorting in *descending* order, and adding most recent records first.
  sort(records.begin(),
       records.end(),
       [](const ServerRecord& r1, const ServerRecord& r2) {
              return r1.timestamp() > r2.timestamp();
            });

  for (const auto &item : records) {
    google::uint64 dt = item.timestamp() - now;
    running_cost += cost(dt);
    if (running_cost > kTimeDecayBudget) break;
    ServerRecord *prec = which == kAlive ?
                         report.mutable_alive()->Add() :
                         report.mutable_suspected()->Add();
    prec->CopyFrom(item);
  }
}

Server SwimServer::GetRandomNeighbor() const {

  // It is IMPORTANT that calls to alive_size() (and _empty()) are done OUTSIDE of
  // the critical section guarded by the mutex, as it is NOT re-entrant and thus causes
  // the thread to wait indefinitely.
  auto size = alive_size();
  if (size == 0) {
    throw empty_set();
  }

  std::uniform_int_distribution<unsigned long> distribution(0,  size - 1);
  auto num = distribution(swim::random_engine);

  mutex_guard lock(alive_mutex_);
  auto iterator = alive_.begin();
  advance(iterator, num);

  assert(iterator != alive_.end());
  VLOG(2) << "Picked " << num << "-th server (of " << size << ")";

  return  (*iterator)->server();
}

bool SwimServer::ReportSuspected(const Server &server,
                                 google::uint64 timestamp) {

  if (server.port() == 0) {
    VLOG(3) << "Refused to add a port 0 server to suspect set";
    return false;
  }

  size_t num{0};
  std::shared_ptr<ServerRecord> suspectRecord = MakeRecord(server);
  suspectRecord->set_timestamp(timestamp);

  // First remove it from the alive set, if there.
  {
    mutex_guard lock(alive_mutex_);
    num = alive_.erase(suspectRecord);
  }
  if (num > 0) {
    VLOG(2) << "Removed " << server << " from the alive set.";
  }

  // Then add it to the suspected set.
  mutex_guard lock(suspected_mutex_);
  auto inserted = suspected_.insert(suspectRecord);
  return inserted.second;
}

bool SwimServer::AddAlive(const Server &server,
                          google::uint64 timestamp) {
  if (server.port() == 0) {
    VLOG(3) << "Refused to add a port 0 server to alive set";
    return false;
  }

  RemoveSuspected(server);

  std::shared_ptr<ServerRecord> aliveRecord = MakeRecord(server);
  aliveRecord->set_timestamp(timestamp);

  mutex_guard lock(alive_mutex_);
  auto inserted = alive_.insert(aliveRecord);

  // If we already knew of this server being healthy, all we have to do
  // is update the timestamp of the last time we saw it.
  if (!inserted.second) {
    auto pr = alive_.find(aliveRecord);
    if (pr != alive_.end()) {
      (*pr)->set_timestamp(timestamp);
    }
  }
  return inserted.second;
}

void SwimServer::RemoveSuspected(const Server &server) {
  std::shared_ptr<ServerRecord> aliveRecord = MakeRecord(server);
  size_t num{0};
  {
    mutex_guard lock(suspected_mutex_);
    num = suspected_.erase(aliveRecord);
  }
  if (num > 0) {
    VLOG(2) << "Removed " << num << " entry from the suspected set";
  }
}

void SwimServer::OnReport(Server *sender, SwimReport *report) {
  std::unique_ptr<Server> ps(sender);

  VLOG(2) << self() << ": received Report from " << *sender;
  AddAlive(*sender);

  for (const auto &record : report->alive()) {
    if (record.server() == self()) {
      // yes, we know we are alive, thank you very much.
      continue;
    }
    // First off, let's make sure this information is not stale; i.e., this same server was
    // suspected *after* this report that it's healthy.
    {
      mutex_guard lock(suspected_mutex_);
      auto found = suspected_.find(MakeRecord(record.server()));
      if (found != suspected_.end() && (*found)->timestamp() > record.timestamp()){
        continue;
      }
    }
    // This will either add a newly found healthy server; or simply update the timestamp for one
    // we already knew about.
    AddAlive(record.server(), record.timestamp());
  }

  for (const auto &record : report->suspected()) {
    if (record.server() == self()) {
      // Reports of our death were greatly exaggerated.
      VLOG(2) << self() << ": " << report->sender()
              << " reported this server as 'suspected' pinging";
      SwimClient client(report->sender(), port_);
      client.Ping();
      continue;
    }

    // If this same server was reported healthy *after* this report, we should ignore this.
    {
      mutex_guard lock(alive_mutex_);
      auto found = alive_.find(MakeRecord(record.server()));
      if (found != alive_.end() && (*found)->timestamp() > record.timestamp()) {
        continue;
      }
    }
    ReportSuspected(record.server(), record.timestamp());
  }
}

void SwimServer::OnUpdate(Server *client) {
  // Make sure pointer will be deleted, even if an exception is thrown.
  std::unique_ptr<Server> ps(client);

  VLOG(3) << "Received a ping from " << *client;
  AddAlive(*ps);

  // If it was previously suspected of being unresponsive, this server is removed from the
  // suspected list:
  std::shared_ptr<ServerRecord> record = MakeRecord(*client);
  unsigned long removed;
  {
    mutex_guard lock(suspected_mutex_);
    removed = suspected_.erase(record);
  }
  if (removed > 0) {
    VLOG(2) << *client << " previously suspected; added back to healthy set";
  }
}
} // namespace swim
