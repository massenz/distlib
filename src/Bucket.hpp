// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.



#ifndef BRICK_BUCKET_HPP
#define BRICK_BUCKET_HPP


#include <memory>
#include <string>
#include <glog/logging.h>


class Bucket {
private:
  const std::string name_;
  std::unique_ptr<float[]> hash_points_;
  int replicas_;

public:
  Bucket(const std::string &name, int replicas);

  virtual ~Bucket() {}

  // Explicitly disallow copy & assignment
  Bucket(const Bucket&) = delete;
  Bucket operator=(const Bucket&) = delete;


  const std::string name() const {
    return name_;
  }


  const std::unique_ptr<float[]> hash_points() const {
    // Make a copy, do not return the original pointer.
    std::unique_ptr<float[]> ret(new float[replicas()]);
    for (int i = 0; i < replicas_; ++i) {
      ret[i] = hash_points_[i];
    }
    return ret;
  }

  float hash_point(int i) const {
    if (i >= replicas_) {
      LOG(FATAL) << "Out of bound: requesting hash_point #" << i << ", when only "
                 << replicas_ << " are available ('" << name_ << "')";
    }
    return hash_points_[i];
  }

  std::pair<int, float> nearest_hash_point(float x) const;

  int replicas() const {
    return replicas_;
  }
};


/**
 * Utility function to stream a string representation of a bucket.
 *
 * @return the passed in stream, to which the bucket has been streamed to.
 */
std::ostream& operator<<(std::ostream& out, const Bucket &bucket);

#endif //BRICK_BUCKET_HPP
