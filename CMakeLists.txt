# Copyright (c) 2022 Carlos Reyes
# This code is licensed under the permissive MIT License (MIT).
# SPDX-License-Identifier: MIT-Modern-Variant
#
# Permission is hereby granted, without written agreement and without
# license or royalty fees, to use, copy, modify, and distribute this
# software and its documentation for any purpose, provided that the
# above copyright notice and the following two paragraphs appear in
# all copies of this software.
#
# IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
# ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
# IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.
#
# THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
# BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
# ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
# PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

# versions supported by CLion 2022.1.3 are 2.8.11 to 3.22.x
# CMake 3.12 introduced support for C++20
cmake_minimum_required(VERSION 3.22)

# set defaults for all targets
set(CMAKE_CXX_STANDARD 20)            # set C++20 as the default
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # require C++20
set(CMAKE_CXX_EXTENSIONS OFF)         # turn off GNU compiler extensions

project(Giopler
  VERSION 0.1.1
  DESCRIPTION "A profiling, logging, and testing library for C++20"
  HOMEPAGE_URL "https://github.com/gioppler/gioppler"
  LANGUAGES CXX)

include(CTest)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules;${CMAKE_MODULE_PATH}")

# ------------------------------------------------------------------------------
# https://cmake.org/cmake/help/latest/module/FindThreads.html
set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND GIOPLER_LIBRARIES Threads::Threads)
if(NOT Threads_FOUND)
message(ERROR A threading library is required.)
endif()

# ------------------------------------------------------------------------------
# https://cmake.org/cmake/help/latest/module/FindOpenSSL.html
find_package(OpenSSL REQUIRED COMPONENTS Crypto SSL)
include_directories(${OPENSSL_INCLUDE_DIR})
list(APPEND GIOPLER_LIBRARIES ${OPENSSL_LIBRARIES})

# ------------------------------------------------------------------------------
find_package(Brotli QUIET)
if(BROTLI_FOUND)
  include_directories(${BROTLI_INCLUDE_DIRS})
  list(APPEND GIOPLER_LIBRARIES ${BROTLI_LIBRARIES})
  add_compile_definitions(PRIVATE GIOPLER_HAVE_BROTLI=1)
else()
  message(WARNING "Brotli library not found. Although not required, it is very helpful when communicating with the Giopler server.")
endif()

# ------------------------------------------------------------------------------
# http://mariobadr.com/creating-a-header-only-library-with-cmake.html
# https://dominikberner.ch/cmake-interface-lib/
# https://www.cppengineer.com/blog/using-cmake-to-create-header-only-shared-and-static-libraries
add_library(giopler INTERFACE)
target_include_directories(giopler INTERFACE "$<BUILD_INTERFACE:${Giopler_SOURCE_DIR}/include>")
list(APPEND GIOPLER_LIBRARIES giopler)

# Usage:
# target_link_libraries(your_app_or_lib PRIVATE ${GIOPLER_LIBS})

# ------------------------------------------------------------------------------
file(GLOB TEST_SOURCE_LIST CONFIGURE_DEPENDS "${Giopler_SOURCE_DIR}/test/*.cpp")
add_executable(giopler_test "${TEST_SOURCE_LIST}")
#target_include_directories(giopler_test PUBLIC $<BUILD_INTERFACE:${Giopler_SOURCE_DIR}/test>)
target_compile_options(giopler_test PRIVATE -Werror -Wall)
target_link_libraries(giopler_test PRIVATE ${GIOPLER_LIBRARIES})
target_link_libraries(giopler_test PRIVATE m)

# ------------------------------------------------------------------------------
file(GLOB SIMPLE_SOURCE_LIST CONFIGURE_DEPENDS "${Giopler_SOURCE_DIR}/sample/simple.cpp")
add_executable(simple "${SIMPLE_SOURCE_LIST}")
#target_include_directories(simple PUBLIC $<BUILD_INTERFACE:${Giopler_SOURCE_DIR}/sample>)
target_compile_options(simple PRIVATE -Werror -Wall)
target_link_libraries(simple PRIVATE ${GIOPLER_LIBRARIES})
target_link_libraries(simple PRIVATE Threads::Threads)
target_link_libraries(simple PRIVATE m)

# ------------------------------------------------------------------------------
file(GLOB MAT_MULT_SOURCE_LIST CONFIGURE_DEPENDS "${Giopler_SOURCE_DIR}/sample/matrix_mult.cpp")
add_executable(matrix_mult "${MAT_MULT_SOURCE_LIST}")
#target_include_directories(matrix_mult PUBLIC $<BUILD_INTERFACE:${Giopler_SOURCE_DIR}/sample>)
target_compile_options(matrix_mult PRIVATE -Werror -Wall)
target_link_libraries(matrix_mult PRIVATE ${GIOPLER_LIBRARIES})
target_link_libraries(matrix_mult PRIVATE m)

# ------------------------------------------------------------------------------
file(GLOB THREADS_SOURCE_LIST CONFIGURE_DEPENDS "${Giopler_SOURCE_DIR}/sample/threads.cpp")
add_executable(threads "${THREADS_SOURCE_LIST}")
#target_include_directories(threads PUBLIC $<BUILD_INTERFACE:${Giopler_SOURCE_DIR}/sample>)
target_compile_options(threads PRIVATE -Werror -Wall)
target_link_libraries(threads PRIVATE ${GIOPLER_LIBRARIES})
target_link_libraries(threads PRIVATE m)

# ------------------------------------------------------------------------------
add_compile_definitions(PRIVATE GIOPLER_BUILD_MODE_PROF=1)

# ------------------------------------------------------------------------------
if(BUILD_TESTING)
  add_test(NAME ackermann_ackermann4_1 COMMAND gioppler_test ackermann ackermann4_1)
endif()
