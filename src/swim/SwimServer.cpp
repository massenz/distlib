// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.



#include <iomanip>
#include <thread>

#include "SwimServer.hpp"


using namespace zmq;

namespace swim {

void SwimServer::start() {

  context_t ctx(1);
  socket_t socket(ctx, ZMQ_REP);
  if (!socket) {
    LOG(ERROR) << "Could not initialize the socket, this is a critical error, aborting.";
    return;
  }

  // No point in keeping the socket around when we exit.
  unsigned int linger = 0;
  socket.setsockopt(ZMQ_LINGER, &linger, sizeof (unsigned int));
  socket.bind(utils::sockAddr(port_).c_str());
  LOG(INFO) << "Server listening on port " << port_;

  // TODO: currently the server won't stop while waiting to receive incoming connection: should
  // use polling instead.
  stopped_ = false;
  while (!stopped_) {
    message_t msg;
    if (!socket.recv(&msg)) {
      LOG(FATAL) << "Error receiving from socket";
    }

    // We may have been stopped while waiting for incoming data.
    if (stopped_)
      break;

    SwimEnvelope message;
    if (!message.ParseFromArray(msg.data(), msg.size())) {
      LOG(ERROR) << "Cannot serialize data to `SwimEnvelope` protocol buffer";
      continue;
    }
    time_t ts = message.timestamp();

    // TODO: these should be invoked asynchronously.
    switch (message.type()) {
      case SwimEnvelope_Type_STATUS_UPDATE:
        onUpdate(message.release_sender(), ts);
        break;
      case SwimEnvelope_Type_STATUS_REPORT:
        onReport(message.release_sender(), message.release_report(), ts);
        break;
      case SwimEnvelope_Type_STATUS_REQUEST:
        onPingRequest(message.release_sender(), message.release_destination_server(), ts);
        break;
      default:
        LOG(ERROR) << "Unexpected message type: '" << message.type();
    }

    message_t reply(2);
    memcpy(reply.data(), "OK", 2);
    socket.send(reply);
  }
  LOG(WARNING) << "SERVER STOPPED";
}

} // namespace swim
