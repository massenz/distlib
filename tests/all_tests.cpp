// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 4/11/17.


#include <glog/logging.h>
#include <gtest/gtest.h>

#include "tests.h"

/**
 * Runs all the Google Test suites.
 *
 * <p>To run with higher logging level sent to console invoke the test suite with something like:
 * <pre>
 *      $ GLOG_v=2 GLOG_logtostderr=1 ./tests/bin/distlib_test --gtest_filter=SwimServer*
 * </pre>
 *
 * See:https://github.com/google/googletest/blob/master/googletest/docs/Primer.md#writing-the-main-function
 */
int main(int argc, char **argv) {
  ::google::InitGoogleLogging(argv[0]);

  ::tests::_init_dre();
  LOG(INFO) << "Running all tests for distlib";

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
