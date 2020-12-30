// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.

#include <string>

#include "ConsistentHash.hpp"

using std::string;


/**
 * Used to compute the consistent hash: this is the base
 * for each digit pair.
 */
unsigned long BASE = 13;

/** The modulo for the consistent hash. */
unsigned long MODULO = 32497;


float consistent_hash(const string &msg) {
  unsigned char* digest;
  unsigned long sum = 0;
  unsigned long base = 1;

  utils::basic_hash(msg.c_str(), strlen(msg.c_str()), &digest);
  for (int i = 0; i < MD5_DIGEST_LENGTH - 1; i += 2) {
    sum += base * (unsigned long) (digest[i] + digest[i+1] * 16);
    base *= BASE;
  }
  delete[](digest);

  return float(sum % MODULO) / MODULO;
}
