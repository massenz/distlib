// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/20/16.

#pragma once

#include <string.h>

#include <memory>
#include <list>
#include <openssl/md5.h>
#include <iostream>
#include <cassert>

#include "brick.hpp"


/**
 * Abstract base class to model a Merkle Tree, whose nodes' values are the hash of the
 * concatenated values of their descendants' hashes.
 * The leaf nodes store both the `value` and its hash.
 *
 * <p>The `hash_func()` is a parameter for the template, as well as the length of the resultant
 * hash.
 *
 * <p>Specializations of this class must "plug" in the actual hashing function that takes two
 * hash values (both of length `hash_len`) concatenates them, and then computes the hash (whose
 * result must equally be `hash_len` bytes).
 *
 * <p>For more details on Merkle Trees, see:<br>
 * "Mastering Bitcoin", Andreas M. Antonopoulos, Chapter 7.
 */
template <typename T, char* (hash_func)(const T&), size_t hash_len>
class MerkleNode {
protected:

  const MerkleNode *left_, *right_;
  const char* hash_;
  const std::shared_ptr<const T> value_;

  /**
   * Computes the hash of the children nodes' respective hashes.
   * In other words, given a node N, with children (N1, N2), whose hash values are,
   * respectively, H1 and H2, computes:
   *
   *     H = hash(H1 || H2)
   *
   * where `||` represents the concatenation operation.
   * If the `right_` descendant is missing, it will simply duplicate the `left_` node's hash.
   *
   * For a "leaf" node (both descendants missing), it will use the `hash_func()` to compute the
   * hash of the stored `value_`.
   */
  virtual const char* computeHash() const = 0;

public:

  /**
   * Builds a "leaf" node, with no descendants and a `value` that will be copied and stored.
   * We also compute the hash (`hash_func()`) of the value and store it in this node.
   *
   * We assume ownership of the pointer returned by the `hash_func()` which we assume has been
   * freshly allocated, and will be released upon destruction of this node.
   */
  MerkleNode(const T& value) : value_(new T(value)), left_(), right_() {
    hash_ = hash_func(value);
  }

  /**
   * Creates an intermediate node, storing the descendants as well as computing the compound hash.
   */
  MerkleNode(const MerkleNode<T, hash_func, hash_len>& left,
             const MerkleNode<T, hash_func, hash_len>& right) :
      left_(&left), right_(&right), value_(nullptr) {
//    hash_ = computeHash();
  }

  /**
   * Deletes the memory allocated in the `hash_` pointer: if your `hash_func()` and/or the
   * `computeHash()` method implementation do not allocate this memory (or you do not wish to
   * free the allocated memory) remember to override this destructor's behavior too.
   */
  virtual ~MerkleNode() {
    if (hash_) delete[] hash_;
  }

  /**
   * Recursively validate the subtree rooted in this node: if executed at the root of the tree,
   * it will validate the entire tree.
   */
  virtual bool validate() const;

  /**
   * The length of the hash, also part of the template instantiation (`hash_len`).
   *
   * @see hash_len
   */
  size_t len() const { return hash_len; }

  /**
   * Returns the buffer containing the hash value, of `len()` bytes length.
   *
   * @see len()
   */
  const char* hash() const { return hash_; }

  std::tuple<const MerkleNode*, const MerkleNode*> getChildren() const {
    return std::make_tuple(left_, right_);
  };

  bool hasChildren() const {
    return left_ != nullptr && right_ != nullptr;
  }

};


template<typename T, char* (hash_func)(const T&), size_t hash_len>
bool MerkleNode<T, hash_func, hash_len>::validate() const {
  std::unique_ptr<const char> computedHash;

  if (!left_ && !right_) {
    computedHash.reset(hash_func(*value_));
  } else {
    computedHash.reset(computeHash());
  }

  /** If either child is not valid, the entire subtree is invalid too. */
  if (left_ && !left_->validate()) {
    return false;
  }
  if (right_ && !right_->validate()) {
    return false;
  }

  return memcmp(hash_, computedHash.get(), len()) == 0;
}


template<typename T, char* (hash_func)(const T&), size_t hash_len>
bool operator==(const MerkleNode<T, hash_func, hash_len>& lhs,
                const MerkleNode<T, hash_func, hash_len>& rhs) {
  // First the trivial case of identity:
  if (&lhs == &rhs) {
    return true;
  }

  if (lhs.hasChildren()) {
    // Compare children:
    auto lhsChildren = lhs.getChildren();
    if (rhs.hasChildren()) {
      auto rhsChildren = rhs.getChildren();

      if ((*std::get<0>(lhsChildren) != *std::get<0>(rhsChildren)) ||
          (*std::get<1>(lhsChildren) != *std::get<1>(rhsChildren))) {
        return false;
      }
    }
  } else {
    if (rhs.hasChildren()) {
      return false;
    }
  }

  // Finally, the hash values themselves:
  return memcmp(lhs.hash(), rhs.hash(), hash_len);
}


template<typename T, char* (hash_func)(const T&), size_t hash_len>
inline bool operator!=(const MerkleNode<T, hash_func, hash_len>& lhs,
                       const MerkleNode<T, hash_func, hash_len>& rhs) {
  return !(lhs == rhs);
}


template<typename T, char* (hash_func)(const T&), size_t hash_len>
const MerkleNode<T, hash_func, hash_len> build(const std::list<const T&>& values);


char* hash_str_func(const std::string& value) {
  unsigned char* buf;
  size_t len = basic_hash(value.c_str(), value.length(), &buf);
  assert(len == MD5_DIGEST_LENGTH);
  return (char *)buf;
}

class MD5StringMerkleNode : public MerkleNode<std::string, hash_str_func, MD5_DIGEST_LENGTH> {
protected:
  virtual const char* computeHash() const {
    char *buffer = new char[2 * MD5_DIGEST_LENGTH];
    unsigned char *computedHash;

    memcpy(buffer, left_->hash(), MD5_DIGEST_LENGTH);
    memcpy(buffer + MD5_DIGEST_LENGTH, right_->hash(), MD5_DIGEST_LENGTH);
    size_t len = basic_hash(buffer, 2 * MD5_DIGEST_LENGTH, &computedHash);
    assert(len == MD5_DIGEST_LENGTH);

    delete[](buffer);
    return (char *)computedHash;
  }

public:
  MD5StringMerkleNode(const std::string& value) : MerkleNode(value) {}
  MD5StringMerkleNode(const MD5StringMerkleNode& lhs, const MD5StringMerkleNode& rhs) :
      MerkleNode(lhs, rhs) {
    hash_ = computeHash();
  }

  virtual ~MD5StringMerkleNode() {}
};
