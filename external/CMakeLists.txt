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

# Disabled threading support for rlottie as this functionality actually leads to far worse
# performance. As well there is a bug on Windows that makes rlottie hang forever on application
# shutdown if compiled using MinGW with threading support enabled.
option(LOTTIE_THREAD OFF)

option(LOTTIE_MODULE OFF)

# Only use the compiler and linker flags defined by rlottie.
unset(CMAKE_CXX_FLAGS)
unset(CMAKE_EXE_LINKER_FLAGS)

if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS -pthread)
endif()

add_subdirectory(rlottie EXCLUDE_FROM_ALL)
