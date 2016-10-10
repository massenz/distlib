// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.


#include <ctime>
#include <iostream>
#include <unistd.h>
#include <zmq.hpp>

#include <glog/logging.h>

#include "config.h"
#include "network.h"

#include "SwimPing.hpp"


#define MAXBUFSIZE 1024
#define PORT_NUM 50050


using namespace zmq;


google::protobuf::int64 current_time() {
  return time(nullptr);
}


std::string hostname() {
  char name[MAXBUFSIZE];
  int retcode = gethostname(name, MAXBUFSIZE);
  if (retcode != 0) {
    LOG(ERROR) << "Could not determine hostname";
    return "UNKNOWN";
  }
  return std::string(name);
}


void send_ok_status(const std::string& destination) {
  context_t ctx(1);
  socket_t socket(ctx, ZMQ_REQ);
  socket.connect(destination.c_str());

  SwimStatus status;
  status.set_state(SwimStatus::State::SwimStatus_State_RUNNING);
  status.set_timestamp(current_time());
  status.set_description("demo_client::single");

  Server *self = status.mutable_server();
  self->set_hostname(hostname());
  self->set_port(PORT_NUM);

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
void listen_for_status(unsigned int port) {
  context_t ctx(1);
  socket_t socket(ctx, ZMQ_REP);
  socket.bind(sockAddr(port).c_str());

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

    LOG(INFO) << "Received from host '" << status.server().hostname() << "': "
              << status.State_Name(status.state());

    message_t reply(7);
    memcpy(reply.data(), "200 OK\n", 7);
    socket.send(reply);
  }
}
#pragma clang diagnostic pop


static void usage() {
  std::cout << "Usage: " << program_invocation_short_name << " send DEST | receive PORT\n\n"
            << "PORT   an int specifying the port the server will listen on;\n"
            << "DEST   the URI to send the status to (e.g., tcp://localhost:3003\n\n"
            << "Use PORT when receiving and DEST when sending.";
}


int main(int argc, char** argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = 1;

  LOG(INFO) << "Running Demo Ping Server (SWIM) - Ver. " << RELEASE_STR;

  if (argc != 3) {
    usage();
    return EXIT_FAILURE;
  }


  if (strcmp(argv[1], "send") == 0) {
    send_ok_status(argv[2]);
  } else if (strcmp(argv[1], "receive") == 0) {
    unsigned int port = atoi(argv[2]);
    listen_for_status(port);
  } else {
    usage();
    LOG(ERROR) << "One of {send, recevive} expected; we got instead: " << argv[1];
    return EXIT_FAILURE;
  }

  LOG(INFO) << "done";
  return EXIT_SUCCESS;
}
