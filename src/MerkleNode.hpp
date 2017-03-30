// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/20/16.

#pragma once

#include <string.h>

#include <memory>
#include <list>
#include <openssl/md5.h>
#include <iostream>
#include <cassert>

#include "ConsistentHash.hpp"


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
template<typename T, char *(hash_func)(const T &), size_t hash_len>
class MerkleNode {
protected:

  std::unique_ptr<const MerkleNode> left_, right_;
  const char *hash_;
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
  virtual const char *computeHash() const = 0;

public:

  typedef enum {
    LEFT, RIGHT
  } WhichNode;

  // THIS SHOULD NOT BE USED.
  // It has been defined purely for testing purposes: setting a child directly here will cause
  // the entire tree's validation to fail.
  //
  // Use the ``::addLeaf()`` function instead.
  void setNode(const MerkleNode *node, const WhichNode which) {
    switch (which) {
      case LEFT:
        left_.reset(node);
        break;
      case RIGHT:
        right_.reset(node);
        break;
      default:
        return;
    }
    if (hash_) delete[](hash_);
    hash_ = computeHash();
  }

  /**
   * Builds a "leaf" node, with no descendants and a `value` that will be copied and stored.
   * We also compute the hash (`hash_func()`) of the value and store it in this node.
   *
   * We assume ownership of the pointer returned by the `hash_func()` which we assume has been
   * freshly allocated, and will be released upon destruction of this node.
   */
  MerkleNode(const T &value) : value_(new T(value)), left_(nullptr), right_(nullptr) {
    hash_ = hash_func(value);
  }

  /**
   * Creates an intermediate node, storing the descendants as well as computing the compound hash.
   */
  MerkleNode(const MerkleNode<T, hash_func, hash_len> *left,
             const MerkleNode<T, hash_func, hash_len> *right) :
      left_(left), right_(right), value_(nullptr) {

    // In C++ we cannot invoke `computeHash()` here (where it would make the most sense), even if
    // this constructor is invoked via the constructor of a derived class, which does implement
    // `computeHash()`: at this point that object is not yet constructed, and its invocation here
    // would trigger a "pure virtual function invocation" error.
    // Call this method instead in the derived class's constructor.
    //    hash_ = computeHash();
  }

  /**
   * Deletes the memory pointed to by the `hash_` pointer: if your `hash_func()` and/or the
   * `computeHash()` method implementation do not allocate this memory (or you do not wish to
   * free the allocated memory) remember to override this destructor's behavior too.
   */
  virtual ~MerkleNode() {
    if (hash_) delete[](hash_);
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
  const char *hash() const { return hash_; }

  bool hasChildren() const {
    return left_ || right_;
  }

  const MerkleNode * left() const { return left_.get(); }
  const MerkleNode * right() const { return right_.get(); }
};


template<typename T, char *(hash_func)(const T &), size_t hash_len>
bool MerkleNode<T, hash_func, hash_len>::validate() const {

  // If either child is not valid, the entire subtree is invalid too.
  if (left_ && !left_->validate()) {
    return false;
  }
  if (right_ && !right_->validate()) {
    return false;
  }

  std::unique_ptr<const char> computedHash(hasChildren() ? computeHash() : hash_func(*value_));
  return memcmp(hash_, computedHash.get(), len()) == 0;
}


template<typename T, char *(hash_func)(const T &), size_t hash_len>
bool operator==(const MerkleNode<T, hash_func, hash_len> &lhs,
                const MerkleNode<T, hash_func, hash_len> &rhs) {
  // First the trivial case of identity.
  if (&lhs == &rhs) {
    return true;
  }

  // If either child is missing, or not equal, the nodes are different.
  if (*lhs.left() != *rhs.left() || *lhs.right() != *rhs.right()) {
    return false;
  }

  // Finally, the hash values themselves.
  return memcmp(lhs.hash(), rhs.hash(), hash_len) == 0;
}


template<typename T, char *(hash_func)(const T &), size_t hash_len>
inline bool operator!=(const MerkleNode<T, hash_func, hash_len> &lhs,
                       const MerkleNode<T, hash_func, hash_len> &rhs) {
  return !(lhs == rhs);
}


/**
 * Takes a list of `T` elements and builds the Merkle Tree with these elements (and their hashes)
 * as the leaves, with all the intermediate nodes hashes.
 *
 * @return the root of the tree.
 */
template<typename T, char *(hash_func)(const T &), size_t hash_len>
const MerkleNode<T, hash_func, hash_len> build(const std::list<const T &> &values);


// Recursive implementation of the build algorithm.
//
template<typename NodeType>
const NodeType *build_(NodeType *nodes[], size_t len) {

  if (len == 1) return new NodeType(nodes[0], nullptr);
  if (len == 2) return new NodeType(nodes[0], nodes[1]);

  size_t half = len % 2 == 0 ? len / 2 : len / 2 + 1;
  return new NodeType(build_(nodes, half), build_(nodes + half, len - half));
}

template<typename T, typename NodeType>
const NodeType *build(const std::list<T> &values) {

  NodeType *leaves[values.size()];
  int i = 0;
  for (auto value : values) {
    leaves[i++] = new NodeType(value);
  }

  return build_(leaves, values.size());
};


template<typename NodeType>
const void destroy(const NodeType *node) {
  if (node->hasChildren()) {
    auto children = node->getChildren();
    auto left = std::get<0>(children);
    auto right = std::get<1>(children);
    if (left) destroy(left);
    if (right) destroy(right);
  }
  delete node;
};

/**
 * Adds a leaf to the Merkle Tree whose `root` is passed in.
 *
 * @param root the root of the Merkle Tree; all hashes in the path to the leaf will be recomputed.
 * @param value the leaf to append to the nearest empty slot.
 *
 * @return `true` if the operation was successful.
 */
template<typename T, char *(hash_func)(const T &), size_t hash_len>
bool addLeaf(MerkleNode<T, hash_func, hash_len> *root, const T &value);


char *hash_str_func(const std::string &value) {
  unsigned char *buf;
  size_t len = basic_hash(value.c_str(), value.length(), &buf);
  assert(len == MD5_DIGEST_LENGTH);
  return (char *) buf;
}

/**
 * An implementation of a Merkle Tree with the nodes' elements ``strings`` and their hashes
 * computed using the MD5 algorithm.
 *
 * This is an example of how one can build a Merkle tree given a concrete type and an arbtrary
 * hashing function.
 */
class MD5StringMerkleNode : public MerkleNode<std::string, hash_str_func, MD5_DIGEST_LENGTH> {
protected:

  // This is the only function that we need to implement, in order to specialize the template to
  // our concrete types: the hash is computed using the MD5 algorithm (as implemented by the
  // OpenSSL library).
  virtual const char *computeHash() const {
    char buffer[2 * MD5_DIGEST_LENGTH];
    unsigned char *computedHash;

    memcpy(buffer, left_->hash(), MD5_DIGEST_LENGTH);

    // If the right node is null, we just double the left node.
    memcpy(buffer + MD5_DIGEST_LENGTH, right_ ? right_->hash() : left_->hash(), MD5_DIGEST_LENGTH);
    size_t len = basic_hash(buffer, 2 * MD5_DIGEST_LENGTH, &computedHash);
    assert(len == MD5_DIGEST_LENGTH);

    return (char *) computedHash;
  }

public:

  // This constructor does not need to do anything, the parent class will take care of all the
  // necessary initialization.
  MD5StringMerkleNode(const std::string &value) : MerkleNode(value) { }

  // We need to explictily invoke the ``computeHash()`` method here, or an linker error will be
  // thrown, as the invocation on the parent class will attempt to invoke a 'pure virtual' method.
  MD5StringMerkleNode(const MD5StringMerkleNode *lhs, const MD5StringMerkleNode *rhs) :
      MerkleNode(lhs, rhs) {
    hash_ = computeHash();
  }

//  virtual ~MD5StringMerkleNode() { }
};
