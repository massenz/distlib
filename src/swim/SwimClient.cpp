// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.



#include <zmq.h>
#include "SwimClient.hpp"


using namespace zmq;
using namespace utils;

namespace swim {

const unsigned long SwimClient::DEFAULT_TIMEOUT_MSEC = 25;
const unsigned int SwimClient::DEFAULT_SOCKET_LINGER_MSEC = 0;

SwimClient::SwimClient(const Server &dest, int self_port, unsigned long timeout) :
    dest_(dest), timeout_(timeout) {
  self_.set_hostname(utils::Hostname());
  self_.set_port(self_port);
}


bool SwimClient::Ping() const {
  SwimEnvelope message;
  message.set_type(SwimEnvelope::Type::SwimEnvelope_Type_STATUS_UPDATE);

  if (!postMessage(&message)) {
    LOG(ERROR) << "Failed to receive reply from server";
    return false;
  }
  return true;
}


bool SwimClient::Send(const SwimReport& report) const {
  SwimEnvelope message;

  message.set_type(SwimEnvelope_Type_STATUS_REPORT);
  message.mutable_report()->CopyFrom(report);

  return postMessage(&message);
}

bool SwimClient::RequestPing(const Server *other) const {
  SwimEnvelope message;
  message.set_type(SwimEnvelope_Type_STATUS_REQUEST);
  message.set_allocated_destination_server(const_cast<Server *>(other));

  return postMessage(&message);
}

unsigned long SwimClient::timeout() const {
  return timeout_;
}

void SwimClient::set_timeout(unsigned long timeout_) {
  SwimClient::timeout_ = timeout_;
}

unsigned int SwimClient::max_allowed_reports() const {
  return max_allowed_reports_;
}

void SwimClient::set_max_allowed_reports(unsigned int max_allowed_reports_) {
  SwimClient::max_allowed_reports_ = max_allowed_reports_;
}

bool SwimClient::postMessage(SwimEnvelope *envelope) const {

  envelope->mutable_sender()->CopyFrom(self_);
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
    LOG(ERROR) << "Timed out waiting for response from " << destinationUri();
    return false;
  }


  VLOG(2) << "Connected to server";
  message_t reply;
  if (!socket.recv(&reply)) {
    LOG(ERROR) << "Failed to receive reply from server" << destinationUri();
    return false;
  }

  char response[reply.size() + 1];
  VLOG(2) << "Received: " << reply.size() << " bytes from " << destinationUri();
  memset(response, 0, reply.size() + 1);
  memcpy(response, reply.data(), reply.size());
  VLOG(2) << "Response: " << response;

  return strcmp(response, "OK") == 0;
//  return reply.size() == 2;
}

} // namespace swim
