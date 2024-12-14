# This file (c) 2016-2024 AlertAvert.com.  All rights reserved.

cmake_minimum_required(VERSION 3.23)
project(distlib)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

##
# Include common cmake utilities.
# See: https://github.com/massenz/common-utils
#
include(${COMMON_UTILS_DIR}/commons.cmake OPTIONAL RESULT_VARIABLE COMMONS_FILE)
message(STATUS "Using Common Utilities: ${COMMONS_FILE}")

##
# Installation directory; this is both used to access
# required libraries as well as to install the compiled artifacts.
#
# The INSTALL_DIR env variable must be set, unless -DINSTALL_DIR is used.
#
if(NOT DEFINED INSTALL_DIR)
    if(DEFINED ENV{INSTALL_DIR})
        set(INSTALL_DIR $ENV{INSTALL_DIR})
        message(STATUS "INSTALL_DIR set to: ${INSTALL_DIR}")
    else()
        message(FATAL_ERROR
                "INSTALL_DIR should be defined as an environment variable, or via -D notation")
    endif()
endif()

##
# Release version management
#
set(RELEASE_MAJOR 0)
set(RELEASE_MINOR 20)
set(RELEASE_PATCH 0)

set(RELEASE "${RELEASE_MAJOR}.${RELEASE_MINOR}.${RELEASE_PATCH}")
if(DEFINED COMMONS_FILE)
    get_build_id(BUILD_ID)
    set(RELEASE_STRING "${RELEASE}-${BUILD_ID}")
else()
    set(RELEASE_STRING "${RELEASE}")
endif()
message(STATUS "Building Release: ${RELEASE_STRING} (${CMAKE_BUILD_TYPE})")

##
# Depending on the build type (Debug/Release) Google Logging
# generates a differently-named library.
# Both the value of CMAKE_BUILD_TYPE and the `build_type` setting
# in `~/.conan/profiles/default` need to match.
#
# TODO: review to confirm whether this is still necessary with Conan V2
#if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
#    set(GLOG glogd)
#else()
#    set(GLOG glog)
#endif()

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(TESTS_DIR ${PROJECT_SOURCE_DIR}/tests)
set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)

# Configure a header file to pass settings into source code.
configure_file(
    ${SOURCE_DIR}/config.h.in
    ${PROJECT_BINARY_DIR}/version.h
)

include_directories(
        ${INCLUDE_DIR}
        ${PROJECT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}
        ${INSTALL_DIR}/include
)

#link_directories(
#        ${INSTALL_DIR}/lib
#        ${PROJECT_BINARY_DIR}/lib
#)

set(UTILS_SOURCES
        ${SOURCE_DIR}/utils/misc.cpp
        ${SOURCE_DIR}/utils/network.cpp
        ${SOURCE_DIR}/utils/ParseArgs.cpp)

set(SOURCES
        ${SOURCE_DIR}/Bucket.cpp
        ${SOURCE_DIR}/ConsistentHash.cpp
        ${SOURCE_DIR}/View.cpp
)

# Add Conan build folder to CMAKE_PREFIX_PATH for dependency resolution
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

find_package(cryptopp REQUIRED)
find_package(glog)
find_package(OpenSSL REQUIRED)
set(LIBS
        cryptopp::cryptopp
        glog::glog
        openssl::openssl
        pthread
)

##
# libdistutils.so
#
# Shared (dynamic) library for various Distributed facilities.
#
add_library(distutils SHARED
        ${SOURCES}
        ${UTILS_SOURCES}
)
target_link_libraries(distutils ${LIBS})


##
# Installation
#
message("Installing shared library libdistutils.so to ${INSTALL_DIR}/lib")
install(TARGETS distutils DESTINATION ${INSTALL_DIR}/lib)
config_install(${INSTALL_DIR} INSTALL_RESULT)

##
# Unit tests build is defined in the tests/ folder
add_subdirectory(${TESTS_DIR})

#########
# Examples - demo execs/programs to show usage of the libraries/utilities.
#########

set(EXAMPLES_DIR ${SOURCE_DIR}/examples)

message(STATUS "Libraries will be linked from
    ${CMAKE_BINARY_DIR} and ${INSTALL_DIR}/lib")
link_directories(
        ${CMAKE_BINARY_DIR}
        ${INSTALL_DIR}/lib
)

##
# Merkle Tree Demo
#
add_executable(merkle_demo ${EXAMPLES_DIR}/merkle_demo.cpp)
target_link_libraries(merkle_demo distutils ${LIBS})

##
# KeyValue Store Demo
#
add_executable(keystore_demo ${EXAMPLES_DIR}/keystore_example.cpp)
target_link_libraries(keystore_demo distutils ${LIBS})
