#  SPDX-License-Identifier: MIT
#
#  ES-DE
#  CMakeLists.txt (qrcodegen)
#
#  CMake configuration for the vendored qrcodegen QR Code generator library.
#

cmake_minimum_required(VERSION 3.10)
project(qrcodegen C)

add_library(qrcodegen STATIC qrcodegen.c)
target_include_directories(qrcodegen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
