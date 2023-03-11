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
    # Set this to the minimum supported macOS version, and also update
    # es-app/assets/EmulationStation-DE_Info.plist accordingly.
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14 CACHE STRING "macOS deployment target")
    # This optional variable is used for code signing the DMG installer.
    set(MACOS_CODESIGN_IDENTITY "" CACHE STRING "macOS code signing certificate identity")
endif()
project(emulationstation-de)

# Application version, update this when making a new release.
set(ES_VERSION 2.0.0)

# Set this to ON to show verbose compiler output (e.g. compiler flags, include directories etc.)
set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "Show verbose compiler output" FORCE)

# Package type to use for CPack on Linux.
set(LINUX_CPACK_GENERATOR DEB CACHE STRING "CPack generator, DEB or RPM")

# Add local find modules to the CMake path.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Utils
                              ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Packages)

# Define the options.
option(GL "Set to ON if targeting Desktop OpenGL" ON)
option(GLES "Set to ON if targeting OpenGL ES" OFF)
option(APPLICATION_UPDATER "Set to OFF to build without the application updater" ON)
option(APPIMAGE_BUILD "Set to ON when building as an AppImage" OFF)
option(AUR_BUILD "Set to ON when building for the AUR" OFF)
option(FLATPAK_BUILD "Set to ON when building as a Flatpak" OFF)
option(STEAM_DECK "Set to ON to enable a Valve Steam Deck specific build" OFF)
option(RETRODECK "Set to ON to enable a RetroDECK specific build" OFF)
option(RPI "Set to ON to enable a Raspberry Pi specific build" OFF)
option(BUNDLED_CERTS "Set to ON to use bundled TLS/SSL certificates" OFF)
option(CEC "Set to ON to enable CEC" OFF)
option(VIDEO_HW_DECODING "Set to ON to enable FFmpeg HW decoding" OFF)
option(CLANG_TIDY "Set to ON to build using the clang-tidy static analyzer" OFF)
option(ASAN "Set to ON to build with AddressSanitizer" OFF)
option(TSAN "Set to ON to build with ThreadSanitizer" OFF)
option(UBSAN "Set to ON to build with UndefinedBehaviorSanitizer" OFF)

if(CLANG_TIDY)
    find_program(CLANG_TIDY_BINARY NAMES clang-tidy)
    if(CLANG_TIDY_BINARY MATCHES CLANG_TIDY_BINARY-NOTFOUND)
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
# Raspberry Pi setup.

# Raspberry Pi OS 32-bit (armv7l).
if(EXISTS ${CMAKE_FIND_ROOT_PATH}/opt/vc/include/bcm_host.h)
    set(RPI ON)
    set(RPI_32 ON)
    set(GLES ON)
    set(BCMHOST ON)
    message("-- Building on a Raspberry Pi (32-bit OS)")
endif()

# Raspberry Pi OS 64-bit (aarch64).
if(EXISTS /usr/include/bcm_host.h)
    set(RPI ON)
    set(RPI_64 ON)
    set(GLES ON)
    set(BCMHOST ON)
    message("-- Building on a Raspberry Pi (64-bit OS)")
endif()

#---------------------------------------------------------------------------------------------------
# Emscripten WebAssembly build.

if(EMSCRIPTEN)
    set(GLES ON)
    set(CMAKE_EXECUTABLE_SUFFIX .html)
endif()

#---------------------------------------------------------------------------------------------------
# OpenGL setup.

if(GLES)
    set(GLSYSTEM "Embedded OpenGL" CACHE STRING "The OpenGL system to be used")
else()
    set(GLSYSTEM "Desktop OpenGL" CACHE STRING "The OpenGL system to be used")
endif()

set_property(CACHE GLSYSTEM PROPERTY STRINGS "Desktop OpenGL" "Embedded OpenGL")

#---------------------------------------------------------------------------------------------------
# Package dependencies.

if(GLSYSTEM MATCHES "Desktop OpenGL")
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(OpenGL REQUIRED)
elseif(GLES AND NOT EMSCRIPTEN)
    find_package(OpenGLES2 REQUIRED)
endif()

# On macOS and Windows all dependencies are kept in-tree in the "external" directory.
if(APPLE)
    if(NOT EXISTS ${PROJECT_SOURCE_DIR}/external/FFmpeg)
        message(FATAL_ERROR "-- You need to build the dependencies in ./external first")
    endif()
    find_package(CURL REQUIRED)
elseif(WIN32)
    if(NOT EXISTS ${PROJECT_SOURCE_DIR}/external/pugixml/libpugixml.dll AND # MinGW
       NOT EXISTS ${PROJECT_SOURCE_DIR}/external/pugixml/pugixml.dll)       # MSVC
        message(FATAL_ERROR "-- You need to build the dependencies in ./external first")
    endif()
elseif(NOT EMSCRIPTEN)
    find_package(CURL REQUIRED)
    find_package(FFmpeg REQUIRED)
    find_package(FreeImage REQUIRED)
    find_package(Freetype REQUIRED)
    find_package(Pugixml REQUIRED)
    find_package(SDL2 REQUIRED)
endif()

# Add libCEC support.
if(CEC)
    find_package(libCEC REQUIRED)
endif()

# Add ALSA for Linux.
if(CMAKE_SYSTEM_NAME MATCHES Linux)
    find_package(ALSA REQUIRED)
endif()

#---------------------------------------------------------------------------------------------------
# Compiler and linker settings.

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    message("-- Compiler is Clang/LLVM")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0.0)
        message(SEND_ERROR "You need at least Clang 5.0.0 to compile EmulationStation-DE")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    message("-- Compiler is GNU/GCC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.1)
        message(SEND_ERROR "You need at least GCC 7.1 to compile EmulationStation-DE")
    endif()
    if(WIN32)
        set(CMAKE_CXX_FLAGS "-mwindows ${CMAKE_CXX_FLAGS}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    message("-- Compiler is MSVC")
    # If using the MSVC compiler on Windows, disable the built-in min() and max() macros.
    add_compile_definitions(NOMINMAX)
endif()

if(CMAKE_BUILD_TYPE)
    message("-- Build type is ${CMAKE_BUILD_TYPE}")
endif()

# Set up compiler and linker flags for debug, profiling or release builds.
if(CMAKE_BUILD_TYPE MATCHES Debug)
    # Enable the C++17 standard and disable optimizations as it's a debug build.
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /Od /DEBUG:FULL")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -g3 -Wall -Wpedantic -Wsign-compare -Wnarrowing -Wmissing-field-initializers -Wunused-macros")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0")
    endif()
    # If using Clang, then add additional debug data needed by GDB.
    # Comment this out if you're using LLDB for debugging as this flag makes the binary
    # much larger and the application much slower. On macOS this setting is never enabled
    # as LLDB is the default debugger on this OS.
    if(NOT APPLE AND CMAKE_CXX_COMPILER_ID MATCHES Clang)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG")
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Profiling)
    # For the profiling build, we enable optimizations and supply the required profiler flags.
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /O2 /DEBUG:FULL")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -g3 -Wall -Wpedantic -Wsign-compare -Wnarrowing -Wmissing-field-initializers -Wunused-macros")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O2")
    endif()
else()
    # Enable the C++17 standard and enable optimizations as it's a release build.
    # This will also disable all assert() macros. Strip the binary too.
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
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

if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2 -pthread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s INITIAL_MEMORY=33554432 -s ALLOW_MEMORY_GROWTH=1 -s WASM=1 -s ASSERTIONS=1 -s EXIT_RUNTIME=1 -s USE_SDL=2 \
                                -pthread -s PTHREAD_POOL_SIZE=4 -s DEMANGLE_SUPPORT=1 -s USE_WEBGL2=1 -s FULL_ES3=1 -s ERROR_ON_UNDEFINED_SYMBOLS=1 -s ASYNCIFY \
                                --preload-file ${PROJECT_SOURCE_DIR}/resources@/home/web_user/.emulationstation/resources/ \
                                --preload-file ${PROJECT_SOURCE_DIR}/themes/rbsimple-DE@/home/web_user/.emulationstation/themes/rbsimple-DE/ \
                                --preload-file ${PROJECT_SOURCE_DIR}/ROMs@/home/web_user/ROMs/")
endif()

# Raspberry Pi model 3 and higher (ARM Cortex-A53 minimum).
if(RPI_32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -latomic -mcpu=cortex-a53 -mfpu=neon-fp-armv8")
elseif(RPI_64)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -latomic -mcpu=cortex-a53")
endif()

if(ASAN AND TSAN)
    message(FATAL_ERROR "-- AddressSanitizer and ThreadSanitizer can't be combined")
endif()

if(ASAN)
    message("-- Building with AddressSanitizer")
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    endif()
endif()

if(TSAN)
    message("-- Building with ThreadSanitizer")
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
        message(FATAL_ERROR "-- ThreadSanitizer not available for MSVC")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
    endif()
endif()

if(UBSAN)
    message("-- Building with UndefinedBehaviorSanitizer")
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
        message(FATAL_ERROR "-- UndefinedBehaviorSanitizer not available for MSVC")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer")
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
        set(BUNDLED_CERTS ON)
        message("-- macOS version 10.13 or lower has been set, so if code signing is enabled, Hardened Runtime will not be used")
    endif()
endif()

if(WIN32)
    set(BUNDLED_CERTS ON)
endif()

#---------------------------------------------------------------------------------------------------
# Preprocessor directives.

if(GLES)
    add_compile_definitions(USE_OPENGLES)
    message("-- Building with OpenGL ES renderer")
endif()

if(APPIMAGE_BUILD AND FLATPAK_BUILD)
    message(FATAL_ERROR "-- APPIMAGE_BUILD and FLATPAK_BUILD can't be combined")
endif()

if(APPIMAGE_BUILD)
    if(NOT CMAKE_SYSTEM_NAME MATCHES Linux)
        message(FATAL_ERROR "-- APPIMAGE_BUILD can only be used when building on Linux")
    endif()
    add_compile_definitions(APPIMAGE_BUILD)
    message("-- Building as an AppImage")
endif()

if(FLATPAK_BUILD)
    if(NOT CMAKE_SYSTEM_NAME MATCHES Linux)
        message(FATAL_ERROR "-- FLATPAK_BUILD can only be used when building on Linux")
    endif()
    add_compile_definitions(FLATPAK_BUILD)
    message("-- Building as a Flatpak")
endif()

if(AUR_BUILD)
    message("-- Building for the AUR")
endif()

if(STEAM_DECK AND RETRODECK)
    message(FATAL_ERROR "-- STEAM_DECK and RETRODECK can't be combined")
endif()

if(STEAM_DECK)
    add_compile_definitions(STEAM_DECK)
    message("-- Building for the Valve Steam Deck")
endif()

if(RETRODECK)
    add_compile_definitions(RETRODECK)
    message("-- Building for RetroDECK")
endif()

if(RPI)
    add_compile_definitions(RASPBERRY_PI)
endif()

if(BUNDLED_CERTS)
    add_compile_definitions(USE_BUNDLED_CERTIFICATES)
    message("-- Building with bundled TLS/SSL certificates")
endif()

if(DEFINED libCEC_FOUND)
    add_compile_definitions(HAVE_LIBCEC)
endif()

if(VIDEO_HW_DECODING)
    add_compile_definitions(VIDEO_HW_DECODING)
    message("-- Building with FFmpeg HW decoding")
endif()

if(AUR_BUILD OR FLATPAK_BUILD OR RETRODECK OR RPI)
    set(APPLICATION_UPDATER OFF)
endif()

if(CMAKE_SYSTEM_NAME MATCHES FreeBSD OR CMAKE_SYSTEM_NAME MATCHES NetBSD OR CMAKE_SYSTEM_NAME MATCHES OpenBSD)
    set(APPLICATION_UPDATER OFF)
endif()

if(APPLICATION_UPDATER)
    add_compile_definitions(APPLICATION_UPDATER)
else()
    message("-- Building without application updater")
endif()

# This is needed by the application updater to identify the package type.
if(CMAKE_SYSTEM_NAME MATCHES Linux)
    if(LINUX_CPACK_GENERATOR MATCHES DEB)
        add_compile_definitions(LINUX_DEB_PACKAGE)
    elseif(LINUX_CPACK_GENERATOR MATCHES RPM)
        add_compile_definitions(LINUX_RPM_PACKAGE)
    endif()
endif()

if(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES arm)
        add_compile_definitions(MACOS_APPLE_CPU)
    else()
        add_compile_definitions(MACOS_INTEL_CPU)
    endif()
endif()

if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS 10.14)
    add_compile_definitions(LEGACY_MACOS)
endif()

# Affects the application updater and is used for displaying version info in the main menu.
if(ES_VERSION MATCHES alpha OR ES_VERSION MATCHES beta OR ES_VERSION MATCHES dev)
    add_compile_definitions(IS_PRERELEASE)
endif()

# GLM library options.
add_compile_definitions(GLM_FORCE_CXX17)
add_compile_definitions(GLM_FORCE_XYZW_ONLY)

# For Unix systems, assign the installation prefix. If it's not explicitly set,
# we use /usr on Linux, /usr/pkg on NetBSD and /usr/local on FreeBSD and OpenBSD.
if(NOT WIN32 AND NOT APPLE)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        if(CMAKE_SYSTEM_NAME MATCHES Linux)
            set(CMAKE_INSTALL_PREFIX /usr CACHE INTERNAL CMAKE_INSTALL_PREFIX)
        elseif(CMAKE_SYSTEM_NAME MATCHES NetBSD)
            set(CMAKE_INSTALL_PREFIX /usr/pkg CACHE INTERNAL CMAKE_INSTALL_PREFIX)
        else()
            set(CMAKE_INSTALL_PREFIX /usr/local CACHE INTERNAL CMAKE_INSTALL_PREFIX)
        endif()
    endif()
    message("-- Installation prefix is set to " ${CMAKE_INSTALL_PREFIX})
    add_compile_definitions(ES_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
endif()

# For Windows, set the minimum OS version to Windows 7.
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
    add_compile_definitions(WINVER=0x0601)
endif()

#---------------------------------------------------------------------------------------------------
# Include files.

set(COMMON_INCLUDE_DIRS ${CURL_INCLUDE_DIR}
                        ${CMAKE_CURRENT_SOURCE_DIR}/external/CImg
                        ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
                        ${CMAKE_CURRENT_SOURCE_DIR}/external/lunasvg/include
                        ${CMAKE_CURRENT_SOURCE_DIR}/external/nanosvg/src
                        ${CMAKE_CURRENT_SOURCE_DIR}/external/rapidjson/include
                        ${CMAKE_CURRENT_SOURCE_DIR}/external/rlottie/inc
                        ${CMAKE_CURRENT_SOURCE_DIR}/es-core/src)

if(APPLE)
    set(COMMON_INCLUDE_DIRS ${COMMON_INCLUDE_DIRS}
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/FFmpeg
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/freeimage/FreeImage/Source
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml/src
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL)
elseif(WIN32)
    set(COMMON_INCLUDE_DIRS ${COMMON_INCLUDE_DIRS}
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/curl-7.86.0-win64-mingw/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/glew-2.1.0/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/FreeImage/Dist/x64
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml/src
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2-2.26.3)
elseif(EMSCRIPTEN)
    set(COMMON_INCLUDE_DIRS ${COMMON_INCLUDE_DIRS}
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/curl/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg.wasm-core
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/FreeImage-CMake/FreeImage/Source
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml/src)
else()
    set(COMMON_INCLUDE_DIRS ${COMMON_INCLUDE_DIRS}
                            ${FFMPEG_INCLUDE_DIRS}
                            ${FreeImage_INCLUDE_DIRS}
                            ${FREETYPE_INCLUDE_DIRS}
                            ${PUGIXML_INCLUDE_DIRS}
                            ${SDL2_INCLUDE_DIR})
endif()

# Add libCEC include directory.
if(DEFINED libCEC_FOUND)
    list(APPEND COMMON_INCLUDE_DIRS ${libCEC_INCLUDE_DIR})
endif()

# For Linux, add the ALSA include directory.
if(CMAKE_SYSTEM_NAME MATCHES Linux)
    list(APPEND COMMON_INCLUDE_DIRS ${ALSA_INCLUDE_DIRS})
endif()

if(RPI_32)
    list(APPEND COMMON_INCLUDE_DIRS ${CMAKE_FIND_ROOT_PATH}/opt/vc/include
                                    ${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vcos
                                    ${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vmcs_host/linux
                                    ${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vcos/pthreads)
endif()

#---------------------------------------------------------------------------------------------------
# Dependency libraries.

if(APPLE)
    set(COMMON_LIBRARIES ${COMMON_LIBRARIES}
                         ${CURL_LIBRARIES}
                         ${PROJECT_SOURCE_DIR}/libavcodec.59.dylib
                         ${PROJECT_SOURCE_DIR}/libavfilter.8.dylib
                         ${PROJECT_SOURCE_DIR}/libavformat.59.dylib
                         ${PROJECT_SOURCE_DIR}/libavutil.57.dylib
                         ${PROJECT_SOURCE_DIR}/libswresample.4.dylib
                         ${PROJECT_SOURCE_DIR}/libswscale.6.dylib
                         ${PROJECT_SOURCE_DIR}/libfreeimage.a
                         ${PROJECT_SOURCE_DIR}/libfreetype.6.dylib
                         ${PROJECT_SOURCE_DIR}/libpugixml.a
                         ${PROJECT_SOURCE_DIR}/libSDL2-2.0.0.dylib)
elseif(WIN32)
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
        set(COMMON_LIBRARIES ${PROJECT_SOURCE_DIR}/avcodec.lib
                             ${PROJECT_SOURCE_DIR}/avfilter.lib
                             ${PROJECT_SOURCE_DIR}/avformat.lib
                             ${PROJECT_SOURCE_DIR}/avutil.lib
                             ${PROJECT_SOURCE_DIR}/swresample.lib
                             ${PROJECT_SOURCE_DIR}/swscale.lib
                             ${PROJECT_SOURCE_DIR}/FreeImage.lib
                             ${PROJECT_SOURCE_DIR}/glew32.lib
                             ${PROJECT_SOURCE_DIR}/libcurl-x64.lib
                             ${PROJECT_SOURCE_DIR}/freetype.lib
                             ${PROJECT_SOURCE_DIR}/lunasvg.lib
                             ${PROJECT_SOURCE_DIR}/pugixml.lib
                             ${PROJECT_SOURCE_DIR}/rlottie.lib
                             ${PROJECT_SOURCE_DIR}/SDL2main.lib
                             ${PROJECT_SOURCE_DIR}/SDL2.lib
                             Winmm.dll)
    else()
        set(COMMON_LIBRARIES ${PROJECT_SOURCE_DIR}/avcodec-59.dll
                             ${PROJECT_SOURCE_DIR}/avfilter-8.dll
                             ${PROJECT_SOURCE_DIR}/avformat-59.dll
                             ${PROJECT_SOURCE_DIR}/avutil-57.dll
                             ${PROJECT_SOURCE_DIR}/swresample-4.dll
                             ${PROJECT_SOURCE_DIR}/swscale-6.dll
                             ${PROJECT_SOURCE_DIR}/FreeImage.dll
                             ${PROJECT_SOURCE_DIR}/glew32.dll
                             ${PROJECT_SOURCE_DIR}/libcurl-x64.dll
                             ${PROJECT_SOURCE_DIR}/libfreetype.dll
                             ${PROJECT_SOURCE_DIR}/liblunasvg.dll
                             ${PROJECT_SOURCE_DIR}/libpugixml.dll
                             ${PROJECT_SOURCE_DIR}/libSDL2main.a
                             ${PROJECT_SOURCE_DIR}/librlottie.dll
                             ${PROJECT_SOURCE_DIR}/SDL2.dll
                             mingw32
                             Winmm.dll)
    endif()
elseif(EMSCRIPTEN)
    set(COMMON_LIBRARIES ${COMMON_LIBRARIES}
                         # FFmpeg core libraries.
                         ${PROJECT_SOURCE_DIR}/libavcodec.a
                         ${PROJECT_SOURCE_DIR}/libavfilter.a
                         ${PROJECT_SOURCE_DIR}/libavformat.a
                         ${PROJECT_SOURCE_DIR}/libavutil.a
                         ${PROJECT_SOURCE_DIR}/libpostproc.a
                         ${PROJECT_SOURCE_DIR}/libswresample.a
                         ${PROJECT_SOURCE_DIR}/libswscale.a
                         # FFmpeg third party libraries.
                         ${PROJECT_SOURCE_DIR}/libx264.a
                         ${PROJECT_SOURCE_DIR}/libx265.a
                         ${PROJECT_SOURCE_DIR}/libass.a
                         ${PROJECT_SOURCE_DIR}/libfdk-aac.a
                         ${PROJECT_SOURCE_DIR}/libfribidi.a
                         ${PROJECT_SOURCE_DIR}/libharfbuzz.a
                         ${PROJECT_SOURCE_DIR}/libmp3lame.a
                         ${PROJECT_SOURCE_DIR}/libogg.a
                         ${PROJECT_SOURCE_DIR}/libopus.a
                         ${PROJECT_SOURCE_DIR}/libtheoradec.a
                         ${PROJECT_SOURCE_DIR}/libtheoraenc.a
                         ${PROJECT_SOURCE_DIR}/libvorbis.a
                         ${PROJECT_SOURCE_DIR}/libvorbisenc.a
                         ${PROJECT_SOURCE_DIR}/libvorbisfile.a
                         ${PROJECT_SOURCE_DIR}/libvpx.a
                         ${PROJECT_SOURCE_DIR}/libwavpack.a
                         ${PROJECT_SOURCE_DIR}/libz.a
                         # Other dependencies.
                         ${PROJECT_SOURCE_DIR}/libcurl.a
                         ${PROJECT_SOURCE_DIR}/libFreeImage.a
                         ${PROJECT_SOURCE_DIR}/libfreetype.a
                         ${PROJECT_SOURCE_DIR}/libpugixml.a)
else()
    set(COMMON_LIBRARIES ${CURL_LIBRARIES}
                         ${FFMPEG_LIBRARIES}
                         ${FreeImage_LIBRARIES}
                         ${FREETYPE_LIBRARIES}
                         ${PUGIXML_LIBRARIES}
                         ${SDL2_LIBRARY})
endif()

if(NOT WIN32)
    # SVG rendering library LunaSVG.
    set(COMMON_LIBRARIES ${COMMON_LIBRARIES} ${PROJECT_SOURCE_DIR}/liblunasvg.a)
    # Lottie animation library rlottie.
    set(COMMON_LIBRARIES ${COMMON_LIBRARIES} ${PROJECT_SOURCE_DIR}/librlottie.a)
endif()

if(APPLE)
    # See es-app/CMakeLists.txt for an explation for why an extra "Resources" directory
    # has been added to the install prefix.
    set(CMAKE_INSTALL_PREFIX "/Applications/EmulationStation Desktop Edition.app/Contents/Resources")

    # Set the same rpath links for the install executable as for the build executable.
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    set(CMAKE_INSTALL_RPATH @executable_path)
endif()

# Add libCEC libraries.
if(DEFINED libCEC_FOUND)
    list(APPEND COMMON_LIBRARIES dl ${libCEC_LIBRARIES})
endif()

# Add ALSA for Linux libraries.
if(CMAKE_SYSTEM_NAME MATCHES Linux)
    list(APPEND COMMON_LIBRARIES ${ALSA_LIBRARY})
endif()

# Raspberry Pi.
if(BCMHOST)
    list(APPEND COMMON_LIBRARIES bcm_host vchiq_arm)
    if(RPI_32)
        link_directories(${CMAKE_FIND_ROOT_PATH}/opt/vc/lib)
    endif()
endif()

# OpenGL.
if(GLSYSTEM MATCHES "Desktop OpenGL")
    list(APPEND COMMON_LIBRARIES ${OPENGL_LIBRARIES})
elseif(GLES)
    list(APPEND COMMON_LIBRARIES ${OPENGLES2_LIBRARIES})
endif()

#---------------------------------------------------------------------------------------------------
# Build directories.

set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)

# Add each component.
add_subdirectory(external)
add_subdirectory(es-core)
add_subdirectory(es-app)

# Make sure rlottie is built before es-core and set lottie2gif to not be built.
add_dependencies(es-core rlottie)
set_target_properties(lottie2gif PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
