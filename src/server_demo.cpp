// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.

#include <glog/logging.h>
#include <iostream>
#include <zmq.hpp>

#include "config.h"
#include "SwimPing.hpp"


const char* LISTEN_SKT = "tcp://*:3003";
const char* SEND_SKT = "tcp://localhost:3003";


void listen_for_status();

using namespace zmq;


void send_ok_status() {
  context_t ctx(1);
  socket_t socket(ctx, ZMQ_REQ);
  socket.connect(SEND_SKT);

  SwimStatus status;

  // TODO: fetch hostname from the system.
  status.set_hostname("gondor");
  status.set_state(SwimStatus::State::SwimStatus_State_RUNNING);
  status.set_port(3003);

  // TODO: fetch current time epoch from system.
  status.set_timestamp(123456789);
  status.set_description("Demo server - running");


  std::string msgAsStr = status.SerializeAsString();
  char buf[msgAsStr.size()];
  memcpy(buf, msgAsStr.data(), msgAsStr.size());

  message_t msg(buf, msgAsStr.size(), NULL);

  LOG(INFO) << "Sending Status Ping to server";
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

  char response[reply.size() + 1];
  memcpy(response, reply.data(), reply.size());
  response[reply.size()] = '\0';

  LOG(INFO) << "Response: " << response;
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void listen_for_status() {
  context_t ctx(1);
  socket_t socket(ctx, ZMQ_REP);
  socket.bind(LISTEN_SKT);

  LOG(INFO) << "Connecting....";
  while (true) {
    message_t msg;
    if (!socket.recv(&msg))
      LOG(FATAL) << "Error receiving from socket";

    LOG(INFO) << "Received " << msg.size() << " bytes";

    char buf[msg.size() + 1];
    memset(buf, 0, msg.size() + 1);
    memcpy(buf, msg.data(), msg.size());

    SwimStatus status;
    status.ParseFromArray(buf, msg.size());

    LOG(INFO) << "Received from host '" << status.hostname() << "': "
              << status.State_Name(status.state());

    message_t reply(7);
    memcpy(reply.data(), "200 OK\n", 7);
    socket.send(reply);
  }
}
#pragma clang diagnostic pop


int main(int argc, char** argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = 1;

  LOG(INFO) << "Running Demo Ping Server (SWIM) - Ver. " << RELEASE_STR;

  if (argc != 2) {
    LOG(ERROR) << "Exactly one of {send, receive} was expected, none given";
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "send") == 0) {
    send_ok_status();
  } else if (strcmp(argv[1], "receive") == 0) {
    listen_for_status();
  } else {
    LOG(ERROR) << "One of {send, recevive} expected; we got instead: " << argv[1];
    return EXIT_FAILURE;
  }

  LOG(INFO) << "done";
  return EXIT_SUCCESS;
}
