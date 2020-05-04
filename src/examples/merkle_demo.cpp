// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/5/16.

#include <iostream>
#include <list>
#include <regex>

#include <glog/logging.h>

#include "version.h"

#include "ConsistentHash.hpp"
#include "MerkleNode.hpp"

using namespace std;

namespace merkle {
namespace demo {

// Simply concatenates the two buffers.
string ConcatHashes(const shared_ptr<string> &psl,
                    const shared_ptr<string> &psr) noexcept {
  if (!(psl || psr)) return "";
  return !psl ? *psr : !psr ? *psl : *psl + *psr;
}

/**
 * An implementation of a Merkle Tree with the nodes' elements ``strings`` and their hashes
 * computed using the MD5 algorithm.
 *
 * This is an example of how one can build a Merkle tree given a concrete type and an arbitrary
 * hashing function.
 */
using MD5StringMerkleNode = MerkleNode<std::string, std::string, ::hash_str, ConcatHashes>;

// Specialization of the generic merkle::Build() method:
std::unique_ptr<MD5StringMerkleNode>
BuildMD5StringMerkleTree(const vector<string> &values) {
  return merkle::Build<string, string, ::hash_str, ConcatHashes>(values);
}

} // namespace demo
} // namespace merkle

static unsigned int kNumNodes = 33;

void usage(const std::string &arg) {
  regex progname { "/?(\\w+)$" };
  smatch matches;
  string prog { arg };

  if (regex_search(arg, matches, progname)) {
    prog = matches[0];
  }
  cout << "Usage: " << prog << " string-to-hash [nodes]" << endl;
}

void headline() {
  cout << "Merkle Tree & Consistent Hash Demo - LibDist ver. " << RELEASE_STR << endl << endl;
}

int main(int argc, char *argv[]) {

  headline();
  if (argc < 2) {
    usage(argv[0]);
    std::cerr << "[ERROR] Missing required argument `string-to-hash`" << endl;
    exit(EXIT_FAILURE);
  }

  std::string mesg { argv[1] };
  cout << "'" << mesg << "' hashes to [" << hash_str(mesg) << "]" << endl
       << "Its consistent hash is: " << consistent_hash(mesg) << endl;

  unsigned int num_nodes = kNumNodes;
  if (argc > 2) {
    num_nodes = static_cast<unsigned int>(::strtoul(argv[2], nullptr, 10));
  }

  cout << "Building a Merkle Tree with " << num_nodes << " nodes" << endl;
  vector<string> nodes;
  nodes.reserve(num_nodes);
  for (int i = 0; i < num_nodes; ++i) {
    nodes.push_back("node #" + std::to_string(i));
  }

  auto root = merkle::demo::BuildMD5StringMerkleTree(nodes);
  cout << "The tree is " << (root->IsValid() ? "" : "not ") << "valid" << endl
       << "Its contents are: \n" << root.get() << endl << endl;

  return root->IsValid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
