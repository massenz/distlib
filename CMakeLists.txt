# This file (c) 2016 AlertAvert.com.  All rights reserved.

project(brick)
cmake_minimum_required(VERSION 3.4)


# Include common cmake utilities:
include(${COMMON_UTILS_DIR}/commons.cmake OPTIONAL RESULT_VARIABLE COMMONS_FILE)

### Setting up common configuration variables

# Version number
set(RELEASE_MAJOR 0)
set(RELEASE_MINOR 5)
set(RELEASE_PATCH 1)

IF (${COMMONS_FILE} STREQUAL NOTFOUND)
    message(WARNING "Not using CMake utilities")
    set(BUILD_ID 0)
ELSE (${COMMONS_FILE} STREQUAL NOTFOUND)
    get_build_id(BUILD_ID)
ENDIF (${COMMONS_FILE} STREQUAL NOTFOUND)

# Conan Packaging support
include(.conan/conanbuildinfo.cmake)
conan_basic_setup()

set(RELEASE_STRING "${RELEASE_MAJOR}.${RELEASE_MINOR}.${RELEASE_PATCH}-${BUILD_ID}")
message(STATUS "Building Release: ${RELEASE_STRING}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(TESTS_DIR ${PROJECT_SOURCE_DIR}/tests)
set(PROTO_DIR ${PROJECT_SOURCE_DIR}/proto)
set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(ZMQ_LIBRARIES zmq)

# Configure a header file to pass settings into source code
configure_file(
    ${SOURCE_DIR}/config.h.in
    ${PROJECT_BINARY_DIR}/config.h
)

# Building Protocol Buffers.
# NOTE -- the variable Protobuf_INCLUDE_DIR must be in that case or CLion will
# fail to build (command line cmake works just fine with the ALL_UPPERCASE value too).
set(Protobuf_INCLUDE_DIR ${INSTALL_DIR}/include)

find_package(Protobuf REQUIRED)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_DIR}/swim.proto)

IF(${PROTOBUF_FOUND})
    message(STATUS "Generated Protocol Buffer Headers: ${PROTO_HDRS}, Sources: ${PROTO_SRCS}")
ELSE(${PROTOBUF_FOUND})
    message(ERROR "Could not find Google Protocol Buffers libraries")
ENDIF(${PROTOBUF_FOUND})

include_directories(
        ${SOURCE_DIR}
        ${INSTALL_DIR}/include
        ${PROJECT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}
        ${PROTOBUF_INCLUDE_DIRS}
        ${PROTO_HDRS}
)

set(SOURCE_FILES
        ${SOURCE_DIR}/brick.cpp
        ${SOURCE_DIR}/Bucket.cpp
        ${SOURCE_DIR}/View.cpp
)

set(SWIM_SOURCES
        ${SOURCE_DIR}/swim/SwimClient.cpp
        ${SOURCE_DIR}/swim/SwimServer.cpp
        ${SOURCE_DIR}/utils/network.cpp
)

add_executable(brick ${SOURCE_FILES} ${SOURCE_DIR}/main.cpp)
target_link_libraries(brick ${CONAN_LIBS})

# Adding a target demo server that will use the SWIM protocol to derive
# a list of healthy server from a configured pool.
link_directories(${INSTALL_DIR}/lib)
add_executable(server
        ${PROTO_SRCS}
        ${SWIM_SOURCES}
        ${SOURCE_DIR}/server_demo.cpp
)
target_link_libraries(server
        protobuf
        zmq
        glog
        pthread
)



### Unit tests build is defined in the tests/ folder
add_subdirectory(${TESTS_DIR})

# My trivial re-implementation of the `file` Linux command.
# Uses the libmagic magic file detection system library.
add_executable(file /usr/include/linux/magic.h ${SOURCE_DIR}/magic.c)
target_link_libraries(file magic)
