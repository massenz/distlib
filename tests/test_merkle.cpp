// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/20/16.


#include <gtest/gtest.h>

#include "brick.hpp"
#include <MerkleNode.hpp>

using namespace std;

TEST(MerkleNodeTests, CanCreate) {
  std::string s("leaves in spring");

  MerkleNode<string> leaf(s, s.length());
  ASSERT_TRUE(leaf.validate());
}

TEST(MerkleNodeTests, CanCreateTwoLeaves) {
  std::string sl[] = { "leaves", "winter" };
  MerkleNode<string>* leaves[2];

  for (int i = 0; i < 2; ++i) {
    leaves[i] = new MerkleNode<string>(sl[i], sl[i].length());
  }

  MerkleNode<string> root(*leaves[0], *leaves[1]);
  EXPECT_FALSE(std::get<0>(root.getChildren()) == nullptr);
  EXPECT_TRUE(root.validate());
}

TEST(MerkleNodeTests, Equals) {
  MerkleNode<string> a(hash_str("test"), MD5_DIGEST_LENGTH);
  MerkleNode<string> b(hash_str("test"), MD5_DIGEST_LENGTH);

  // First, must be equal to itself.
  EXPECT_EQ(a, a);

  // Then equal to an identical constructed other.
  EXPECT_EQ(a, b);

  // Then, non-equal to a different one.
  MerkleNode<string> c(hash_str("not a test"), MD5_DIGEST_LENGTH);
  EXPECT_NE(a, c);
}
