# Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.

project(distlib_test)
enable_testing()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC")

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

add_executable(unit_tests
        ${UNIT_TESTS}
        tests.h
        all_tests.cpp
)

message(STATUS "Libraries will be linkde from
    ${CMAKE_BINARY_DIR} and ${INSTALL_DIR}/lib")
link_directories(
        ${CMAKE_BINARY_DIR}
        ${INSTALL_DIR}/lib
)

find_package(GTest REQUIRED)
target_link_libraries(unit_tests
        gtest::gtest
        distutils
)
add_test(unit_tests
        ${CMAKE_CURRENT_BINARY_DIR}/unit_tests)
