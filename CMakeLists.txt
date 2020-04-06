# This file (c) 2016-2017 AlertAvert.com.  All rights reserved.

project(distlib)
cmake_minimum_required(VERSION 3.4)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fPIC")


# Include common cmake utilities:
include(${COMMON_UTILS_DIR}/commons.cmake OPTIONAL RESULT_VARIABLE COMMONS_FILE)

##
#
# Installation directory; this is both used to access
# required libraries as well as to install the compiled artifacts.
#
# The INSTALL_DIR env variable must be set, unless -DINSTALL_DIR is used.
if(NOT DEFINED INSTALL_DIR)
    if(DEFINED ENV{INSTALL_DIR})
        set(INSTALL_DIR $ENV{INSTALL_DIR})
    else()
        message(FATAL_ERROR
                "INSTALL_DIR should be defined as an environment variable, or via -D notation")
    endif()
endif()

### Setting up common configuration variables

# Version number
set(RELEASE_MAJOR 0)
set(RELEASE_MINOR 14)
set(RELEASE_PATCH 0)

IF (${COMMONS_FILE} STREQUAL NOTFOUND)
    message(WARNING "Not using CMake utilities")
    set(BUILD_ID 0)
ELSE (${COMMONS_FILE} STREQUAL NOTFOUND)
    get_build_id(BUILD_ID)
ENDIF (${COMMONS_FILE} STREQUAL NOTFOUND)

# Conan Packaging support
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()



set(RELEASE_STRING "${RELEASE_MAJOR}.${RELEASE_MINOR}.${RELEASE_PATCH}-${BUILD_ID}")
message(STATUS "Building Release: ${RELEASE_STRING} (${CMAKE_BUILD_TYPE})")

##
# Depending on the build type (Debug/Release) Google Logging
# generates a differently-named library.
# Both the value of CMAKE_BUILD_TYPE and the `build_type` setting
# in `~/.conan/profiles/default` need to match.
#
message("Build type: ${CMAKE_BUILD_TYPE}")

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
)

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


if (NOT ${INSTALL_DIR})
    message(STATUS "Installing shared libraries to ${INSTALL_DIR}/lib "
                   "and include files to ${INSTALL_DIR}/include/${PROJECT_NAME}")
    install(TARGETS distutils DESTINATION ${INSTALL_DIR}/lib)

    # Generated configuration file, containing version and build number.
    install(FILES ${PROJECT_BINARY_DIR}/version.h DESTINATION
            "${INSTALL_DIR}/include/${PROJECT_NAME}")

    # Install Library headers.
    install(DIRECTORY ${INCLUDE_DIR}/ DESTINATION ${INSTALL_DIR}/include/${PROJECT_NAME}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

else (NOT ${INSTALL_DIR})
    message(WARNING "INSTALL_DIR is not defined, files will not be installed."
                    " Use -DINSTALL_DIR=/path/to/install to enable")
endif (NOT ${INSTALL_DIR})

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
