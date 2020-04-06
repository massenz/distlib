// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/20/16.

#pragma once

#include <string.h>

#include <memory>
#include <list>
#include <openssl/md5.h>
#include <iostream>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

#include "ConsistentHash.hpp"
#include "utils/utils.hpp"


namespace merkle {

using std::unique_ptr;
using std::shared_ptr;

/**
 * Exception thrown when the tree made of `MerkleNode` is found to be in an invalid state.
 */
class merkle_tree_invalid_state_exception : public utils::base_error {
 public:
  explicit merkle_tree_invalid_state_exception(std::string error) :
      base_error { std::move(error) } { }

  merkle_tree_invalid_state_exception() :
      base_error("Merkle Tree failed to validate hashes") {}
};

/**
 * Abstract base class to model a Merkle Tree, whose nodes' values are the hash of the
 * concatenated values of their descendants' hashes.
 * The leaf nodes store both the `value` and its hash.
 *
 * <p>The `Hash` is a parameter for the template, and is the hashing function for the
 * leaf nodes' values; the children hashes are computed with the `HashNode` function,
 * which in turn takes the hashes and generates a new one.
 *
 * <p>In the most trivial case, two `U` objects can simply concatenated and the same
 * `Hash` function can be applied, but this is left to the user of this class to
 * decide and implement.
 *
 * <p>`HashNode` must return a correct value when either (or, possibly, both) arguments
 * are `nullptr`s; however, the hashed values may differ for the same value passed, but
 * in either positions, when the other is null; in other words, this MAY fail:
 *
 *      shared_ptr<U> myHash = someHash(aT);
 *      ASSERT_EQ(HashNode(myHash, shared_ptr<U> {}), HashNode(shared_ptr<U> {}, myHash))
 *
 * <p>For more details on Merkle Trees, see:<br>
 * "Mastering Bitcoin", Andreas M. Antonopoulos, Chapter 7.
 *
  * @tparam T the type of the leaf nodes' values
  * @tparam U the type of the hash operation, must be composable
  * @tparam Hash the function that computes the hash of a `T` object
  * @tparam HashNode a function that computes the hash of two `U` hash values
 */
template<typename T, typename U,
     U (Hash)(const T &),
     U (HashNode)(const shared_ptr<U> &, const shared_ptr<U> &)
>
class MerkleNode {
 protected:

  std::unique_ptr<const MerkleNode> left_, right_;
  U hash_;
  const std::shared_ptr<T> value_;

 public:
  using node_type = MerkleNode<T, U, Hash, HashNode>;

  /**
   * Builds a "leaf" node, with no descendants and a `value` that will be copied and stored.
   * We also compute the hash (`hash_`) of the value and store it in this node.
   *
   * @param value the value stored in the node and whose `Hash()` value is stored to validate
   *    eventually the whole tree.
   */
  explicit MerkleNode(const T &value) : value_{ new T(value) }, left_(nullptr), right_(nullptr) {
    hash_ = Hash(value);
  }

  /**
   * Creates an intermediate node, storing the descendants as well as computing the computed hash.
   *
   * <p>Both arguments MUST be pointers allocated on the heap, they can't be pointers to
   * stack-allocated objects, as the `unique_ptr`s that hold them will try to deallocate
   * them upon destruction of this node.
   *
   * <p>Calling this method means that the caller must relinquish ownership of these pointers.
   *
   * @param left one of the children of this node, cannot be `nullptr`
   * @param right the other children of this node, cannot be `nullptr`
   */
  MerkleNode(const node_type *left,
             const node_type *right) {
    left_.reset(left);
    right_.reset(right);
    hash_ = HashNode(std::make_shared<U>(left->hash_),
        std::make_shared<U>(right->hash_));
  }

  virtual ~MerkleNode() = default;

  // A MerkleNode is by nature immutable; it makes no sense to copy or move it.
  MerkleNode(const MerkleNode&) = delete;
  MerkleNode& operator=(const MerkleNode&) = delete;
  MerkleNode(MerkleNode&&) = delete;
  MerkleNode& operator=(MerkleNode&&) = delete;

  /**
   * Recursively validate the subtree rooted in this node: if executed at the root of the tree,
   * it will validate the entire tree.
   */
  bool IsValid() const {
    // A node is either a leaf (and both descendants' pointers are null) or an intermediate node,
    // in which case both children are non-null (this is a direct consequence of
    // how we add nodes: see `AddLeaf`).
    if (IsLeaf()) {
      return hash() == Hash(*value_);
    } else {
      return left_->IsValid() && right_->IsValid();
    }
  }

  /**
   * *Note* this is *not* the `Hash()` function (case is different).
   *
   * @return this node's hash value.
   */
  U hash() const { return hash_; }

  /**
   * Getter for the node's stored value.
   */
  T value() const { return *value_; }

  /**
   * Whether this node is a "leaf node," i.e., it has no descendants.
   *
   * <p> Note that, due to the nature of the nodes of a `MerkleTree` (and how
   * we construct the tree itself) a non-leaf node has _always_ both children
   * nodes non-null.
   *
   * @return whether this is a leaf (terminal) node (from which it is hence possible to
   *    extract the `value`)
   * @see AddLeaf()
   * @see value()
   */
  bool IsLeaf() const {
    return (left_ == nullptr) && (right_ == nullptr);
  }

  const node_type *left() const { return left_.get(); }
  const node_type *right() const { return right_.get(); }

  /**
   * Relinquish ownership of the pointer to the LHS descendant, and will not
   * attempt to release memory upon destruction; the caller becomes responsible
   * to manage the pointer's lifecycle.
   *
   * @return the "raw" pointer to the LHS node
   */
  const node_type *release_left() { return left_.release(); }

  /**
 * Relinquish ownership of the pointer to the LHS descendant, and will not
 * attempt to release memory upon destruction; the caller becomes responsible
 * to manage the pointer's lifecycle.
 *
 * @return the "raw" pointer to the LHS node
 */
  const node_type *release_right() { return right_.release(); }

};


template<typename T, typename U,
     U (Hash)(const T &),
     U (HashNode)(const shared_ptr<U> &, const shared_ptr<U> &)
>
bool operator==(const MerkleNode<T, U, Hash, HashNode> &lhs,
                const MerkleNode<T, U, Hash, HashNode> &rhs) {
  // First the trivial case of identity.
  if (&lhs == &rhs) {
    return true;
  }

  // Both nodes must be either leaf, or non-leaf.
  if (lhs.IsLeaf() != rhs.IsLeaf()) {
    return false;
  }

  // Finally, the hash values themselves.
  return lhs.hash() == rhs.hash();
}

template<typename T, typename U,
     U (Hash)(const T &),
     U (HashNode)(const shared_ptr<U> &, const shared_ptr<U> &)
>
inline bool operator!=(const MerkleNode<T, U, Hash, HashNode> &lhs,
                       const MerkleNode<T, U, Hash, HashNode> &rhs) {
  return !(lhs == rhs);
}

/**
 * Adds a leaf to the Merkle Tree whose `root` is passed in.
 *
 * <p>This method simply creates a new root node, whose left descendant is the original `root`
 * and the right descendant is the newly created `MerkleNode` with the `T value`.
 *
 * <p>This is a very efficient insertion model, but leaves the tree unbalanced: this is not
 * as important (as, for example, in a sorted tree, where the lookup time is logarithmic only
 * if the tree is approximately balanced): a Merkle Tree is only usually traversed to visit
 * all nodes and ensure the validity of the tree.
 *
 *
 * <pre>
 *   Before:                    After:
 *          root                              root (returned)
 *          / \                               /           \
 *         --  v1                     previous root        value
 *        / \                         / \
 *      ...  v2                      --  v1
 *                                  / \
 *                                ...  v2
 * </pre>
 *
 * @param root the root of the Merkle Tree; we take ownership of this pointer.
 * @param value the leaf to append at the end of the tree.
 * @return the new `root` to the tree.
 */
template<typename T, typename U,
     U (Hash)(const T &),
     U (HashNode)(const shared_ptr<U> &, const shared_ptr<U> &)
>
inline std::unique_ptr<MerkleNode<T, U, Hash, HashNode>> AddLeaf(
    std::unique_ptr<MerkleNode<T, U, Hash, HashNode>> root, const T &value) {
  auto right = std::make_unique<MerkleNode<T, U, Hash, HashNode>>(value);
  return std::make_unique<MerkleNode<T, U, Hash, HashNode>>(root.release(), right.release());
}


/**
 * Takes a list of `T` elements and builds the Merkle Tree with these elements (and their hashes)
 * as the leaves, with all the intermediate nodes hashes.
 *
 * @return the root of the tree.
 */
template<typename T, typename U,
     U (Hash)(const T &),
     U (HashNode)(const shared_ptr<U> &, const shared_ptr<U> &)
>
std::unique_ptr<MerkleNode<T, U, Hash, HashNode>> Build(const std::vector<T> &values) {

  if (values.empty()) {
    throw merkle_tree_invalid_state_exception{"Cannot build a MerkleTree with an empty vector"};
  }

  auto root = std::make_unique<MerkleNode<T, U, Hash, HashNode>>(values[0]);
  auto pos = values.begin() + 1;
  while (pos != values.end()) {
    root = std::move(AddLeaf<T, U, Hash, HashNode>(std::move(root), *pos));
    pos++;
  }
  return root;
}


/**
 * Traverses the tree and returns all values in insertion order.
 *
 * @tparam T the type of the nodes' values.
 * @tparam U the type of the hash for the node
 * @tparam Hash the hashing function, returns values of type `U`
 * @tparam HashNode the hashing functions for non-leaf nodes, hashes two `U` objects
 *      and returns a `U` value
 * @param root the root of the tree
 * @return the `list` of all values in the tree, in insertion order.
 */
template<typename T, typename U,
     U (Hash)(const T &),
     U (HashNode)(const shared_ptr<U> &, const shared_ptr<U> &)
>
std::list<T> GetAllValues(MerkleNode<T, U, Hash, HashNode> *root) {
  if (!root->IsValid()) {
    throw merkle_tree_invalid_state_exception { };
  }

  return __GetValues(root);
}


/**
 * Internal function to recursively traverse the tree.
 *
 * <p><bold>DO NOT USE</bold>: this is meant for internal use,
 * use instead `GetAllValues`.
 *
 * @param root a "raw" pointer to the head of the tree.
 * @return a list of values (in insertion order) from the tree.
 */
template<typename T, typename U,
     U (Hash)(const T &),
     U (HashNode)(const shared_ptr<U> &, const shared_ptr<U> &)
>
std::list<T> __GetValues(const MerkleNode<T, U, Hash, HashNode> *root) {

  using NodeType = MerkleNode<T, U, Hash, HashNode>;
  std::list<T> values;

  const NodeType *current = root;
  if (current->IsLeaf()) {
    values.push_front(current->value());
    return values;
  }

  // While not guaranteed, currently traversing the tree breadth-first and
  // checking on the right branch first, guarantees that values are emitted
  // in the same order as they were inserted, if they are added to the list
  // in "reverse" order (adding to the front of the list).
  if (current->right()) {
      auto others = __GetValues(current->right());
      // Remember we want to prepend the nodes returned from
      // a depth-first traversal.
      // See also: https://stackoverflow.com/questions/1449703/how-to-append-a-listt-object-to-another
      others.splice(others.end(), values);
      values = std::move(others);
  }
  if (current->left()) {
      auto others = __GetValues(current->left());
      // Remember we want to prepend the nodes returned from
      // a depth-first traversal.
      // See also: https://stackoverflow.com/questions/1449703/how-to-append-a-listt-object-to-another
      others.splice(others.end(), values);
      values = std::move(others);
  }
  return values;
}

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
 * This is an example of how one can build a Merkle tree given a concrete type and an arbitrary
 * hashing function.
 */
 /*
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
  MD5StringMerkleNode(const std::string &value) : MerkleNode(value) {}

  // We need to explictily invoke the ``computeHash()`` method here, or an linker error will be
  // thrown, as the invocation on the parent class will attempt to invoke a 'pure virtual' method.
  MD5StringMerkleNode(const MD5StringMerkleNode *lhs, const MD5StringMerkleNode *rhs) :
      MerkleNode(lhs, rhs) {
    hash_ = computeHash();
  }

//  virtual ~MD5StringMerkleNode() { }
};
  */
} // namespace merkle
