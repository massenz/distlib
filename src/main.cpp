// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/5/16.

#include <iostream>
#include <list>

#include <glog/logging.h>

#include "brick.hpp"
#include "MerkleNode.hpp"


using std::cin;
using std::cout;
using std::endl;
using std::list;
using std::string;


void usage(const std::string &prog) {
  LOG(ERROR) << "Usage: " << prog << " string-to-hash";
  exit(1);
}


int main(int argc, char *argv[]) {

  list<string> nodes;
  const MD5StringMerkleNode* root;

  // Initialize Google Logs and send logs to stderr too (in addition
  // to the default file location; default: /tmp).
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = 1;

  if (argc == 1) {
    usage(argv[0]);
  }

  std::string mesg(argv[1]);

  LOG(INFO) << "'" << mesg << "' hashes to [" << hash_str(mesg) << "]";
  LOG(INFO) << "Its consistent hash is: " << consistent_hash(mesg);

  for (int i = 0; i < 33; ++i) {
    nodes.push_back("node #" + std::to_string(i));
  }

  root = build<std::string, MD5StringMerkleNode>(nodes);

  bool valid = root->validate();
  LOG(INFO) << "The tree is " << (valid ? "" : "not ") << "valid" << endl;
  delete(root);

  return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
