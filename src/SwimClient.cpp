// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.


#include <zmq.hpp>

#include "SwimClient.hpp"


using namespace zmq;

namespace swim {


SwimClient::SwimClient(const Server &dest, int self_port, unsigned long timeout) :
    server_(dest), timeout_(timeout) {
  self_ = makeServer(hostname(), self_port);
}

void SwimClient::ping() {

  SwimEnvelope message;
  message.set_type(SwimEnvelope::Type::SwimEnvelope_Type_STATUS_UPDATE);
  message.mutable_sender()->set_hostname(self_.hostname());
  message.mutable_sender()->set_port(self_.port());
  message.set_timestamp(current_time());

  std::string msgAsStr = message.SerializeAsString();
  char buf[msgAsStr.size()];
  memcpy(buf, msgAsStr.data(), msgAsStr.size());

  // TODO: make it better
  std::string destination = "tcp://" + server_.hostname() + ":" + std::to_string(server_.port());
  context_t ctx(1);
  socket_t socket(ctx, ZMQ_REQ);

  socket.connect(destination.c_str());
  message_t msg(buf, msgAsStr.size(), NULL);

  LOG(INFO) << "Sending Status Ping to server";
  // TODO: implement timing out if server does not respond
  if (!socket.send(msg)) {
    LOG(ERROR) << "Failed to send status out";
    return;
  }

  LOG(INFO) << "Message sent, awaiting reply";

  message_t reply;
  if (!socket.recv(&reply)) {
    LOG(ERROR) << "Failed to receive reply from server";
    return;
  }

  LOG(INFO) << "Received: " << reply.size() << " bytes";
}


void SwimClient::send(const SwimReport *report) {
  LOG(FATAL) << "Not implemented";
}

void SwimClient::requestPing(const Server *other) {
  LOG(FATAL) << "Not implemented";
}

} // namespace swim
