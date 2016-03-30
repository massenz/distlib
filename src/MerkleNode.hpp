// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/20/16.



#ifndef MERKLENODE_HPP
#define MERKLENODE_HPP


#include <memory>
#include <list>
#include <openssl/md5.h>
#include <iostream>
#include <cassert>

#include "brick.hpp"


template <typename E>
E hash(const E& lhs, const E &rhs);


// String specialization of the hashing function.
template <>
inline std::string hash(const std::string& lhs, const std::string& rhs) {
  return hash_str(lhs + rhs);
}


template <typename T>
class MerkleNode {
protected:
  const std::shared_ptr<const MerkleNode> left_, right_;
  T hash_;
  size_t hash_len_;

  virtual T computeHash() const;

public:
  MerkleNode(const T& hash, size_t len) : hash_(hash), hash_len_(len),
                                          left_(nullptr), right_(nullptr) {}

  MerkleNode(const MerkleNode<T>& left, const MerkleNode<T>& right) :
      left_(&left), right_(&right), hash_len_(left.hash_len()), hash_(computeHash()) {
    assert(left.hash_len_ == right.hash_len_);
  }

  virtual ~MerkleNode() { }

  virtual bool validate() const;

  size_t hash_len() const { return hash_len_; }

  std::tuple<std::shared_ptr<const MerkleNode<T>>,
             std::shared_ptr<const MerkleNode<T>>> getChildren() const {
    return std::make_tuple(left_, right_);
  };

  template<typename E>
  friend bool operator==(const MerkleNode<E> &lhs, const MerkleNode<E> &rhs);
};


template<typename T>
T MerkleNode<T>::computeHash() const {
  if (!left_ && !right_) {
    return hash_;
  }

  // If either child is empty, just double up the other one.
  const T &tmp_l = left_ ? left_->hash_ : right_->hash_;
  const T &tmp_r = right_ ? right_->hash_ : left_->hash_;

  return hash(tmp_l, tmp_r);
}


template<typename T>
bool MerkleNode<T>::validate() const {
  // If a leaf node, it's valid by definition.
  if (!left_ && !right_) {
    return true;
  }

  // If either child is not valid, the entire subtree is invalid too.
  if (left_ && !left_->validate()) {
    return false;
  }
  if (right_ && !right_->validate()) {
    return false;
  }

  return hash_ == computeHash();
}


template<typename T>
bool operator==(const MerkleNode<T>& lhs, const MerkleNode<T>& rhs) {
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


template <typename T>
inline bool operator!=(const MerkleNode<T>& lhs, const MerkleNode<T>& rhs) {
  return !(lhs == rhs);
}


template<typename T>
const MerkleNode<T> build(const std::list<const T&>& values);

#endif // MERKLENODE_HPP
