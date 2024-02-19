#  SPDX-License-Identifier: MIT
#
#  ES-DE
#  CMakeLists.txt (external)
#
#  CMake configuration for bundled dependencies built in-tree.
#

# This makes it possible to set options in subprojects.
set (CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# Suppress warnings about cmake_minimum_required() defining deprecated versions.
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

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

if (WIN32)
    set(BUILD_SHARED_LIBS ON)
else()
    set(BUILD_SHARED_LIBS OFF)
endif()

add_subdirectory(lunasvg EXCLUDE_FROM_ALL)

# Disable LunaSVG DLL interface warnings and narrowing conversion warnings.
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    target_compile_options(lunasvg PRIVATE "/wd4251" "/wd4267")
endif()

if(ANDROID)
    if(ANDROID_LITE_RELEASE)
        set_target_properties(lunasvg PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
                              ${PROJECT_SOURCE_DIR}/android_lite_${ANDROID_ABI})
    else()
        set_target_properties(lunasvg PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
                              ${PROJECT_SOURCE_DIR}/android_${ANDROID_ABI})
    endif()
endif()

# Disable threading support for rlottie as this functionality actually leads to far worse
# performance. As well there is a bug on Windows that makes rlottie hang forever on application
# shutdown if compiled using MinGW with threading support enabled.
option(LOTTIE_THREAD OFF)

option(LOTTIE_MODULE OFF)

if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS -pthread)
endif()

add_subdirectory(rlottie EXCLUDE_FROM_ALL)

# rlottie generates a lot of annoying compiler warnings that we don't need to show.
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    target_compile_options(rlottie PRIVATE "/wd4244" "/wd4251" "/wd4263" "/wd4334" "/wd4267" "/wd4530" "/wd4996")
else()
    target_compile_options(rlottie PRIVATE "-w")
endif()

if(ANDROID)
    if(ANDROID_LITE_RELEASE)
        set_target_properties(rlottie PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
                              ${PROJECT_SOURCE_DIR}/android_lite_${ANDROID_ABI})
    else()
        set_target_properties(rlottie PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
                              ${PROJECT_SOURCE_DIR}/android_${ANDROID_ABI})
    endif()
endif()

# Build LunaSVG before rlottie.
add_dependencies(rlottie lunasvg)
