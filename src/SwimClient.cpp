// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.



#include <zmq.h>
#include "SwimClient.hpp"


using namespace zmq;

namespace swim {

const unsigned long SwimClient::DEFAULT_TIMEOUT_MSEC = 250;
const unsigned int SwimClient::DEFAULT_SOCKET_LINGER_MSEC = 150;

SwimClient::SwimClient(const Server &dest, int self_port, unsigned long timeout) :
    dest_(dest), timeout_(timeout) {
  self_ = makeServer(hostname(), self_port);
}


bool SwimClient::ping() {
  SwimEnvelope message;
  message.set_type(SwimEnvelope::Type::SwimEnvelope_Type_STATUS_UPDATE);

  if (!postMessage(&message)) {
    LOG(ERROR) << "Failed to receive reply from server";
    return false;
  }
  return true;
}


void SwimClient::send(const SwimReport *report) {
  SwimEnvelope message;
  message_t reply;

  message.set_type(SwimEnvelope_Type_STATUS_REPORT);
  message.set_allocated_report(const_cast<SwimReport *>(report));

  if (!postMessage(&message)) {
    LOG(ERROR) << "Could not post report";
  }
}

void SwimClient::requestPing(const Server *other) {
  SwimEnvelope message;
  message.set_type(SwimEnvelope_Type_STATUS_REQUEST);
  message.set_allocated_destination_server(const_cast<Server *>(other));

  if (!postMessage(&message)) {
    LOG(ERROR) << "Ping request to " << other->hostname() << " failed";
  }
}

unsigned long SwimClient::timeout() const {
  return timeout_;
}

void SwimClient::setTimeout(unsigned long timeout_) {
  SwimClient::timeout_ = timeout_;
}

unsigned int SwimClient::max_allowed_reports() const {
  return max_allowed_reports_;
}

void SwimClient::setMax_allowed_reports(unsigned int max_allowed_reports_) {
  SwimClient::max_allowed_reports_ = max_allowed_reports_;
}

bool SwimClient::postMessage(SwimEnvelope *envelope) const {

  envelope->mutable_sender()->set_hostname(self_.hostname());
  envelope->mutable_sender()->set_port(self_.port());
  envelope->set_timestamp(current_time());

  std::string msgAsStr = envelope->SerializeAsString();

  size_t msgSize = msgAsStr.size();
  char buf[msgSize];
  memcpy(buf, msgAsStr.data(), msgSize);

  context_t ctx(1);
  socket_t socket(ctx, ZMQ_REQ);
  socket.setsockopt (ZMQ_LINGER, &DEFAULT_SOCKET_LINGER_MSEC, sizeof (unsigned int));
  socket.connect(destinationUri().c_str());

  message_t msg(buf, msgSize, nullptr);


  VLOG(2) << "Connecting to " << destinationUri();
  if (!socket.send(msg)) {
    LOG(ERROR) << "Failed to send message to " << destinationUri();
    return false;
  }

  zmq_pollitem_t items[] = { { socket, 0, ZMQ_POLLIN, 0 }};
  poll(items, 1, timeout());

  if (!items[0].revents & ZMQ_POLLIN) {
    LOG(ERROR) << "Timed out";
    return false;
  }

  VLOG(2) << "Connected to server";
  message_t reply;
  if (!socket.recv(&reply)) {
    LOG(ERROR) << "Failed to receive reply from server";
    return false;
  }

  VLOG(2) << "Received: " << reply.size() << " bytes";
  return true;
}

} // namespace swim
