# This file (c) 2016-2020 AlertAvert.com.  All rights reserved.

project(distlib)
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC")

# Conan Packaging support
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

##
# Include common cmake utilities.
# See: https://bitbucket.org/marco/common-utils
#
include(${COMMON_UTILS_DIR}/commons.cmake OPTIONAL RESULT_VARIABLE COMMONS_FILE)

##
# Installation directory; this is both used to access
# required libraries as well as to install the compiled artifacts.
#
# The INSTALL_DIR env variable must be set, unless -DINSTALL_DIR is used.
#
if(NOT DEFINED INSTALL_DIR)
    if(DEFINED ENV{INSTALL_DIR})
        set(INSTALL_DIR $ENV{INSTALL_DIR})
    else()
        message(FATAL_ERROR
                "INSTALL_DIR should be defined as an environment variable, or via -D notation")
    endif()
endif()

##
# Release version management
#
set(RELEASE_MAJOR 0)
set(RELEASE_MINOR 17)
set(RELEASE_PATCH 1)

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
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(GLOG glogd)
else()
    set(GLOG glog)
endif()

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

link_directories(
        ${INSTALL_DIR}/lib
        ${PROJECT_BINARY_DIR}/lib
)

set(UTILS_SOURCES
        ${SOURCE_DIR}/utils/misc.cpp
        ${SOURCE_DIR}/utils/network.cpp
        ${SOURCE_DIR}/utils/ParseArgs.cpp)

set(SOURCES
        ${SOURCE_DIR}/Bucket.cpp
        ${SOURCE_DIR}/ConsistentHash.cpp
        ${SOURCE_DIR}/View.cpp
)

set(UTILS_LIBS
        crypto
        ${GLOG}
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
target_link_libraries(distutils ${UTILS_LIBS})


##
# Installation
#
message("Installing shared library libdistutils.so to ${INSTALL_DIR}/lib")
install(TARGETS distutils DESTINATION ${INSTALL_DIR}/lib)
config_install(${INSTALL_DIR} INSTALL_RESULT)


## Generated configuration file, containing version and build number.
#install(FILES ${PROJECT_BINARY_DIR}/version.h DESTINATION
#        "${INSTALL_DIR}/include/${PROJECT_NAME}")
#
## Install Library headers.
#install(DIRECTORY ${INCLUDE_DIR}/ DESTINATION ${INSTALL_DIR}/include/${PROJECT_NAME}
#        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
#
## Install all dependencies
#install(DIRECTORY ${PROJECT_BINARY_DIR}/include DESTINATION
#        ${INSTALL_DIR}
#        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
#install(DIRECTORY ${PROJECT_BINARY_DIR}/lib DESTINATION
#        ${INSTALL_DIR}
#        FILES_MATCHING PATTERN "*.so*" PATTERN "*.dylib*")

##
# Unit tests build is defined in the tests/ folder
add_subdirectory(${TESTS_DIR})

#########
# Examples - demo execs/programs to show usage of the libraries/utilities.
#########

set(EXAMPLES_DIR ${SOURCE_DIR}/examples)

# Examples will use the latest build for the shared libraries
link_directories(
        ${CMAKE_BINARY_DIR}/lib
        ${INSTALL_DIR}/lib
)

##
# Merkle Tree Demo
#
add_executable(merkle_demo ${EXAMPLES_DIR}/merkle_demo.cpp)
target_link_libraries(merkle_demo distutils ${UTILS_LIBS})

##
# KeyValue Store Demo
#
add_executable(keystore_demo ${EXAMPLES_DIR}/keystore_example.cpp)
target_link_libraries(keystore_demo distutils ${UTILS_LIBS})
