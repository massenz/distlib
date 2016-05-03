// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/20/16.


#include <gtest/gtest.h>

#include "brick.hpp"
#include <MerkleNode.hpp>

using namespace std;

TEST(MerkleNodeTests, CanCreate) {
  std::string s("leaves in spring");

  MD5StringMerkleNode leaf(s);
  ASSERT_TRUE(leaf.validate());
}

TEST(MerkleNodeTests, CanCreateTree) {
  std::string sl[] = { "leaves", "winter", "spring", "flowers" };
  MD5StringMerkleNode* leaves[4];

  for (int i = 0; i < 4; ++i) {
    leaves[i] = new MD5StringMerkleNode(sl[i]);
  }

  MD5StringMerkleNode a(*leaves[0], *leaves[1]);
  MD5StringMerkleNode b(*leaves[2], *leaves[3]);
  MD5StringMerkleNode root(a, b);

  EXPECT_TRUE(root.hasChildren());
  EXPECT_FALSE(std::get<0>(b.getChildren()) == nullptr);
  EXPECT_TRUE(root.validate());
}

TEST(MerkleNodeTests, hasChildren) {
  MD5StringMerkleNode a("test");
  EXPECT_FALSE(a.hasChildren());

  MD5StringMerkleNode b("another node");
  MD5StringMerkleNode parent(a, b);
  EXPECT_TRUE(parent.hasChildren());
}

//TEST(MerkleNodeTests, Equals) {
//  MD5StringMerkleNode a("test");
//  MD5StringMerkleNode b("test");
//
//  // First, must be equal to itself.
//  EXPECT_EQ(a, a);
//
//  // Then equal to an identical constructed other.
//  EXPECT_EQ(a, b);
//
//  // Then, non-equal to a different one.
//  MD5StringMerkleNode c("not a test");
//  EXPECT_NE(a, c);
//}
