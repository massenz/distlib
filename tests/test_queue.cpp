// Copyright (c) 2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2020-08-01.

#include <atomic>
#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "utils/ThreadsafeQueue.hpp"
#include "View.hpp"

using namespace utils;
using namespace std;


TEST(QueueTests, CanPushPopItems) {
  ThreadsafeQueue<int> my_ints;

  for (int i = 0; i < 10; ++i) {
    my_ints.push(i);
  }

  ASSERT_EQ(10, my_ints.size());

  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(i, *(my_ints.pop()));
  }

  ASSERT_FALSE(my_ints.pop());
  ASSERT_EQ(0, my_ints.size());
}

TEST(QueueTests, WorksWithSharedPtrs) {
  BucketPtr bp1 = std::make_shared<Bucket>("b1"s, std::vector<float>{0.1, 0.5});
  BucketPtr bp2 = std::make_shared<Bucket>("b2"s, std::vector<float>{0.2, 0.6});

  utils::ThreadsafeQueue<BucketPtr> my_buckets;
  my_buckets.push(bp1);
  my_buckets.push(bp2);

  auto x = my_buckets.pop();
  ASSERT_TRUE(x);
  auto bp = *x;
  ASSERT_EQ(bp1, bp);

  x = my_buckets.pop();
  ASSERT_TRUE(x);
  bp = *x;
  ASSERT_EQ(bp2, bp);

  ASSERT_EQ("b2", bp2->name());

  ASSERT_EQ(0, my_buckets.size());
}

class ThreadingTests : public ::testing::Test {
 protected:
  utils::ThreadsafeQueue<std::string> stringsq_;
  std::atomic<bool> done;
  std::atomic<int> count;

  void SetUp() override {
    done.store(false);
    count.store(0);
  };

  void TearDown() override {
    // teardown goes here
  }

  bool empty() const {
    return stringsq_.size() == 0;
  }

 public:
  void PutN(int num) {
    for (int i = 0; i < num; ++i) {
      stringsq_.push("Item #"s + std::to_string(i));
    }
  }

  void Fetch() {
    while (!done.load() || !empty()) {
      if (empty()) {
        std::this_thread::sleep_for(1ms);
      } else {
        if (stringsq_.pop())
          count++;
      }
    }
  }

 public:
  // Needed to make the compiler happy, as ThreadsafeQueue has a noexcept(false) destructor.
  ~ThreadingTests() noexcept override = default;
};

TEST_F(ThreadingTests, CanFillFlushSeparately) {
  std::thread t(&ThreadingTests::PutN, this, 5);
  t.join();
  ASSERT_EQ(5, stringsq_.size());
  std::thread t2(&ThreadingTests::Fetch, this);
  done.store(true);
  t2.join();
  ASSERT_TRUE(empty());
  ASSERT_EQ(5, count);
}

TEST_F(ThreadingTests, CanFillFlushConcurrently) {
  std::thread t(&ThreadingTests::PutN, this, 5),
    t2(&ThreadingTests::PutN, this, 15);
  std::thread fetch(&ThreadingTests::Fetch, this);
  t.join();
  t2.join();
  done.store(true);
  fetch.join();
  ASSERT_TRUE(empty());
  ASSERT_EQ(20, count);
}

TEST_F(ThreadingTests, CanFillFlushConcurrentlyHigh) {
  std::thread fetch1(&ThreadingTests::Fetch, this);
  std::thread fetch2(&ThreadingTests::Fetch, this);
  std::vector<std::thread> threads;
  for (int j = 0; j < 5; j++) {
    threads.push_back(std::thread(&ThreadingTests::PutN, this, 15));
  }
  std::for_each(threads.begin(), threads.end(), [] (auto &t) { t.join(); });
  done.store(true);

  fetch1.join();
  fetch2.join();

  ASSERT_TRUE(empty());
  ASSERT_EQ(5 * 15, count);
}
