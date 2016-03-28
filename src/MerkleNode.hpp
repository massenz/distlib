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

  virtual T computeHash() const {
    if (!left_ && !right_) {
      return hash_;
    }

    // If either child is empty, just double up the other one.
    const T &tmp_l = left_ ? left_->hash_ : right_->hash_;
    const T &tmp_r = right_ ? right_->hash_ : left_->hash_;

    return hash(tmp_l, tmp_r);
  }

public:
  MerkleNode(const T& hash, size_t len) : hash_(hash), hash_len_(len),
                                          left_(nullptr), right_(nullptr) {}

  MerkleNode(const MerkleNode<T>& left, const MerkleNode<T>& right) :
      left_(&left), right_(&right) {

    std::cout << "Constructing a MerkleNode... ";

    assert(left.hash_len_ == right.hash_len_);
    hash_len_ = left.hash_len();
    hash_ = computeHash();

    std::cout << " done." << std::endl;
  }

  virtual ~MerkleNode() { }

  virtual bool validate() const {
    // If a leaf node, it's valid by definition.
    if (!left_ && !right_) {
      return true;
    }

    if (left_ && !left_->validate()) {
      return false;
    }
    if (right_ && !right_->validate()) {
      return false;
    }

    return hash_ == computeHash();
  }

  size_t hash_len() const { return hash_len_; }

  std::tuple<std::shared_ptr<const MerkleNode<T>>,
             std::shared_ptr<const MerkleNode<T>>> getChildren() const {
    return std::make_tuple(left_, right_);
  };

  template<typename E>
  friend bool operator==(const MerkleNode<E> &lhs, const MerkleNode<E> &rhs);
};


template <typename T>
bool operator==(const MerkleNode<T>& lhs, const MerkleNode<T>& rhs);



template<typename T>
const MerkleNode<T> build(const std::list<const T&> &values);


#endif // MERKLENODE_HPP
