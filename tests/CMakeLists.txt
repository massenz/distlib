# This file (c) 2016 AlertAvert.com.  All rights reserved.

project(distlib_test)
set(TEST_TARGET ${PROJECT_NAME})

enable_testing()

# Conan Packaging support
include(../.conan/conanbuildinfo.cmake)
conan_basic_setup()


find_library(GTEST gtest ${INSTALL_DIR}/lib)
if (${GTEST} STREQUAL GTEST-NOTFOUND)
    message(FATAL_ERROR "Could not locate a valid Google Test library installed.")
endif (${GTEST} STREQUAL GTEST-NOTFOUND)

set(TEST_FILES
        ${TESTS_DIR}/test_consistent_hash.cpp
        ${TESTS_DIR}/test_bucket.cpp
        ${TESTS_DIR}/test_gossip.cpp
        ${TESTS_DIR}/test_merkle.cpp
        ${TESTS_DIR}/test_parse_args.cpp
        ${TESTS_DIR}/test_swim_server.cpp
        ${TESTS_DIR}/test_utils_network.cpp
        ${TESTS_DIR}/test_view.cpp
)

# This is necessary as the .pb.cc and .pb.h files will not (yet) be present
# when we run cmake, and thus it will fail complaining it can't find the files.
# See http://stackoverflow.com/questions/19343018/cmake-and-findprotobuf as to
# why this is only necessary in this file (but not the parent).
#
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS} PROPERTIES
                            GENERATED TRUE)

add_executable(${TEST_TARGET}
        ${PROTO_SRCS}
        ${SOURCE_FILES}
        ${SWIM_SOURCES}
        ${TEST_FILES}
        all_tests.cpp
)
target_link_libraries(${TEST_TARGET}
        crypto
        ${GTEST}
        ${SWIM_LIBS}
)

add_test(${TEST_TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TEST_TARGET})
