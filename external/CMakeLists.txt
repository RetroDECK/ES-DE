#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  CMakeLists.txt (external)
#
#  CMake configuration for bundled dependencies built in-tree.
#

# On Windows, rlottie is built as a DLL file.
if(NOT WIN32)
    set(BUILD_SHARED_LIBS OFF)
endif()

option(LOTTIE_MODULE OFF)
add_subdirectory("rlottie")
