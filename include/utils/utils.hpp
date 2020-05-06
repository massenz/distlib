// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 9/17/17.

#pragma once

#include <arpa/inet.h>
#include <ctime>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <regex>
#include <sstream>
#include <vector>

#include <glog/logging.h>


#include "version.h"


namespace utils {

using std::string;

/********************* Exception Classes ************************************/

/**
 * Base class for all exceptions in this library.
 */
class base_error : public std::exception {
 protected:
  std::string what_;

 public:
  explicit base_error(string  error) : what_(std::move(error)) {}

#if defined( __linux__ )
  virtual const char* what() const _GLIBCXX_NOTHROW {
#elif defined( __APPLE__ )
    virtual const char* what() const _NOEXCEPT {
#endif
    return what_.c_str();
  }
};

/**
 * Marker exception for unimplemented methods.
 */
class not_implemented : public base_error {
public:
  explicit not_implemented(const std::string &method_or_class) :
      base_error { method_or_class + " not implemented" } { }
};


/********************* Hashing Functions *************************************/

/**
 * Converts a char buffer into a hex string.
 *
 * It expects the ``digest`` buffer to contain exactly ``MD5_DIGEST_LENGTH``
 * bytes, that will be converted to hex notation and appended to the returned
 * string.
 *
 * @param digest The MD5 digest to convert into a string; exactly
 *    `MD5_DIGEST_LENGTH` bytes in length.
 * @return the hex-encoded string representation of the `digest`.
 */
std::string md5_to_string(const unsigned char *digest);


/**
 * Computes the hash of the given ``value`` and puts into the destination buffer,
 * ``hash_value``, whose length is returned.
 *
 * The buffer is newly allocated and it is the caller's responsibility to deallocate it
 * when done.
 *
 * @param value the value to hash.
 * @param len the length of the value to hash.
 * @param hash_value the digest from the hash, newly allocated.
 *
 * @return the size of the digest buffer.
 */
size_t basic_hash(const char* value, size_t len, unsigned char** hash_value);
/**
 * Hashes the given string using MD5 and returns the hash.
 *
 * @param msg The string to hash.
 * @return the MD5 hash of `msg`.
 */
std::string hash_str(const std::string &msg);


/**
 * Convenience method, can be used by projects using this library to emit their version
 * string, alongside the library version.
 *
 * @param server_name the name for the server that uses this library
 * @param version the server's version
 * @param out the stream to write out the version information (by default, `stdout`)
 * @return the same stream that was passed in, for ease of chaining
 */
std::ostream& PrintVersion(const string& server_name,
                           const string& version, std::ostream &out = std::cout);

/**
 * Converts a vector to a string, the concatenation of its element, separated by `sep`.
 * Only really useful for debugging purposes.
 *
 * @tparam T the type of the elements, MUST support emitting to an `ostream` via the `<<` operator
 * @param vec the vector whose element we are emitting
 * @param sep a separator string, by default a newline
 * @return the string resulting from the concatenation of all elements
 */
template<typename T>
std::string Vec2Str(const std::vector<T> &vec, const std::string &sep = "\n") {
  bool first = true;
  std::ostringstream out;

  for (auto t : vec) {
    if (!first) out << sep; else first = false;
    out << t;
  }
  return out.str();
}

/********************* Network Utilities *************************************/


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
 * @return a string suitable for use with ZMQ `socket()` call.
 *      Or an empty string, if no suitable socket is available.
 */
std::string SocketAddress(unsigned int port);

/**
 * Returns the current time in a format suitable for use as a timestamp field
 * in a Protobuf.
 *
 * @return the current time (via `std::time()`).
 */
google::uint64 CurrentTime();

/**
 * Tries to find this node's hostname, as returned by the OS.
 *
 * @return the hostname, if available.
 */
std::string Hostname();
} // namespace utils
