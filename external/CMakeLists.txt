#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  CMakeLists.txt (external)
#
#  CMake configuration for bundled dependencies built in-tree.
#

set(BUILD_SHARED_LIBS OFF)
option(LOTTIE_MODULE OFF)
add_subdirectory("rlottie")
