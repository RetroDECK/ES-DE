#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  CMakeLists.txt (external)
#
#  CMake configuration for bundled dependencies built in-tree.
#

# This makes it possible to set options in subprojects.
set (CMAKE_POLICY_DEFAULT_CMP0077 NEW)

if (WIN32)
    # On Windows the build type needs to match the main binary.
    if (CMAKE_BUILD_TYPE MATCHES Profiling)
        set (CMAKE_BUILD_TYPE Release)
    elseif(NOT CMAKE_BUILD_TYPE MATCHES Debug)
        set (CMAKE_BUILD_TYPE Release)
    endif()
else()
    # Always build with optimizations enabled and without debug info.
    set (CMAKE_BUILD_TYPE Release)
endif()

unset(CMAKE_CXX_FLAGS)
unset(CMAKE_EXE_LINKER_FLAGS)

if(WIN32)
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
        # Disable DLL interface warnings.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251")
    else()
        # Strip the DLL files when building with MinGW.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
    endif()
endif()

if (WIN32)
    set(BUILD_SHARED_LIBS ON)
else()
    set(BUILD_SHARED_LIBS OFF)
endif()

add_subdirectory(lunasvg EXCLUDE_FROM_ALL)

# It's quite strange that this is needed but LunaSVG saves these artifacts to separate
# subdirectories by default.
set_target_properties(lunasvg
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

# Disable threading support for rlottie as this functionality actually leads to far worse
# performance. As well there is a bug on Windows that makes rlottie hang forever on application
# shutdown if compiled using MinGW with threading support enabled.
option(LOTTIE_THREAD OFF)

option(LOTTIE_MODULE OFF)

if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS -pthread)
endif()

# rlottie generates a lot of annoying compiler warnings that we don't need to show.
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4267 /wd4530 /wd4996")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
endif()

add_subdirectory(rlottie EXCLUDE_FROM_ALL)

# Build LunaSVG before rlottie.
add_dependencies(rlottie lunasvg)
