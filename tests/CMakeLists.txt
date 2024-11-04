# Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.

project(distlib_test)
enable_testing()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC")


find_package(GTest REQUIRED)
#find_library(GMOCK gmock ${INSTALL_DIR}/lib)

#if (${GTEST} STREQUAL GTEST-NOTFOUND)
#    message(FATAL_ERROR "Could not locate a valid Google Test library installed.")
#endif ()

set(UNIT_TESTS
        ${TESTS_DIR}/test_bucket.cpp
        ${TESTS_DIR}/test_hash.cpp
        ${TESTS_DIR}/test_keystore.cpp
        ${TESTS_DIR}/test_merkle.cpp
        ${TESTS_DIR}/test_parse_args.cpp
        ${TESTS_DIR}/test_utils_network.cpp
        ${TESTS_DIR}/test_queue.cpp
        ${TESTS_DIR}/test_view.cpp
)

# Add the build directory to the library search path
link_directories(${CMAKE_BINARY_DIR})

add_executable(unit_tests
        ${UNIT_TESTS}
        tests.h
        all_tests.cpp
)

target_link_libraries(unit_tests
        gtest::gtest
        #${GMOCK}
        distutils
)

add_test(unit_tests ${CMAKE_CURRENT_BINARY_DIR}/unit_tests)
