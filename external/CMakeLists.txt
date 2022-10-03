#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  CMakeLists.txt (external)
#
#  CMake configuration for bundled dependencies built in-tree.
#

if (CMAKE_BUILD_TYPE MATCHES Profiling)
    set (CMAKE_BUILD_TYPE Debug)
elseif(NOT CMAKE_BUILD_TYPE MATCHES Debug)
    set (CMAKE_BUILD_TYPE Release)
endif()

# Disabled threading support for rlottie as this functionality actually leads to far worse
# performance. As well there is a bug on Windows that makes rlottie hang forever on application
# shutdown if compiled using MinGW with threading support enabled.
option(LOTTIE_THREAD OFF)

option(LOTTIE_MODULE OFF)

# Only use the compiler and linker flags defined by LunaSVG and rlottie.
unset(CMAKE_CXX_FLAGS)
unset(CMAKE_EXE_LINKER_FLAGS)

add_subdirectory(lunasvg)

# On Windows, rlottie is built as a DLL file.
if (WIN32)
    set(BUILD_SHARED_LIBS ON)
else()
    set(BUILD_SHARED_LIBS OFF)
endif()

if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS -pthread)
endif()

# rlottie generates a lot of annoying compiler warnings that we don't need to show.
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    set(CMAKE_CXX_FLAGS "/wd4244 /wd4251 /wd4267 /wd4530 /wd4996")
else()
    set(CMAKE_CXX_FLAGS "-w")
endif()

add_subdirectory(rlottie EXCLUDE_FROM_ALL)

# Build LunaSVG before rlottie.
add_dependencies(rlottie lunasvg)
