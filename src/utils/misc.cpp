// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 8/6/17.

#include <openssl/md5.h>
#include <chrono>

#include "utils/utils.hpp"

namespace utils {

std::ostream& PrintVersion(const string& server_name,
                           const string& version,
                           std::ostream &out) {
  out << server_name << " Ver. " << version
      << " (libdist ver. " << RELEASE_STR << ")" << std::endl;
  return out;
}

std::ostream& PrintCurrentTime(std::ostream &out) {
  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  out << std::ctime(&now);
  return out;
}



string md5_to_string(const unsigned char *digest) {
  char buf[MD5_DIGEST_LENGTH * 2 + 1];
  for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf(&buf[2 * i], "%02x", digest[i]);
  }
  return string(buf);
}


size_t basic_hash(const char* value, size_t len, unsigned char** hash_value) {
  *hash_value = new unsigned char[MD5_DIGEST_LENGTH];
  MD5((unsigned char *)value, len, *hash_value);

  return MD5_DIGEST_LENGTH;
}


string hash_str(const string &msg) {

  unsigned char* digest;

  basic_hash(msg.c_str(), strlen(msg.c_str()), &digest);
  string s = md5_to_string(digest);
  delete[](digest);

  return s;
}
} // namespace utils
