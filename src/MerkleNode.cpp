// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/20/16.


#include "MerkleNode.hpp"

#include <algorithm>
#include <cassert>


//template<typename T>
//MerkleNode<T>::MerkleNode(const MerkleNode<T> &left, const MerkleNode<T> &right) :
//    left_(&left), right_(&right) {
//
//  std::cout << "Constructing a MerkleNode... " << std::endl;
//  assert(left.hash_len_ == right.hash_len_);
//  hash_len_ = left.hash_len();
//  hash_ = computeHash();
//
//}


// TODO(marco): this must be generalized too, using a hashing_function template argument.
//template<typename T>
//T MerkleNode<T>::computeHash() const {
//  if (!left_ && !right_) {
//    return hash_;
//  }
//
//  // If either child is empty, just double up the other one.
//  const T &tmp_l = left_ ? left_->hash_ : right_->hash_;
//  const T &tmp_r = right_ ? right_->hash_ : left_->hash_;
//
//  return hash(tmp_l, tmp_r);
//}

//template<typename T>
//bool MerkleNode<T>::validate() const {
//  // If a leaf node, it's valid by definition.
//  if (!left_ && !right_) {
//    return true;
//  }
//
//  if (left_ && !left_->validate()) {
//    return false;
//  }
//  if (right_ && !right_->validate()) {
//    return false;
//  }
//
//  return hash_ == computeHash();
//}

template<typename T>
bool operator==(const MerkleNode<T> &lhs, const MerkleNode<T> &rhs) {
  // First the trivial case of identity:
  if (&lhs == &rhs) {
    return true;
  }

  // Hash lengths must be the same:
  if (lhs.hash_len() != rhs.hash_len()) {
    return false;
  }

  // Compare children:
  auto lhsChildren = lhs.getChildren();
  auto rhsChildren = rhs.getChildren();

  if ((*std::get<0>(lhsChildren) != *std::get<0>(rhsChildren)) ||
      (*std::get<1>(lhsChildren) != *std::get<1>(rhsChildren))) {
    return false;
  }

  // Finally, the hash values themselves:
  return lhs.hash_ == rhs.hash_;
}
