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

# There is a bug that makes rlottie hang forever on application shutdown if compiled using MinGW
# with threading support enabled. Disable it for MSVC as well to keep the behavior consistent
# across builds with either of these compilers.
if(WIN32)
    option(LOTTIE_THREAD OFF)
endif()

option(LOTTIE_MODULE OFF)

# Only use the compiler and linker flags defined by rlottie.
unset(CMAKE_CXX_FLAGS)
unset(CMAKE_EXE_LINKER_FLAGS)

add_subdirectory("rlottie" EXCLUDE_FROM_ALL)
