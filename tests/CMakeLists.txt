# This file (c) 2016 AlertAvert.com.  All rights reserved.

project(test_brick)
set(TEST_TARGET ${PROJECT_NAME})

enable_testing()

# Conan Packaging support
include(../.conan/conanbuildinfo.cmake)
conan_basic_setup()

find_library(GTEST gtest ${INSTALL_DIR}/lib)
find_library(GTEST_MAIN gtest_main ${INSTALL_DIR}/lib)

if (DEFINED ${GTEST})
    message("Found Google Test library at ${GTEST}")
else (DEFINED ${GTEST})
    message(ERROR "Could not locate a valid Google Test library installed.")
endif(DEFINED ${GTEST})

set(TEST_FILES
        ${TESTS_DIR}/test_brick.cpp
        ${TESTS_DIR}/test_bucket.cpp
        ${TESTS_DIR}/test_merkle.cpp
        ${TESTS_DIR}/test_view.cpp
)

add_executable(${TEST_TARGET} ${SOURCE_FILES} ${TEST_FILES})
target_link_libraries(${TEST_TARGET}
        crypto
        glog
        pthread
        ${GTEST}
        ${GTEST_MAIN})

add_test(${TEST_TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TEST_TARGET})
