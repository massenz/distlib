// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.

#include <iostream>
#include "utils/network.h"
#include "SwimServer.hpp"


using namespace zmq;

namespace swim {

const unsigned long SwimServer::DEFAULT_POLLING_INTERVAL_MSEC = 50;
const unsigned int SwimServer::DEFAULT_SOCKET_LINGER_MSEC = 0;

void SwimServer::start() {

  context_t ctx(NUM_THREADS);
  socket_t socket(ctx, ZMQ_REP);
  if (!socket) {
    LOG(ERROR) << "Could not initialize the socket, this is a critical error, aborting.";
    return;
  }

  // No point in keeping the socket around when we exit.
  socket.setsockopt(ZMQ_LINGER, &DEFAULT_SOCKET_LINGER_MSEC, sizeof (unsigned int));
  auto address = utils::SocketAddress(port_);
  socket.bind(address.c_str());

  LOG(INFO) << "Server listening on: " << address;

  // Polling from socket, so stopping the server does not hang indefinitely in the absence of
  // incoming messages.
  zmq_pollitem_t items[] = {
      {socket, 0, ZMQ_POLLIN, 0}
  };
  stopped_ = false;
  while (!stopped_) {
    // TODO: make timeout configurable; also confirm the timeout is in msec.
    long timeout = 10;
    int rc = zmq_poll(items, 1, timeout);
    if (rc > 0 && items[0].revents && ZMQ_POLLIN) {
      // FIXME: without this noop wait, the server crashes with a SIGSEGV.
      //    A yield() also won't work, as moving the message_t initialization place.
      std::this_thread::sleep_for(std::chrono::milliseconds(0));
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
            onUpdate(message.release_sender());
            break;
          case SwimEnvelope_Type_STATUS_REPORT:
            VLOG(2) << "Received a STATUS_REPORT message";
            onReport(message.release_sender(), message.release_report());
            break;
          case SwimEnvelope_Type_STATUS_REQUEST:
            VLOG(2) << "Received a STATUS_REQUEST message";
            onPingRequest(message.release_sender(), message.release_destination_server());
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

void SwimServer::onPingRequest(Server* sender, Server* destination) {

  Server from(*sender);

  std::thread([this, destination, from] {

    std::shared_ptr<Server> pdest(destination);

    // TODO: the timeout should be a property that we could set; for now using the default value.
    SwimClient client(*destination, port_);
    if (client.Ping()) {
      VLOG(2) << "Forwarded request to " << *destination << " succeded; reporting ALIVE";
      SwimReport report;
      report.mutable_alive()->Add()->CopyFrom(*MakeRecord(*destination));

      SwimClient respond(from, port_);
      respond.Send(report);
    } else {
      mutex_guard lock(suspected_mutex_);
      suspected_.insert(MakeRecord(*destination));
      VLOG(2) << "Forwarded request for PING to " << *destination
              << " failed: added to SUSPECTED list here too.";
    }
  }).detach();


  // We also update the sender's status to be "alive" (and this will
  // also take care of destroying the resource).
  VLOG(2) << "Updating status for " << *sender << " as ALIVE";
  onUpdate(sender);
}

SwimReport SwimServer::PrepareReport() const {
  SwimReport report;
  report.mutable_sender()->CopyFrom(self());

  {
    mutex_guard lock(alive_mutex_);

    for (const auto &item : alive_) {
      if (!item->didgossip()) {
        ServerRecord *prec = report.mutable_alive()->Add();
        prec->CopyFrom(*item);
      }
    }
  }

  {
    mutex_guard lock(suspected_mutex_);

    for (const auto &item : suspected_) {
      if (!item->didgossip()) {
        ServerRecord *prec = report.mutable_suspected()->Add();
        prec->CopyFrom(*item);
      }
    }
  }

  return report;
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

bool SwimServer::ReportSuspect(const Server &server) {
  size_t num{0};
  std::shared_ptr<ServerRecord> suspectRecord = MakeRecord(server);

  // First remove it from the alive set, if there.
  {
    mutex_guard lock(alive_mutex_);
    num = alive_.erase(suspectRecord);
  }
  VLOG(2) << "Removed " << num << " servers from the alive set.";

  // Then add it to the suspected set.
  mutex_guard lock(suspected_mutex_);
  auto inserted = suspected_.insert(suspectRecord);
  return inserted.second;
}

bool SwimServer::AddAlive(const Server &server) {
  RemoveSuspected(server);

  std::shared_ptr<ServerRecord> aliveRecord = MakeRecord(server);
  mutex_guard lock(alive_mutex_);
  auto inserted = alive_.insert(aliveRecord);
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

void SwimServer::onReport(Server *sender, SwimReport *report) {
  std::unique_ptr<Server> ps(sender);

  // TODO: grabbing two locks in sequence is a sure recipe for deadlocks: use unique_lock to
  // add timeouts and backing off if exceeded.
  //
  // We grab both locks here in a very broad scope as this is the only method that requires
  // BOTH mutexes, so we prevent it from running at all, if either lock is already taken.
  VLOG(2) << "onReport: About to grab both mutexes";
  mutex_guard alive_lock(alive_mutex_);
  mutex_guard suspected_lock(suspected_mutex_);
  VLOG(2) << "onReport: mutexes locked";

  VLOG(2) << self() << " received: " << *report;
  for (auto record : report->alive()) {
    if (record.server() == self()) {
      // yes, we know we are alive, thank you very much.
      continue;
    }

    std::shared_ptr<ServerRecord> pRecord(new ServerRecord(record));
    pRecord->set_didgossip(false);

    auto was_suspected = suspected_.find(pRecord);
    if (was_suspected != suspected_.end() &&
        (*was_suspected)->timestamp() < record.timestamp()) {
      suspected_.erase(pRecord);
      alive_.insert(pRecord);
    } else if (was_suspected == suspected_.end()) {
      alive_.insert(pRecord);
    }
  }

  for (auto record : report->suspected()) {
    if (record.server() == self()) {
      // Reports of our death were greatly exaggerated.
      VLOG(2) << *sender << " reported this server (" << self() << ") as 'suspected'; pinging";
      SwimClient client(*sender, port());
      client.Ping();
      continue;
    }

    std::shared_ptr<ServerRecord> pRecord(new ServerRecord(record));
    if (suspected_.find(pRecord) == suspected_.end()) {

      pRecord->set_didgossip(false);
      pRecord->set_timestamp(::utils::CurrentTime());

      auto was_alive = alive_.find(pRecord);
      if (was_alive != alive_.end() && (*was_alive)->timestamp() < record.timestamp()) {
        // This is genuine new news and we need to update our records.
        suspected_.insert(pRecord);
        alive_.erase(pRecord);
      } else if (was_alive == alive_.end()) {
        // If we are here, it means that we haven't seen this server before, and
        // we should record it as "suspicious."
        suspected_.insert(pRecord);
      }
    }
  }

  VLOG(2) << "After merging, alive: " << alive_
          << "; suspected: " << suspected_;
  VLOG(2) << "onReport: about to release both locks";
}

void SwimServer::onUpdate(Server *client) {
  // Make sure client will be deleted, even if an exception is thrown.
  std::unique_ptr<Server> ps(client);

  std::shared_ptr<ServerRecord> record = MakeRecord(*client);
  {
    mutex_guard lock(alive_mutex_);
    alive_.insert(record);
  }

  // If it was previously suspected of being unresponsive, this server is removed from the
  // suspected list:
  unsigned long removed;
  {
    mutex_guard lock(suspected_mutex_);
    removed = suspected_.erase(record);
  }
  if (removed > 0) {
    VLOG(2) << *client << " previously suspected; added back to healthy set";
  } else {
    VLOG(3) << "Received a ping from " << *client;
  }
}
} // namespace swim
