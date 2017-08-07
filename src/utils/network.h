// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/9/16.

#pragma once

#include <ctime>
#include <memory>
#include <netdb.h>
#include <sstream>

#include <glog/logging.h>
#include <google/protobuf/stubs/common.h>


namespace utils {

/**
 * Returns the IP address of the host whose name is `hostname`; if `hostname` is empty, the IP
 * address of this server will be returned.
 *
 * @param hostname the host whose IP address we want to resolve.
 * @return the IP address, in human-readable format.
 */
std::string InetAddress(const std::string& hostname = "");


/**
 * Given a port number will return a suitable socket address in string
 * format for the server to be listening on (e.g., "tcp://0.0.0.0:5000").
 *
 * @param port the host port the server would like to listen to
 * @return a string suitable for use with ZMQ {@link socket_t socket()} call.
 *      Or an empty string, if no suitable socket is available.
 */
std::string SocketAddress(unsigned int port);

/**
 * Returns the current time in a format suitable for use as a timestamp field
 * in a Protobuf.
 *
 * @return the current time (via `std::time()`).
 */
google::protobuf::int64 CurrentTime();

/**
 * Tries to find this node's hostname, as returned by the OS.
 *
 * @return the hostname, if available.
 */
std::string Hostname();

/**
 * Prints this library's (and supporting third-party libraries') version.
 */
void printVersion();

} // namespace utils
