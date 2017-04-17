// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.

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
  socket.bind(utils::sockAddr(port_).c_str());
  LOG(INFO) << "Server listening on port " << port_;

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
      if (!message.ParseFromArray(msg.data(), msg.size())) {
        LOG(ERROR) << "Cannot serialize data to `SwimEnvelope` protocol buffer";
        continue;
      }

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
    }
  }
  LOG(WARNING) << "SERVER STOPPED";
}

} // namespace swim
