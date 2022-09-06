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

project(Gioppler
  VERSION 0.1.1
  DESCRIPTION "A profiling, logging, and testing library for C++20"
  HOMEPAGE_URL "https://github.com/gioppler/gioppler"
  LANGUAGES CXX)

include(CTest)

# ------------------------------------------------------------------------------
# http://mariobadr.com/creating-a-header-only-library-with-cmake.html
# https://dominikberner.ch/cmake-interface-lib/
# https://www.cppengineer.com/blog/using-cmake-to-create-header-only-shared-and-static-libraries
add_library(gioppler INTERFACE)
target_include_directories(gioppler INTERFACE "$<BUILD_INTERFACE:${Gioppler_SOURCE_DIR}/include>")

# Usage:
# target_link_libraries(your_app_or_lib PRIVATE gioppler)

# ------------------------------------------------------------------------------
file(GLOB TEST_SOURCE_LIST CONFIGURE_DEPENDS "${Gioppler_SOURCE_DIR}/test/*.cpp")
add_executable(gioppler_test "${TEST_SOURCE_LIST}")
#target_include_directories(gioppler_test PUBLIC $<BUILD_INTERFACE:${Gioppler_SOURCE_DIR}/test>)
target_compile_options(gioppler_test PRIVATE -Werror -Wall)
target_link_libraries(gioppler_test PRIVATE m)
target_link_libraries(gioppler_test PRIVATE gioppler)

# ------------------------------------------------------------------------------
file(GLOB SIMPLE_SOURCE_LIST CONFIGURE_DEPENDS "${Gioppler_SOURCE_DIR}/sample/simple.cpp")
add_executable(simple "${SIMPLE_SOURCE_LIST}")
#target_include_directories(simple PUBLIC $<BUILD_INTERFACE:${Gioppler_SOURCE_DIR}/sample>)
target_compile_options(simple PRIVATE -Werror -Wall)
target_link_libraries(simple PRIVATE m)
target_link_libraries(simple PRIVATE gioppler)

# ------------------------------------------------------------------------------
file(GLOB MAT_MULT_SOURCE_LIST CONFIGURE_DEPENDS "${Gioppler_SOURCE_DIR}/sample/matrix_mult.cpp")
add_executable(matrix_mult "${MAT_MULT_SOURCE_LIST}")
#target_include_directories(matrix_mult PUBLIC $<BUILD_INTERFACE:${Gioppler_SOURCE_DIR}/sample>)
target_compile_options(matrix_mult PRIVATE -Werror -Wall)
target_link_libraries(matrix_mult PRIVATE m)
target_link_libraries(matrix_mult PRIVATE gioppler)

# ------------------------------------------------------------------------------
file(GLOB THREADS_SOURCE_LIST CONFIGURE_DEPENDS "${Gioppler_SOURCE_DIR}/sample/threads.cpp")
add_executable(threads "${THREADS_SOURCE_LIST}")
#target_include_directories(threads PUBLIC $<BUILD_INTERFACE:${Gioppler_SOURCE_DIR}/sample>)
target_compile_options(threads PRIVATE -Werror -Wall)
target_link_libraries(threads PRIVATE m)
target_link_libraries(threads PRIVATE gioppler)

# ------------------------------------------------------------------------------
add_compile_definitions(PRIVATE GIOPPLER_BUILD_MODE_DEV=1)

# ------------------------------------------------------------------------------
if(BUILD_TESTING)
  add_test(NAME ackermann_ackermann4_1 COMMAND gioppler_test ackermann ackermann4_1)
endif()
