// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/5/16.

#include <iostream>
#include <list>


#include <glog/logging.h>

#include "version.h"

#include "../../include/ConsistentHash.hpp"
#include "../../include/MerkleNode.hpp"


using std::cin;
using std::cout;
using std::endl;
using std::list;
using std::string;


static unsigned int kNumNodes = 33;


void usage(const std::string &prog) {

  cout << "Merkle Tree & Consistent Hash Demo - LibDist ver. " << RELEASE_STR
       << "\n\nUsage: " << prog.c_str() << " string-to-hash [nodes]" << endl;
}


int main(int argc, char *argv[]) {

  list<string> nodes;
  const MD5StringMerkleNode* root;

  // Initialize Google Logs and send logs to stderr too (in addition
  // to the default file location; default: /tmp).
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = 1;

  if (argc < 2) {
    usage(argv[0]);
    LOG(ERROR) << "Missing one required argument `string-to-hash`";
    exit(EXIT_FAILURE);
  }

  std::string mesg(argv[1]);

  LOG(INFO) << "'" << mesg << "' hashes to [" << hash_str(mesg) << "]";
  LOG(INFO) << "Its consistent hash is: " << consistent_hash(mesg);

  unsigned int num_nodes = kNumNodes;
  if (argc > 2) {
    num_nodes = static_cast<unsigned int>(atoi(argv[2]));
  }

  LOG(INFO) << "Building a Merkle Tree with " << num_nodes << " nodes";
  for (int i = 0; i < num_nodes; ++i) {
    nodes.push_back("node #" + std::to_string(i));
  }

  root = build<std::string, MD5StringMerkleNode>(nodes);

  bool valid = root->validate();
  LOG(INFO) << "The tree is " << (valid ? "" : "not ") << "valid" << endl;
  delete(root);

  return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
