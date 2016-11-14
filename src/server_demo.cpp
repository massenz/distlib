// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.


#include <ctime>
#include <iostream>
#include <zmq.hpp>

#include <glog/logging.h>

#include "config.h"
#include "network.h"

#include "SwimClient.hpp"


#define PORT_NUM 50050


using namespace zmq;
using namespace swim;


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

    swim::SwimEnvelope envelope;
    envelope.ParseFromArray(buf, msg.size());

    LOG(INFO) << "Received from host '" << envelope.sender().hostname() << "' at "
              << envelope.timestamp(); // TODO: convert to human-readable format

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

  if (argc < 3) {
    usage();
    return EXIT_FAILURE;
  }


  if (strcmp(argv[1], "send") == 0 && (argc == 4)) {
    SwimClient client(argv[2], atoi(argv[3]));
    client.ping();
  } else if (strcmp(argv[1], "receive") == 0 && argc == 3) {
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
