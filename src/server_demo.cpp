// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.


#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <zmq.hpp>

#include <glog/logging.h>

#include "config.h"

#include "utils/network.h"
#include "utils/ParseArgs.hpp"

#include "swim/SwimClient.hpp"


using namespace zmq;
using namespace swim;


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void listen_for_status(unsigned int port) {
  context_t ctx(1);
  socket_t socket(ctx, ZMQ_REP);
  socket.bind(utils::sockAddr(port).c_str());

  LOG(INFO) << "Server listening on port " << port;
  while (true) {
    message_t msg;
    if (!socket.recv(&msg)) {
      LOG(ERROR) << "Error receiving from socket";
      continue;
    }
    swim::SwimEnvelope envelope;
    envelope.ParseFromArray(msg.data(), msg.size());

    time_t ts = envelope.timestamp();

    LOG(INFO) << "Received from host '" << envelope.sender().hostname() << "' at "
              << std::put_time(std::gmtime(&ts), "%c %Z");

    message_t reply(6);
    memcpy(reply.data(), "200 OK", 6);
    socket.send(reply);
  }
}
#pragma clang diagnostic pop


static void usage() {
  std::cout << "Usage: " << program_invocation_short_name << " --port=PORT [--host=HOST] ACTION\n\n"
            << "\tPORT       an int specifying the port the server will listen on, or connect to;\n"
            << "\tHOST       the hostname/IP to send the status to (e.g., h123.example.org or \n"
            << "\t           192.168.1.1)\n"
            << "\tACTION     one of `send` or `receive`; if the former, also specifiy the host to\n"
            << "\t           send the data to.\n\n"
            << "HOST is only required when sending.\n\n";
}


int main(int argc, const char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = 1;

  LOG(INFO) << "Running Demo Ping Server (SWIM) - Ver. " << RELEASE_STR;

  utils::ParseArgs parser(argv, argc);
  parser.parse();

  int port = std::atoi(parser.parsed_options()["port"].c_str());
  if (port < 1024) {
    LOG(ERROR) << "Port should be specified with the --port option and be a valid number greater "
        "than 1024";
    usage();
    return EXIT_FAILURE;
  }

  if (parser.positional_args().size() != 1) {
    LOG(ERROR) << "Please specify an ACTION ('send' or 'receive')";
    return EXIT_FAILURE;
  }

  std::string action = parser.positional_args()[0];

  if (action == "send") {
    SwimClient client(parser.parsed_options()["host"], port);
    client.ping();
  } else if (action == "receive") {
    listen_for_status(port);
  } else {
    LOG(ERROR) << "One of {send, recevive} expected; we got instead: "
               << parser.positional_args()[0];
    usage();
    return EXIT_FAILURE;
  }

  LOG(INFO) << "done";
  return EXIT_SUCCESS;
}
