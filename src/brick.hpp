// Copyright (c) 2016 Marco Massenzio.  All rights reserved.

#pragma once

#include <string>
#include <openssl/md5.h>


/**
 * Exception thrown when errors encountered.
 */
class md5_error {
  // TODO: learn more about C++ exception handling.
};

/**
 * Marker exception for unimplemented methods.
 */
class not_implemented {

};


/**
 * Converts a char buffer into a hex string.
 *
 * It expects the ``digest`` buffer to contain exactly ``MD5_DIGEST_LENGTH``
 * bytes, that will be converted to hex notation and appended to the returned
 * string.
 *
 * @param digest The MD5 digest to convert into a string; exactly
 *    `MD5_DIGEST_LENGTH` bytes in length.
 * @return the hex-encoded string representation of the `digest`.
 */
std::string md5_to_string(const unsigned char *digest);


/**
 * Hashes the given string using MD5 and returns the hash.
 *
 * @param msg The string to hash.
 * @return the MD5 hash of `msg`.
 */
std::string hash_str(const std::string &msg);

/**
 * Computes a "consistent hash" of the given string.
 *
 * For the definition of "consistent hash" see:
 * Karger et al., "Consistent Hashing and Random Trees"
 * https://goo.gl/NBY9yN
 *
 * @param msg the string to hash
 * @return a value in the [0, 1.0) range that can be used as `msg`'s
 *    consistent hash.
 */
float consistent_hash(const std::string &msg);

/**
 * Computes the hash of the given ``value`` and puts into the destination buffer,
 * ``hash_value``, whose length is returned.
 *
 * The buffer is newly allocated and it is the caller's responsibility to deallocate it
 * when done.
 *
 * @param value the value to hash.
 * @param len the length of the value to hash.
 * @param hash_value the digest from the hash, newly allocated.
 *
 * @return the size of the digest buffer.
 */
size_t basic_hash(const char* value, size_t len, unsigned char** hash_value);
