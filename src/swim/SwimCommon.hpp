// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 4/10/17.


#pragma once

#include <chrono>
#include <iomanip>
#include <set>

#include "swim.pb.h"


using std::set;
using namespace std::chrono;
using Timestamp = system_clock::time_point;

namespace swim {

inline ::google::protobuf::uint64 TimestampToFixed64(const Timestamp &timestamp) {
  return std::chrono::system_clock::to_time_t(timestamp);
}

/**
 * Thrown when the set from which the caller is trying to obtain an element is empty.
 */
struct empty_set : public std::exception {
  virtual const char* what() { return "empty set"; };
};

extern std::default_random_engine random_engine;


/**
 * Creates a Protobuf representation of the given hostname:port server.
 *
 * @param hostname the server's name, if known (or its IP address).
 * @param port the listening port
 * @param ip the IP address (optional, may be useful if the name is not DNS-resolvable)
 * @return the PB representation of this server
 */
inline std::unique_ptr<Server> MakeServer(
    const std::string &hostname,
    int port,
    const std::string &ip = "") {

  std::unique_ptr<Server> server(new Server());
  server->set_hostname(hostname);
  server->set_port(port);
  // TODO: check that `ip` is a valid IP address (syntactically).
  if (ip.length() > 0) {
    server->set_ip_addr(ip);
  }
  return server;
}


inline std::unique_ptr<ServerRecord> MakeRecord(
    const Server &server,
    const Timestamp &timestamp = std::chrono::system_clock::now()) {

  std::unique_ptr<ServerRecord> record(new ServerRecord());
  Server *psvr = record->mutable_server();
  psvr->set_hostname(server.hostname());
  psvr->set_port(server.port());
  psvr->set_ip_addr(server.ip_addr());

  record->set_timestamp(TimestampToFixed64(timestamp));
  record->set_didgossip(false);

  return record;
}


/**
 * Ordering operator for server records, uses the `hostname`  as a total ordering
 * criterion.  For servers on the same host (IP), the port number is used; the timestamp is
 * not used as a sorting criterion.
 *
 * <p>The sorting by hostname is done solely lexicographically, no meaning is assigned
 * to the either the name or the IP address (if present); it is simply a means to allow us to store
 * `ServerRecord` objects into a `set`.
 *
 * @param lhs the left-hand side ServerRecord for the comparison
 * @param rhs the right-hand side of the comparison
 * @return whether `lhs` is less than `rhs`, according to a lexicographic ordering of their
 *      respective `ServerRecord#server()`
 */
bool operator<(const ServerRecord &lhs, const ServerRecord &rhs);


/**
 * Equality operator for servers; we assume we are talking to the same server if it has the same
 * hostname and the same port.  The IP address is not used.
 *
 * @param lhs the left-hand operand
 * @param rhs the right-hand operand
 * @return `true` if both `hostname` and `port` match
 */
bool operator==(const Server &lhs, const Server &rhs);

/**
 * Without this, associative containers (such as `set`) would compare the value of the pointers
 * (as opposed to the actual value of the records) for insertion and/or retrieval.
 *
 * <p>See Item 20 of "Effective STL", Scott Meyers.
 */
struct ServerRecordPtrLess :
    public std::binary_function<
        const std::shared_ptr<ServerRecord> &,
        const std::shared_ptr<ServerRecord> &,
        bool> {

  /**
   * Comparison operator for the "less than" predicate (associative containers semantics) for
   * containers of pointers.
   *
   * @param lhs the left-hand side of the operation
   * @param rhs the right-hand side of the operation
   * @return whether the record pointed to by `lhs` is "less than" the one pointed to by `rhs`
   */
  bool operator()(const std::shared_ptr<ServerRecord> &lhs,
                  const std::shared_ptr<ServerRecord> &rhs) {
    return *lhs < *rhs;
  }
};


typedef set<std::shared_ptr<ServerRecord>, ServerRecordPtrLess> ServerRecordsSet;


inline std::ostream &operator<<(std::ostream &out, const Server &server) {
  out << "'" << server.hostname() << ":" << server.port() << "'";

  if (server.has_ip_addr() && server.ip_addr().size() > 0) {
    out << " [" << server.ip_addr() << "]";
  }
  return out;
}

inline std::ostream &operator<<(std::ostream &out, const std::shared_ptr<Server> &ps) {
  out << (*ps);
  return out;
}

inline std::ostream &operator<<(std::ostream &out, const ServerRecord &record) {
  long ts = record.timestamp();

  out << "[" << record.server() << " at: "
      << std::put_time(std::gmtime(&ts), "%c %Z");

  if (record.has_forwarder()) {
    out << "; forwarded by: " << record.forwarder();
  }
  out << "]";

  return out;
}

inline std::ostream& operator<<(std::ostream& out, const ServerRecordsSet& recordsSet) {
  out << "{ ";
  for (auto record : recordsSet) {
    out << *record << ", ";
  }

  if (!recordsSet.empty()) {
    out << "\b\b";
  }
  out << " }";
  return out;
}

inline std::ostream &operator<<(std::ostream &out, const SwimReport &report) {
  out << "Report from: " << report.sender();
  out << "\n=================================\nHealthy servers\n--------------------------\n";
  for (auto healthy : report.alive()) {
    out << healthy << std::endl;
  }
  out << "\nUnresponsive servers\n--------------------------\n";
  for (auto suspect : report.suspected()) {
    out << suspect << std::endl;
  }
  out << "\n=================================\n";

  return out;
}
} // namespace swim
