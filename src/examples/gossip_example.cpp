// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.


#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <thread>

#include <glog/logging.h>

#include <zmq.hpp>

#include "swim.pb.h"

#include "swim/GossipFailureDetector.hpp"

#include "utils/ParseArgs.hpp"
#include "utils/utils.hpp"


using namespace std;
using namespace swim;
using namespace zmq;


namespace {

const unsigned int kDefaultPort = 30395;
const unsigned int kDefaultHttpPort = 30396;

// Default values for all of the GossipFailureDetector intervals/durations.
// These defaults are for the example app usage only
const unsigned int kDefaultGracePeriodSec = 35;
const unsigned int kDefaultPingIntervalSec = 5;


/**
 * Prints out usage instructions for this application.
 */
void usage() {
  std::cout << "Usage: " << program_invocation_short_name << " --seeds=SEEDS_LIST [--port=PORT]\n"
    << "\t\t[--timeout=TIMEOUT] [--ping=PING_SEC] [--http [--http-port=HTTP_PORT]]\n"
    << "\t\t[--grace-period=GRACE_PERIOD]\n"
    << "\t\t[--debug] [--version] [--help]\n\n"
    << "\t--debug       verbose output (LOG_v = 2)\n"
    << "\t--trace       trace output (LOG_v = 3)\n"
    << "\t              Using either option will also cause the logs to be emitted to stdout,\n"
    << "\t              otherwise the default Google Logs logging directory/files will be used.\n\n"
    << "\t--help        prints this message and exits;\n"
    << "\t--version     prints the version string for this demo and third-party libraries\n"
    << "\t              and exits\n"
    << "\t--http        whether this server should expose a REST API (true by default,\n"
    << "\t              use --no-http to disable);\n\n"
    << "\tPORT          an integer value specifying the TCP port the server will listen on,\n"
    << "\t              if not specified, uses " << kDefaultPort << " by default;\n"
    << "\tHTTP_PORT     the HTTP port for the REST API, if server exposes it (see --http);\n"
    << "\t              if not specified, uses " << kDefaultHttpPort << " by default;\n"
    << "\tTIMEOUT       in milliseconds, how long to wait for the server to respond to the ping\n"
    << "\tGRACE_PERIOD  in seconds, how long to wait before evicting suspected servers\n"
    << "\tPING_SEC      interval, in seconds, between pings to servers in the Gossip Circle\n"
    << "\tSEEDS_LIST    a comma-separated list of host:port of peers that this server will\n"
    << "\t              initially connect to, and part of the Gossip ring: from these \"seeds\"\n"
    << "\t              the server will learn eventually of ALL the other servers and\n"
    << "\t              connect to them too.\n"
    << "\t              The `host` part may be either an IP address (such as 192.168.1.1) or\n"
    << "\t              the DNS-resolvable `hostname`; for example:\n\n"
    << "\t                192.168.1.101:8080,192.168.1.102:8081\n"
    << "\t                node1.example.com:9999,node1.example.com:9999,node3.example.com:9999\n\n"
    << "\t              Both host and port are required and no spaces must be left\n"
    << "\t              between entries; the hosts may not ALL be active.\n\n"
    << "\tThe server will run forever in foreground, use Ctrl-C to terminate.\n";
  utils::PrintVersion();
}
} // namespace


int main(int argc, const char *argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  google::InitGoogleLogging(argv[0]);

  ::utils::ParseArgs parser(argv, argc);
  if (parser.enabled("debug") || parser.enabled("trace")) {
    FLAGS_logtostderr = true;
    FLAGS_v = parser.enabled("debug") ? 2 : 3;
  }

  if (parser.has("help")) {
    usage();
    return EXIT_SUCCESS;
  }

  utils::PrintVersion();
  if (parser.has("version")) {
    return EXIT_SUCCESS;
  }

  try {
    int port = parser.getInt("port", ::kDefaultPort);
    if (port < 0 || port > 65535) {
      LOG(ERROR) << "Port number must be a positive integer, less than 65,535. "
                 << "Found: " << port;
      return EXIT_FAILURE;
    }
    LOG(INFO) << "Gossip Detector listening on incoming TCP port " << port;

    if (parser.enabled("http")) {
      int httpPort = parser.getInt("http-port", ::kDefaultHttpPort);
      LOG(INFO) << "Enabling HTTP REST API on port " << httpPort;
      throw utils::not_implemented("HTTP Server");
    } else {
      LOG(INFO) << "REST API will not be available";
    }

    long ping_timeout_msec = parser.getInt("timeout", swim::kDefaultTimeoutMsec);
    long ping_interval_sec = parser.getInt("ping", kDefaultPingIntervalSec);
    long grace_period_sec = parser.getInt("grace-period", kDefaultGracePeriodSec);

    auto detector = std::make_shared<GossipFailureDetector>(
        port,
        ping_interval_sec,
        grace_period_sec,
        ping_timeout_msec);

    if (!parser.has("seeds")) {
      usage();
      LOG(ERROR) << "A list of peers (possibly just one) is required to start the Gossip Detector";
      return EXIT_FAILURE;
    }


    auto seedNames = ::utils::split(parser.get("seeds"));
    LOG(INFO) << "Connecting to initial Gossip Circle: "
              << ::utils::Vec2Str(seedNames, ", ");

    std::for_each(seedNames.begin(), seedNames.end(),
                   [&](const std::string &name) {
                     try {
                       auto ipPort = ::utils::ParseHostPort(name);
                       string ip;
                       string host{std::get<0>(ipPort)};
                       if (::utils::ParseIp(host)) {
                         ip = host;
                       } else {
                         ip = ::utils::InetAddress(host);
                       }
                       Server server;
                       server.set_hostname(host);
                       server.set_port(std::get<1>(ipPort));
                       server.set_ip_addr(ip);

                       LOG(INFO) << "Adding neighbor: " << server;
                       detector->AddNeighbor(server);

                     } catch (::utils::parse_error& e) {
                       LOG(ERROR) << "Could not parse '" << name << "': " << e.what();
                     }
                   }
    );

    LOG(INFO) << "Waiting for server to start...";
    int waitCycles = 10;
    while (!detector->gossip_server().isRunning() && waitCycles-- > 0) {
      std::this_thread::sleep_for(seconds(1));
    }

    LOG(INFO) << "Gossip Server " << detector->gossip_server().self()
              << " is running. Starting all background threads";
    detector->InitAllBackgroundThreads();

    LOG(INFO) << "Threads started; detector process running"; // TODO: << PID?

    // TODO: "trap" the KILL and execute a graceful exit
    while (true) {
      std::this_thread::sleep_for(seconds(10));
      std::cout << "~~~>>>>> S T A T U S   R E P O R T <<<<<~~~" << std::endl;
      std::cout << detector->gossip_server().PrepareReport() << std::endl;
      std::cout << "~~~ >>>> ~~~~~~~~~~~~~~~~~~~~~~~~~ <<<< ~~~" << std::endl;
    }

  } catch (::utils::parse_error& error) {
    LOG(ERROR) << "A parsing error occurred: " << error.what();
    return EXIT_FAILURE;
  }



  return EXIT_SUCCESS;
}
