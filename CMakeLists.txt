# This file (c) 2016-2017 AlertAvert.com.  All rights reserved.

project(distlib)
cmake_minimum_required(VERSION 3.4)


# Include common cmake utilities:
include(${COMMON_UTILS_DIR}/commons.cmake OPTIONAL RESULT_VARIABLE COMMONS_FILE)

### Setting up common configuration variables

# Version number
set(RELEASE_MAJOR 0)
set(RELEASE_MINOR 7)
set(RELEASE_PATCH 0)

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(TESTS_DIR ${PROJECT_SOURCE_DIR}/tests)
set(PROTO_DIR ${PROJECT_SOURCE_DIR}/proto)
set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)

# Configure a header file to pass settings into source code.
configure_file(
    ${SOURCE_DIR}/config.h.in
    ${PROJECT_BINARY_DIR}/version.h
)

# Building Protocol Buffers.
#
find_package(Protobuf REQUIRED)

IF(${PROTOBUF_FOUND})
    PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_DIR}/swim.proto)
    message(STATUS "When generated, Protocol Buffer files will be in:\n"
                   "\tHeaders: ${PROTO_HDRS}\n\tSources: ${PROTO_SRCS}")
ELSE(${PROTOBUF_FOUND})
    message(FATAL_ERROR "Could not find Google Protocol Buffers libraries")
ENDIF(${PROTOBUF_FOUND})

include_directories(
        ${SOURCE_DIR}
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

set(SWIM_LIBS
        crypto
        glog
        protobuf
        pthread
        zmq
)

# Create a shared (dynamic) library.
add_library(${PROJECT_NAME} SHARED
        ${SOURCE_FILES}
        ${SWIM_SOURCES}
        ${PROTO_SRCS}
)
target_link_libraries(${PROJECT_NAME}
        ${SWIM_LIBS})

if (NOT ${INSTALL_DIR})
    message(STATUS "Installing lib${PROJECT_NAME}.a static library to ${INSTALL_DIR}/lib\n"
                   "   and include files to ${INSTALL_DIR}/include/${PROJECT_NAME}")
    install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_DIR}/lib)

    # Generated configuration file, containing version and build number.
    install(FILES ${PROJECT_BINARY_DIR}/version.h DESTINATION
            "${INSTALL_DIR}/include/${PROJECT_NAME}")

    # Install Library headers.
    install(DIRECTORY ${SOURCE_DIR}/ DESTINATION ${INSTALL_DIR}/include/${PROJECT_NAME}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

    # Copy Proto headers to include folder.
    install(FILES ${PROTO_HDRS} DESTINATION ${INSTALL_DIR}/include/${PROJECT_NAME}/proto)

else (NOT ${INSTALL_DIR})
    message(WARNING "INSTALL_DIR is not defined, files will not be installed."
                    " Use -DINSTALL_DIR=/path/to/install to enable")
endif (NOT ${INSTALL_DIR})

# Example creating an exec using the shared lib.
add_executable(merkle_demo ${SOURCE_DIR}/merkle_demo.cpp)
target_link_libraries(merkle_demo
        ${PROJECT_NAME}
)

# Adding a target demo server that will use the SWIM protocol to derive
# a list of healthy server from a configured pool.
link_directories(${INSTALL_DIR}/lib)
add_executable(swim_server_demo
        ${SOURCE_DIR}/server_demo.cpp
)
target_link_libraries(swim_server_demo
        ${PROJECT_NAME}
)


# Unit tests build is defined in the tests/ folder
add_subdirectory(${TESTS_DIR})
