// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.

#include <string>

#include "ConsistentHash.hpp"


float consistent_hash(const std::string &msg) {
  unsigned char* digest;
  unsigned long sum = 0;

  utils::basic_hash(msg.c_str(), strlen(msg.c_str()), &digest);
  for (int i = 0; i < MD5_DIGEST_LENGTH - 1; i += 2) {
    sum += kBase * (unsigned long) (digest[i] + digest[i+1] * 16);
  }
  delete[](digest);

  return float(sum % kModulo) / kModulo;
}
