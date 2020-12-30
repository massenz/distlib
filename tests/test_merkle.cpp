// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/20/16.

// Ignore CLion warning caused by GTest TEST() macro.
#pragma ide diagnostic ignored "cert-err58-cpp"


#include <gtest/gtest.h>

#include "MerkleNode.hpp"

using namespace std;
using namespace merkle;

int IdentityHash(const int& n) { return n; }
int ConcatHash(const shared_ptr<int> &psl,
    const shared_ptr<int> &psr) {
  return IdentityHash(*psl + *psr);
}

using MerkleIntNode =
    merkle::MerkleNode<int, int, IdentityHash, ConcatHash>;

TEST(MerkleNodeTests, CanCreate) {
//  std::string s("leaves in spring");

  MerkleIntNode leaf(999);
  ASSERT_TRUE(leaf.IsLeaf());
  ASSERT_TRUE(leaf.IsValid());
}

TEST(MerkleNodeTests, CanCreateIntermediate) {
//  std::string s("leaves in spring");

  MerkleIntNode leaf1(999);
  MerkleIntNode leaf2(777);
  MerkleIntNode root(&leaf1, &leaf2);

  ASSERT_FALSE(root.IsLeaf());
  ASSERT_TRUE(root.IsValid());

  auto values = GetAllValues(&root);

  // Release the pointers before root's destructor tries to free stack-allocated memory
  root.release_left();
  root.release_right();
}

TEST(MerkleNodeTests, CanCreateTree) {
  std::vector<int> sl {100, 101, 102, 103};

  auto root = merkle::Build<int, int, IdentityHash, ConcatHash>(sl);

  ASSERT_NE(root, nullptr);
  EXPECT_TRUE(root->IsValid());
  EXPECT_FALSE(root->IsLeaf());
}


TEST(MerkleNodeTests, CanNavigateTree) {
  std::vector<int> sl {100, 101, 102, 103};

  auto root = merkle::Build<int, int, IdentityHash, ConcatHash>(sl);

  ASSERT_NE(root, nullptr);
  EXPECT_TRUE(root->IsValid());

  auto values = merkle::GetAllValues(root.get());
  ASSERT_EQ(4, values.size());

  auto pos = values.begin();
  ASSERT_EQ(100, *pos);
  pos++; // 101
  pos++; // 102
  ASSERT_EQ(102, *pos);
}

// Some tests with a more challenging hash.

// Simply concatenates the two buffers.
string HashOfHashes(const shared_ptr<string> &psl,
                    const shared_ptr<string> &psr) {
  if (!(psl || psr)) return "";
  return !psl ? *psr : !psr ? *psl : *psl + *psr;
}

TEST(MerkleNodeTests, AssertInvariant) {
  std::shared_ptr<string> myHash  = std::make_shared<string>( utils::hash_str("a test string") );
  VLOG(2) << "The Hash is: " << *myHash;

  ASSERT_EQ(HashOfHashes(myHash, shared_ptr<string> {}),
      HashOfHashes(shared_ptr<string> {}, myHash));
}

TEST(MerkleNodeTests, CanCreateStringsTree) {
  std::vector<string> sl { "first", "second", "third", "fourth", "fifth" };

  auto root = merkle::Build<std::string, std::string, utils::hash_str, HashOfHashes>(sl);

  ASSERT_NE(root, nullptr);
  EXPECT_TRUE(root->IsValid());
  EXPECT_FALSE(root->IsLeaf());
}


TEST(MerkleNodeTests, CanNavigateStringsTree) {
  std::vector<string> sl { "first", "second", "third", "fourth", "fifth" };
  auto root = merkle::Build<std::string, std::string, utils::hash_str, HashOfHashes>(sl);

  ASSERT_NE(root, nullptr);
  EXPECT_TRUE(root->IsValid());

  auto values = merkle::GetAllValues(root.get());
  ASSERT_EQ(5, values.size());

  auto pos = values.begin();
  ASSERT_EQ("first", *pos);
  pos++; // second
  pos++; // third
  pos++; // fourth
  ASSERT_EQ("fourth", *pos);
}




