// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.



#include <iomanip>
#include "SwimServer.hpp"


using namespace zmq;

namespace swim {

void SwimServer::start() {
  context_t ctx(1);
  socket_.reset(new socket_t(ctx, ZMQ_REP));
  if (!socket_) {
    LOG(ERROR) << "Could not initialize the socket, this is a critical error, aborting.";
    stopped_.store(true);
    return;
  }
  socket_->bind(utils::sockAddr(port_).c_str());

  LOG(INFO) << "Server listening on port " << port_;
  while (!stopped_.load()) {
    message_t msg;
    if (!socket_->recv(&msg)) {
      LOG(ERROR) << "Error receiving from socket";
      continue;
    }
    SwimEnvelope message;
    message.ParseFromArray(msg.data(), msg.size());

    time_t ts = message.timestamp();

    LOG(INFO) << "Received from host '" << message.sender().hostname() << "' at "
              << std::put_time(std::gmtime(&ts), "%c %Z");

    if (message.type() == SwimEnvelope_Type_STATUS_UPDATE) {
      onUpdate(message.sender());
    }

    message_t reply(2);
    memcpy(reply.data(), "OK", 2);
    socket_->send(reply);
  }
}

void SwimServer::onUpdate(const Server &server) {
  // TODO: this should be instead one of the callbacks.
  LOG(INFO) << "Server '" << server.hostname() << "' is dandy";
}

} // namespace swim
