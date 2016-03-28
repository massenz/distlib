// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 3/6/16.


#include <gtest/gtest.h>
#include <View.hpp>

using namespace std;

TEST(ViewTests, CanCreate) {
  View v(5);
}

TEST(ViewTests, CanAddBucket) {
  Bucket b("test_bucket", 5);
  View v(5);
  ASSERT_NO_FATAL_FAILURE(v.add(&b));

  ASSERT_EQ(1, v.num_buckets());
}


TEST(ViewTests, CanFindBucket)
{
  Bucket b("test_bucket", 5);
  View v(5);
  ASSERT_NO_FATAL_FAILURE(v.add(&b));

  // As this is the only bucket, anything will be assigned to it.
  auto found = v.bucket("foobar");
  ASSERT_EQ(&b, found);
}

TEST(ViewTests, CanEmitToStdout)
{
  View v(5);
  Bucket b1("test-1", 3), b2("test-2", 3), b3("test-3", 5);

  v.add(&b1);
  v.add(&b2);
  v.add(&b3);
  ASSERT_EQ(3, v.num_buckets());

  ASSERT_NO_FATAL_FAILURE(std::cout << v << std::endl);
}

TEST(ViewTests, CanRemoveBucket) {
  View v(7);
  Bucket b1("test-1", 3), b2("test-2", 3), b3("test-3", 5);

  v.add(&b1);
  v.add(&b2);
  v.add(&b3);
  EXPECT_EQ(3, v.num_buckets());

  v.remove(&b2);
  EXPECT_EQ(2, v.num_buckets());

  v.remove(&b1);
  v.remove(&b2);
  EXPECT_EQ(1, v.num_buckets());

  v.remove(&b1);
  v.remove(&b1);
  EXPECT_EQ(1, v.num_buckets());

  v.remove(&b3);
  EXPECT_EQ(0, v.num_buckets());
}
