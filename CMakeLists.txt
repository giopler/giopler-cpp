# Copyright (c) 2023 Giopler
# Creative Commons Attribution No Derivatives 4.0 International license
# https://creativecommons.org/licenses/by-nd/4.0
# SPDX-License-Identifier: CC-BY-ND-4.0
#
# Share         — copy and redistribute the material in any medium or format for any purpose, even commercially.
# NoDerivatives — If you remix, transform, or build upon the material, you may not distribute the modified material.
# Attribution   — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
#                 You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

# Our target system is the current long-term support version of Ubuntu (22.04).
# Ubuntu 22.04LTS includes CMake 3.22, CLang 14.0, Gcc 11.2, and OpenSSL 3.0.

# Versions of CMake supported by CLion 2022.1.3 are 2.8.11 to 3.22.x.
cmake_minimum_required(VERSION 3.22)

# set defaults for all targets
set(CMAKE_CXX_STANDARD 20)            # set C++20 as the default
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # require C++20
set(CMAKE_CXX_EXTENSIONS OFF)         # turn off GNU compiler extensions

project(Giopler-Cpp
  VERSION 0.2.0
  DESCRIPTION "A profiling, logging, and testing library for C++20"
  HOMEPAGE_URL "https://github.com/giopler/giopler-cpp"
  LANGUAGES CXX)

include(CTest)

# need our own FindBrotli.cmake file
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ------------------------------------------------------------------------------
# https://cmake.org/cmake/help/latest/module/FindThreads.html
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(NOT Threads_FOUND)
message(ERROR A threading library is required.)
endif()

# ------------------------------------------------------------------------------
# https://cmake.org/cmake/help/latest/module/FindOpenSSL.html
find_package(OpenSSL REQUIRED COMPONENTS Crypto SSL)
if(NOT OpenSSL_FOUND)
message(ERROR OpenSSL library is required.)
endif()

# ------------------------------------------------------------------------------
# cmake/FindBrotli.cmake
# https://github.com/google/brotli/
find_package(Brotli REQUIRED)
if(NOT BROTLI_FOUND)
message(ERROR Brotli compression library is required.)
endif()

# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# Giopler is a header-only library
# http://mariobadr.com/creating-a-header-only-library-with-cmake.html
# https://dominikberner.ch/cmake-interface-lib/
# https://www.cppengineer.com/blog/using-cmake-to-create-header-only-shared-and-static-libraries
# Usage:   target_link_libraries(your_app_or_lib PRIVATE ${GIOPLER_LIBRARIES})
add_library(giopler INTERFACE)
target_include_directories(giopler INTERFACE
        ${OPENSSL_INCLUDE_DIR}
        ${BROTLI_INCLUDE_DIRS}
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(giopler INTERFACE
        Threads::Threads
        ${OPENSSL_LIBRARIES}
        ${BROTLI_LIBRARIES}
)

# ------------------------------------------------------------------------------
add_executable(simple "${CMAKE_CURRENT_SOURCE_DIR}/sample/simple.cpp")
target_compile_options(simple PRIVATE -Werror -Wall)
target_link_libraries(simple PRIVATE giopler m)

# ------------------------------------------------------------------------------
add_executable(hello "${CMAKE_CURRENT_SOURCE_DIR}/sample/hello.cpp")
target_compile_options(hello PRIVATE -Werror -Wall)
target_link_libraries(hello PRIVATE giopler m)

# ------------------------------------------------------------------------------
add_executable(there "${CMAKE_CURRENT_SOURCE_DIR}/sample/there.cpp")
target_compile_options(there PRIVATE -Werror -Wall)
target_link_libraries(there PRIVATE giopler m)

# ------------------------------------------------------------------------------
add_executable(threads "${CMAKE_CURRENT_SOURCE_DIR}/sample/threads.cpp")
target_compile_options(threads PRIVATE -Werror -Wall)
target_link_libraries(threads PRIVATE giopler m)

# ------------------------------------------------------------------------------
add_executable(factorial "${CMAKE_CURRENT_SOURCE_DIR}/sample/factorial.cpp")
target_compile_options(factorial PRIVATE -Werror -Wall)
target_link_libraries(factorial PRIVATE giopler m)

# ------------------------------------------------------------------------------
add_executable(ackermann "${CMAKE_CURRENT_SOURCE_DIR}/sample/ackermann.cpp")
target_compile_options(ackermann PRIVATE -Werror -Wall)
target_link_libraries(ackermann PRIVATE giopler m)

# ------------------------------------------------------------------------------
add_executable(fibonacci "${CMAKE_CURRENT_SOURCE_DIR}/sample/fibonacci.cpp")
target_compile_options(fibonacci PRIVATE -Werror -Wall)
target_link_libraries(fibonacci PRIVATE giopler m)

# ------------------------------------------------------------------------------
add_executable(collatz "${CMAKE_CURRENT_SOURCE_DIR}/sample/collatz.cpp")
target_compile_options(collatz PRIVATE -Werror -Wall)
target_link_libraries(collatz PRIVATE giopler m)

# ------------------------------------------------------------------------------
add_executable(matrices "${CMAKE_CURRENT_SOURCE_DIR}/sample/matrices.cpp")
target_compile_options(matrices PRIVATE -Werror -Wall)
target_link_libraries(matrices PRIVATE giopler m)

# ------------------------------------------------------------------------------
#add_compile_definitions(PRIVATE GIOPLER_BUILD_MODE_DEV=1)
add_compile_definitions(PRIVATE GIOPLER_BUILD_MODE_PROF=1)

# ------------------------------------------------------------------------------
if(BUILD_TESTING)
  add_test(NAME ackermann_ackermann4_1 COMMAND giopler_test ackermann ackermann4_1)
endif()
