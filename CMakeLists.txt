#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  CMakeLists.txt
#
#  Main CMake configuration file.
#  Sets up the overall build environment including dependencies detection, build options,
#  compiler and linker flags and preprocessor directives.
#

cmake_minimum_required(VERSION 3.13)
if(APPLE)
    # Set this to the operating system version you're building on, and also update
    # es-app/assets/EmulationStation-DE_Info.plist accordingly.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "macOS deployment target")
    # This optional variable is used for code signing the DMG installer.
    set(MACOS_CODESIGN_IDENTITY "" CACHE STRING "macOS code signing certificate identity")
endif()
project(emulationstation-de)

# Set this to ON to show verbose compiler output (e.g. compiler flags, include directories etc.)
set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "Show verbose compiler output" FORCE)

# Package type to use for CPack on Linux.
set(LINUX_CPACK_GENERATOR "DEB" CACHE STRING "CPack generator, DEB or RPM")

# Add local find modules to the CMake path.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Utils
                              ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Packages)

# Define the options.
option(GLES "Set to ON if targeting Embedded OpenGL" ${GLES})
option(GL "Set to ON if targeting Desktop OpenGL" ${GL})
option(RPI "Set to ON to enable Raspberry Pi specific build" ${RPI})
option(CEC "Set to ON to enable CEC" ${CEC})
option(VLC_PLAYER "Set to ON to build the VLC-based video player" ${VLC_PLAYER})
option(CLANG_TIDY "Set to ON to build using the clang-tidy static analyzer" ${CLANG_TIDY})

if(CLANG_TIDY)
    find_program(CLANG_TIDY_BINARY NAMES clang-tidy)
    if(CLANG_TIDY_BINARY STREQUAL "CLANG_TIDY_BINARY-NOTFOUND")
        message("-- CLANG_TIDY was set but the clang-tidy binary was not found")
    else()
        message("-- Building with the clang-tidy static analyzer")
        set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*,\
                                  -fuchsia-*,\
                                  -hicpp-*,\
                                  -llvm-*,\
                                  -readability-braces-*,\
                                  -google-readability-braces-*,\
                                  -readability-uppercase-literal-suffix,\
                                  -modernize-use-trailing-return-type,\
                                  -cppcoreguidelines-avoid-magic-numbers,\
                                  -readability-magic-numbers")
    endif()
endif()

#---------------------------------------------------------------------------------------------------
# OpenGL setup.

# Check if we're running on a Raspberry Pi.
if(EXISTS "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/bcm_host.h")
    message("-- Building on a Raspberry Pi (bcm_host.h found)")
    # Setting BCMHOST seems to break OpenGL ES on the RPi 4 so set RPI instead.
    #set(BCMHOST found)
    set(RPI ON)
    set(GLSYSTEM "Embedded OpenGL" CACHE STRING "The OpenGL system to be used")
elseif(GLES OR RPI)
    set(GLSYSTEM "Embedded OpenGL" CACHE STRING "The OpenGL system to be used")
else()
    set(GLSYSTEM "Desktop OpenGL" CACHE STRING "The OpenGL system to be used")
endif()

set_property(CACHE GLSYSTEM PROPERTY STRINGS "Desktop OpenGL" "Embedded OpenGL")

#---------------------------------------------------------------------------------------------------
# Package dependencies.

if(GLSYSTEM MATCHES "Desktop OpenGL")
    set(OpenGL_GL_PREFERENCE "GLVND")
    find_package(OpenGL REQUIRED)
else()
    find_package(OpenGLES REQUIRED)
endif()

# Skip package dependency checks if we're on Windows.
if(NOT WIN32)
    find_package(CURL REQUIRED)
    find_package(FFmpeg REQUIRED)
    find_package(FreeImage REQUIRED)
    find_package(Freetype REQUIRED)
    find_package(Pugixml REQUIRED)
    find_package(SDL2 REQUIRED)
    if(VLC_PLAYER)
        find_package(VLC REQUIRED)
    endif()
endif()

# Add libCEC support.
if(CEC)
    find_package(libCEC REQUIRED)
endif()

# Add ALSA for Linux.
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(ALSA REQUIRED)
endif()

#---------------------------------------------------------------------------------------------------
# Compiler and linker settings.

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message("-- Compiler is Clang/LLVM")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0.0)
        message(SEND_ERROR "You need at least Clang 5.0.0 to compile EmulationStation-DE")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message("-- Compiler is GNU/GCC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.1)
        message(SEND_ERROR "You need at least GCC 7.1 to compile EmulationStation-DE")
    endif()
    if(WIN32)
        set(CMAKE_CXX_FLAGS "-mwindows ${CMAKE_CXX_FLAGS}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message("-- Compiler is MSVC")
    # If using the MSVC compiler on Windows, disable the built-in min() and max() macros.
    add_definitions(-DNOMINMAX)
endif()

if(CMAKE_BUILD_TYPE)
    message("-- Build type is ${CMAKE_BUILD_TYPE}")
endif()

# Set up compiler and linker flags for debug, profiling or release builds.
if(CMAKE_BUILD_TYPE MATCHES Debug)
    # Enable the C++17 standard and disable optimizations as it's a debug build.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /Od /DEBUG:FULL")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -Wall -Wpedantic -Wsign-compare -Wnarrowing -Wmissing-field-initializers -Wunused-macros")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0")
    endif()
    # If using Clang, then add additional debug data needed by GDB.
    # Comment this out if you're using LLDB for debugging as this flag makes the binary
    # much larger and the application much slower. On macOS this setting is never enabled
    # as LLDB is the default debugger on this OS.
    if(NOT APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG")
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Profiling)
    # For the profiling build, we enable optimizations and supply the required profiler flags.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /O2 /DEBUG:FULL")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -pg -g -Wall -Wpedantic -Wsign-compare -Wnarrowing -Wmissing-field-initializers -Wunused-macros")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O2 -pg")
    endif()
else()
    # Enable the C++17 standard and enable optimizations as it's a release build.
    # This will also disable all assert() macros. Strip the binary too.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG /std:c++17 /O2 /DEBUG:NONE")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -DNDEBUG -Wall -Wpedantic -Wsign-compare -Wnarrowing -Wmissing-field-initializers -Wunused-macros")
        if(APPLE)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O2")
        else()
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O2 -s")
        endif()
    endif()
endif()

# The following removes half of the ranlib warnings on macOS regarding no symbols for files
# that are #ifdef'ed away. There must be a way to remove the other half as well?
if(APPLE)
    SET(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()

if(APPLE)
    if(MACOS_CODESIGN_IDENTITY)
        message("-- Code signing certificate identity: " ${MACOS_CODESIGN_IDENTITY})
    endif()
    if(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS 10.14)
        message("-- macOS version 10.13 or lower has been set, so if code signing is enabled, Hardened Runtime will not be used")
    endif()
endif()

#---------------------------------------------------------------------------------------------------
# Preprocessor directives.

if(GLSYSTEM MATCHES "Desktop OpenGL")
    add_definitions(-DUSE_OPENGL_21)
else()
    add_definitions(-DUSE_OPENGLES_10)
endif()

if(VLC_PLAYER)
    add_definitions(-DBUILD_VLC_PLAYER)
endif()

if(DEFINED BCMHOST OR RPI)
    add_definitions(-D_RPI_)
endif()

if(DEFINED libCEC_FOUND)
    add_definitions(-DHAVE_LIBCEC)
endif()

# GLM library options.
add_definitions(-DGLM_FORCE_CXX17)
add_definitions(-DGLM_FORCE_XYZW_ONLY)

# For Unix systems, assign the installation prefix. If it's not explicitly set,
# we use /usr on Linux, /usr/pkg on NetBSD and /usr/local on FreeBSD and OpenBSD.
if(NOT WIN32 AND NOT APPLE)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        if(CMAKE_SYSTEM_NAME MATCHES "Linux")
            set(CMAKE_INSTALL_PREFIX "/usr" CACHE INTERNAL "CMAKE_INSTALL_PREFIX")
        elseif(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
            set(CMAKE_INSTALL_PREFIX "/usr/pkg" CACHE INTERNAL "CMAKE_INSTALL_PREFIX")
        else()
            set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE INTERNAL "CMAKE_INSTALL_PREFIX")
        endif()
    endif()
    message("-- Installation prefix is set to " ${CMAKE_INSTALL_PREFIX})
    add_definitions(-DES_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
endif()

# For Windows, set the minimum OS version to Windows 7.
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
    add_compile_definitions(WINVER=0x0601)
endif()

#---------------------------------------------------------------------------------------------------
# Include files.

set(COMMON_INCLUDE_DIRS ${CURL_INCLUDE_DIR}
                        ${FFMPEG_INCLUDE_DIRS}
                        ${FreeImage_INCLUDE_DIRS}
                        ${FREETYPE_INCLUDE_DIRS}
                        ${PUGIXML_INCLUDE_DIRS}
                        ${SDL2_INCLUDE_DIR}
                        ${CMAKE_CURRENT_SOURCE_DIR}/external/CImg
                        ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
                        ${CMAKE_CURRENT_SOURCE_DIR}/external/nanosvg/src
                        ${CMAKE_CURRENT_SOURCE_DIR}/external/rapidjson/include
                        ${CMAKE_CURRENT_SOURCE_DIR}/es-core/src)
if(VLC_PLAYER)
    set(COMMON_INCLUDE_DIRS ${COMMON_INCLUDE_DIRS} ${VLC_INCLUDE_DIR})
endif()

# For Windows we need to add local include files for the dependency packages.
if(WIN32)
    set(WIN32_INCLUDE_DIR "NOT_DEFINED" CACHE FILEPATH "")
    if(NOT EXISTS ${WIN32_INCLUDE_DIR})
        message(SEND_ERROR "Can't find WIN32 include directory: ${WIN32_INCLUDE_DIR}")
    endif()
endif()

# Temporary solution until the VLC find module has been updated to work properly on macOS.
if(APPLE AND VLC_PLAYER)
    set(COMMON_INCLUDE_DIRS ${COMMON_INCLUDE_DIRS} "/Applications/VLC.app/Contents/MacOS/include")
endif()

if(WIN32)
    set(COMMON_INCLUDE_DIRS ${COMMON_INCLUDE_DIRS} ${WIN32_INCLUDE_DIR})
endif()

# Add libCEC include directory.
if(DEFINED libCEC_FOUND)
    list(APPEND COMMON_INCLUDE_DIRS ${libCEC_INCLUDE_DIR})
endif()

# For Linux, add the ALSA include directory.
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND COMMON_INCLUDE_DIRS ${ALSA_INCLUDE_DIRS})
endif()

if(DEFINED BCMHOST OR RPI)
    list(APPEND COMMON_INCLUDE_DIRS "${CMAKE_FIND_ROOT_PATH}/opt/vc/include"
                                    "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vcos"
                                    "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vmcs_host/linux"
                                    "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vcos/pthreads")
endif()

#---------------------------------------------------------------------------------------------------
# Dependency libraries.

if(NOT WIN32)
    set(COMMON_LIBRARIES ${CURL_LIBRARIES}
                         ${FFMPEG_LIBRARIES}
                         ${FreeImage_LIBRARIES}
                         ${FREETYPE_LIBRARIES}
                         ${PUGIXML_LIBRARIES}
                         ${SDL2_LIBRARY})
    if(VLC_PLAYER)
        set(COMMON_LIBRARIES ${COMMON_LIBRARIES} ${VLC_LIBRARIES})
    endif()
elseif(WIN32)
    if(DEFINED MSVC)
        set(COMMON_LIBRARIES "${PROJECT_SOURCE_DIR}/avcodec.lib"
                             "${PROJECT_SOURCE_DIR}/avfilter.lib"
                             "${PROJECT_SOURCE_DIR}/avformat.lib"
                             "${PROJECT_SOURCE_DIR}/avutil.lib"
                             "${PROJECT_SOURCE_DIR}/swresample.lib"
                             "${PROJECT_SOURCE_DIR}/swscale.lib"
                             "${PROJECT_SOURCE_DIR}/FreeImage.lib"
                             "${PROJECT_SOURCE_DIR}/glew32.lib"
                             "${PROJECT_SOURCE_DIR}/libcurl-x64.lib"
                             "${PROJECT_SOURCE_DIR}/freetype.lib"
                             "${PROJECT_SOURCE_DIR}/pugixml.lib"
                             "${PROJECT_SOURCE_DIR}/SDL2main.lib"
                             "${PROJECT_SOURCE_DIR}/SDL2.lib"
                             "Winmm.dll")
        if(VLC_PLAYER)
            set(COMMON_LIBRARIES ${COMMON_LIBRARIES} "${PROJECT_SOURCE_DIR}/libvlc.lib")
        endif()
    else()
        set(COMMON_LIBRARIES "${PROJECT_SOURCE_DIR}/avcodec-58.dll"
                             "${PROJECT_SOURCE_DIR}/avfilter-7.dll"
                             "${PROJECT_SOURCE_DIR}/avformat-58.dll"
                             "${PROJECT_SOURCE_DIR}/avutil-56.dll"
                             "${PROJECT_SOURCE_DIR}/swresample-3.dll"
                             "${PROJECT_SOURCE_DIR}/swscale-5.dll"
                             "${PROJECT_SOURCE_DIR}/FreeImage.dll"
                             "${PROJECT_SOURCE_DIR}/glew32.dll"
                             "${PROJECT_SOURCE_DIR}/libcurl-x64.dll"
                             "${PROJECT_SOURCE_DIR}/libfreetype.dll"
                             "${PROJECT_SOURCE_DIR}/libpugixml.dll"
                             "${PROJECT_SOURCE_DIR}/libSDL2main.a"
                             "${PROJECT_SOURCE_DIR}/SDL2.dll"
                             "mingw32"
                             "Winmm.dll")
        if(VLC_PLAYER)
            set(COMMON_LIBRARIES ${COMMON_LIBRARIES} "${PROJECT_SOURCE_DIR}/libvlc.dll")
        endif()
    endif()
endif()

if(APPLE)
    # See es-app/CMakeLists.txt for an explation for why an extra "Resources" directory
    # has been added to the install prefix.
    set(CMAKE_INSTALL_PREFIX "/Applications/EmulationStation Desktop Edition.app/Contents/Resources")

    if(VLC_PLAYER)
        # Required as the VLC find module doesn't work properly on macOS.
        set(COMMON_LIBRARIES ${COMMON_LIBRARIES} "/Applications/VLC.app/Contents/MacOS/lib/libvlc.dylib")
    endif()

    # Set the same rpath links for the install executable as for the build executable.
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    set(CMAKE_INSTALL_RPATH @executable_path)
endif()

# Add libCEC libraries.
if(DEFINED libCEC_FOUND)
    if(DEFINED BCMHOST OR RPI)
        list(APPEND COMMON_LIBRARIES bcm_host vchiq_arm)
    endif()
    list(APPEND COMMON_LIBRARIES dl ${libCEC_LIBRARIES})
endif()

# Add ALSA for Linux libraries.
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND COMMON_LIBRARIES ${ALSA_LIBRARY})
endif()

if(DEFINED BCMHOST)
    link_directories("${CMAKE_FIND_ROOT_PATH}/opt/vc/lib")
    list(APPEND COMMON_LIBRARIES bcm_host brcmEGL ${OPENGLES_LIBRARIES})
elseif(RPI)
    link_directories("${CMAKE_FIND_ROOT_PATH}/opt/vc/lib")
    list(APPEND COMMON_LIBRARIES ${OPENGLES_LIBRARIES})
endif()

if(GLSYSTEM MATCHES "Desktop OpenGL")
    list(APPEND COMMON_LIBRARIES ${OPENGL_LIBRARIES})
else()
    list(APPEND COMMON_LIBRARIES EGL ${OPENGLES_LIBRARIES})
endif()

#---------------------------------------------------------------------------------------------------
# Build directories.

set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)

# Add each component.
add_subdirectory("es-core")
add_subdirectory("es-app")
