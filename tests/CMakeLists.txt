# This file (c) 2016 AlertAvert.com.  All rights reserved.

project(test_brick)
set(TEST_TARGET ${PROJECT_NAME})

enable_testing()

# Conan Packaging support
include(../.conan/conanbuildinfo.cmake)
conan_basic_setup()

set(TEST_FILES
    ${TESTS_DIR}/test_brick.cpp
    ${TESTS_DIR}/test_view.cpp
    ${TESTS_DIR}/test_merkle.cpp)

add_executable(${TEST_TARGET} ${SOURCE_FILES} ${TEST_FILES})
target_link_libraries(${TEST_TARGET} pthread ${CONAN_LIBS})

add_test(${TEST_TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TEST_TARGET})
