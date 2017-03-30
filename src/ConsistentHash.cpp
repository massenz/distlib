// Copyright (c) 2016 Marco Massenzio.  All rights reserved.

#include "ConsistentHash.hpp"

#include <iostream>
#include <openssl/md5.h>
#include <string.h>


using std::string;


/**
 * Used to compute the consistent hash: this is the base
 * for each digit pair.
 */
unsigned long BASE = 13;

/** The modulo for the consistent hash. */
unsigned long MODULO = 32497;


string md5_to_string(const unsigned char *digest) {
  char buf[MD5_DIGEST_LENGTH * 2 + 1];
  for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf(&buf[2 * i], "%02x", digest[i]);
  }
  return string(buf);
}


string hash_str(const string &msg) {

  unsigned char* digest;

  basic_hash(msg.c_str(), strlen(msg.c_str()), &digest);
  string s = md5_to_string(digest);
  delete[](digest);

  return s;
}


float consistent_hash(const string &msg) {
  unsigned char* digest;
  unsigned long sum = 0;
  unsigned long base = 1;

  basic_hash(msg.c_str(), strlen(msg.c_str()), &digest);
  for (int i = 0; i < MD5_DIGEST_LENGTH - 1; i += 2) {
    sum += base * (unsigned long) (digest[i] + digest[i+1] * 16);
    base *= BASE;
  }
  delete[](digest);

  return float(sum % MODULO) / MODULO;
}


size_t basic_hash(const char* value, size_t len, unsigned char** hash_value) {
  *hash_value = new unsigned char[MD5_DIGEST_LENGTH];
  MD5((unsigned char *)value, len, *hash_value);

  return MD5_DIGEST_LENGTH;
}
