# This file (c) 2016 AlertAvert.com.  All rights reserved.

project(distlib)
cmake_minimum_required(VERSION 3.4)


# Include common cmake utilities:
include(${COMMON_UTILS_DIR}/commons.cmake OPTIONAL RESULT_VARIABLE COMMONS_FILE)
include(FindProtobuf)

### Setting up common configuration variables

# Version number
set(RELEASE_MAJOR 0)
set(RELEASE_MINOR 6)
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

IF(${PROTOBUF_FOUND})
    PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_DIR}/swim.proto)
    message(STATUS "Generated Protocol Buffer\n\tHeaders: ${PROTO_HDRS}\n\tSources: ${PROTO_SRCS}")
ELSE(${PROTOBUF_FOUND})
    message(FATAL_ERROR "Could not find Google Protocol Buffers libraries")
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
        ${SOURCE_DIR}/ConsistentHash.cpp
        ${SOURCE_DIR}/Bucket.cpp
        ${SOURCE_DIR}/View.cpp
)

set(SWIM_SOURCES
        ${SOURCE_DIR}/swim/SwimClient.cpp
        ${SOURCE_DIR}/swim/SwimServer.cpp
        ${SOURCE_DIR}/swim/GossipFailureDetector.cpp
        ${SOURCE_DIR}/utils/network.cpp
)

add_executable(merkle_demo ${SOURCE_FILES} ${SOURCE_DIR}/main.cpp)
target_link_libraries(merkle_demo ${CONAN_LIBS})

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


# Unit tests build is defined in the tests/ folder
add_subdirectory(${TESTS_DIR})
