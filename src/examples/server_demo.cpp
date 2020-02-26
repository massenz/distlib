// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.


#include <chrono>
#include <iostream>
#include <iomanip>
#include <thread>

#include <glog/logging.h>

#include <zmq.hpp>
#include <atomic>

#include "version.h"

#include "../../include/utils/ParseArgs.hpp"
#include "../../include/utils/utils.hpp"

#include "../../include/swim/SwimClient.hpp"
#include "../../include/swim/SwimServer.hpp"

// TODO: find Posix equivalent of GNU `program_invocation_short_name`
#ifdef __linux__
#define PROG_NAME program_invocation_short_name
#else
#define PROG_NAME "swim_server_demo"
#endif


using namespace zmq;
using namespace swim;


namespace {

/**
 * Prints out usage instructions for this application.
 */
void usage() {
  std::cout << "Usage: " << PROG_NAME << " --port=PORT [--host=HOST] \n"
            << "\t\t[--timeout=TIMEOUT] [--duration=DURATION] ACTION\n"
            << "\t\t[--debug] [--version] [--help]\n\n"
            << "\t--debug    verbose output (LOG_v = 2)\n"
            << "\t--help     prints this message and exits\n"
            << "\t--version  prints the version string for this demo and third-party libraries "
               "and exits\n\n"
            << "\tPORT       an int specifying the port the server will listen on (`receive`), or\n"
            << "\t           connect to (`send`);\n"
            << "\tHOST       the hostname/IP to send the status to (e.g., h123.example.org or \n"
            << "\t           192.168.1.1).  Required for sending, ignored otherwise.\n"
            << "\tTIMEOUT    in milliseconds, how long to wait for the server to respond to the\n"
            << "\t           ping\n"
            << "\tDURATION   in seconds, how long the client (`send`) should run\n"
            << "\tACTION     one of `send` or `receive`; if the former, also specifiy the host to\n"
            << "\t           send the data to.\n\n"
            << "HOST is only required when sending.\n\n";
}

std::atomic<bool> stopped(false);

} // namespace


void start_timer(unsigned long duration_sec) {
  std::thread t([=] {
    std::this_thread::sleep_for(std::chrono::seconds(duration_sec));
    stopped.store(true);
  });
  t.detach();
}


int runClient(
    const std::string& host,
    int port,
    const std::string& name,
    unsigned long timeout,
    unsigned long duration
) {
  LOG(INFO) << "Running for " << duration << " seconds; timeout: " << timeout << " msec.";

  auto server = MakeServer(host, port);
  SwimClient client(*server, 0, timeout);
  auto client_svr = MakeServer(name, client.self().port());
  client.setSelf(*client_svr);

  std::chrono::milliseconds wait(1500);
  start_timer(duration);
  while (!stopped.load()) {
    if (!client.Ping()) {
      LOG(ERROR) << "Could not ping server " << host;
      return EXIT_FAILURE;
    }
    std::this_thread::sleep_for(wait);
  }
  return EXIT_SUCCESS;
}


int main(int argc, const char *argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = true;

  ::utils::ParseArgs parser(argv, argc);
  if (parser.enabled("debug")) {
    FLAGS_v = 2;
  }

  if (parser.has("help")) {
    usage();
    return EXIT_SUCCESS;
  }

  ::utils::PrintVersion("Server Demo", RELEASE_STR);
  if (parser.has("version")) {
    return EXIT_SUCCESS;
  }

  int port = parser.getInt("port", 6060);
  if (port < 0 || port > 65535) {
    LOG(ERROR) << "Port number must be a positive integer, less than 65,535. "
               << "Found: " << port;
    return EXIT_FAILURE;
  }


  if (parser.size() != 1) {
    LOG(ERROR) << "Please specify an ACTION ('send' or 'receive')";
    return EXIT_FAILURE;
  }
  std::string action = parser[0];

  if (action == "send") {
    std::string host = parser["host"];
    if (host.empty()) {
      LOG(ERROR) << "Missing required --host option. Please specify a server to send the status to";
      return EXIT_FAILURE;
    }

    std::string name = parser.get("name", "client");

    runClient(host, port, name,
              static_cast<unsigned long>(parser.getInt("timeout", 200)),
              static_cast<unsigned long>(parser.getInt("duration", 5)));

  } else if (action == "receive") {
    SwimServer server(port);
    // TODO: this should run in a separate thread instead, and we just join() on the timer thread.
    server.start();
  } else {
    usage();
    LOG(ERROR) << "One of {send, recevive} expected; we got instead: '" << parser[0] << "'";
    return EXIT_FAILURE;
  }

  LOG(INFO) << "done";
  return EXIT_SUCCESS;
}
