cmake_minimum_required(VERSION 3.13)
if(APPLE)
    # Set this to the operating system version you're building on, and also update
    # es-app/assets/EmulationStation-DE_Info.plist accordingly.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "macOS deployment target")
endif()
project(emulationstation-de)

# Set this to ON to show verbose compiler output (e.g. compiler flags, include directories etc.)
set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "Show verbose compiler output" FORCE)

# Package type to use for CPack on Linux.
set(LINUX_CPACK_GENERATOR "DEB" CACHE STRING "CPack generator, DEB or RPM")

# Add local find modules to the CMake path.
list(APPEND CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Utils
        ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Packages)

# Define the options.
option(GLES "Set to ON if targeting Embedded OpenGL" ${GLES})
option(GL "Set to ON if targeting Desktop OpenGL" ${GL})
option(RPI "Set to ON to enable Raspberry Pi specific build" ${RPI})
option(CEC "Set to ON to enable CEC" ${CEC})
option(APPLE_SKIP_INSTALL_LIBS "Set to ON to skip installation of shared libraries (macOS only)"
        ${APPLE_SKIP_INSTALL_LIBS})

#---------------------------------------------------------------------------------------------------
# OpenGL setup.

if(GLES)
    set(GLSystem "Embedded OpenGL" CACHE STRING "The OpenGL system to be used")
elseif(GL)
    set(GLSystem "Desktop OpenGL" CACHE STRING "The OpenGL system to be used")

# Check if we're running on a Raspberry Pi.
elseif(EXISTS "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/bcm_host.h")
    MESSAGE("bcm_host.h found")
    set(BCMHOST found)
    set(GLSystem "Embedded OpenGL" CACHE STRING "The OpenGL system to be used")

# Check if we're running on OSMC Vero4K.
elseif(EXISTS "${CMAKE_FIND_ROOT_PATH}/opt/vero3/lib/libMali.so")
    MESSAGE("libMali.so found")
    set(VERO4K found)
    set(GLSystem "Embedded OpenGL" CACHE STRING "The OpenGL system to be used")

# Check if we're running on olinuxino / odroid / etc.
elseif(EXISTS "${CMAKE_FIND_ROOT_PATH}/usr/lib/libMali.so" OR
    EXISTS "${CMAKE_FIND_ROOT_PATH}/usr/lib/arm-linux-gnueabihf/libMali.so" OR
    EXISTS "${CMAKE_FIND_ROOT_PATH}/usr/lib/aarch64-linux-gnu/libMali.so" OR
    EXISTS "${CMAKE_FIND_ROOT_PATH}/usr/lib/arm-linux-gnueabihf/mali-egl/libmali.so" OR
    EXISTS "${CMAKE_FIND_ROOT_PATH}/usr/lib/arm-linux-gnueabihf/libmali.so")
    MESSAGE("libMali.so found")
    set(GLSystem "Embedded OpenGL" CACHE STRING "The OpenGL system to be used")
else()
    set(GLSystem "Desktop OpenGL" CACHE STRING "The OpenGL system to be used")
endif(GLES)

set_property(CACHE GLSystem PROPERTY STRINGS "Desktop OpenGL" "Embedded OpenGL")

#---------------------------------------------------------------------------------------------------
# Package dependencies.

if(${GLSystem} MATCHES "Desktop OpenGL")
    set(OpenGL_GL_PREFERENCE "GLVND")
    find_package(OpenGL REQUIRED)
else()
    find_package(OpenGLES REQUIRED)
endif()

# Skip package dependency checks if we're on Windows.
if(NOT WIN32)
    find_package(CURL REQUIRED)
    find_package(FreeImage REQUIRED)
    find_package(Freetype REQUIRED)
    find_package(Pugixml REQUIRED)
    find_package(RapidJSON REQUIRED)
    find_package(SDL2 REQUIRED)
    find_package(VLC REQUIRED)
endif()

# Add libCEC support.
if(CEC)
    find_package(libCEC REQUIRED)
endif()

# Add ALSA for Linux.
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(ALSA REQUIRED)
endif()

#---------------------------------------------------------------------------------------------------
# Compiler and linker settings.

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    message("-- Compiler is Clang/LLVM")
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE CLANG_VERSION)
    if(CLANG_VERSION VERSION_LESS 4.2.1)
        message(SEND_ERROR "You need at least Clang 4.2.1 to compile EmulationStation-DE")
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message("-- Compiler is GNU/GCC")
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpfullversion OUTPUT_VARIABLE G++_VERSION)
    if(G++_VERSION VERSION_LESS 4.8)
        message(SEND_ERROR "You need at least GCC 4.8 to compile EmulationStation-DE")
    endif()
    if(WIN32)
        set(CMAKE_CXX_FLAGS "-mwindows ${CMAKE_CXX_FLAGS}")
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    message("-- Compiler is MSVC")
    # If using the MSVC compiler on Windows, disable the built-in min() and max() macros.
    add_definitions(-DNOMINMAX)
endif()

if (CMAKE_BUILD_TYPE)
    message("-- Build type is ${CMAKE_BUILD_TYPE}")
endif()

# Set up compiler and linker flags for debug, profiling or release builds.
if(CMAKE_BUILD_TYPE MATCHES Debug)
    # Enable the C++11 standard and disable optimizations as it's a debug build.
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c11 /Od /DEBUG:FULL")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0")
    endif()
    # If using Clang, then add additional debug data needed by GDB.
    # Comment this out if you're using LLDB for debugging as this flag makes the binary
    # much larger and the application much slower. On macOS this setting is never enabled
    # as LLDB is the default debugger on this OS.
    if(NOT APPLE AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG")
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Profiling)
    # For the profiling build, we enable optimizations and supply the required profiler flags.
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c11 /O2 /DEBUG:FULL")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -pg -g")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O2 -pg")
    endif()
else()
    # Enable the C++11 standard and enable optimizations as it's a release build.
    # This will also disable all assert() macros. Strip the binary too.
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG /std:c11 /O2 /DEBUG:NONE")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -DNDEBUG")
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
    if(${CMAKE_OSX_DEPLOYMENT_TARGET} VERSION_LESS 10.14)
        message("-- macOS version 10.13 or lower has been set, so if code signing is enabled, Hardened Runtime will not be used")
    endif()
endif()

#---------------------------------------------------------------------------------------------------
# Preprocessor directives.

if(${GLSystem} MATCHES "Desktop OpenGL")
    add_definitions(-DUSE_OPENGL_21)
else()
    add_definitions(-DUSE_OPENGLES_10)
endif()

if(DEFINED BCMHOST OR RPI)
    add_definitions(-D_RPI_)
endif()

if(DEFINED VERO4K)
    add_definitions(-D_VERO4K_)
endif()

if(DEFINED libCEC_FOUND)
    add_definitions(-DHAVE_LIBCEC)
endif()

# For Unix systems, assign the installation prefix. If it's not explicitly set,
# we use /usr on Linux, /usr/pkg on NetBSD and /usr/local on FreeBSD and OpenBSD.
if(NOT WIN32 AND NOT APPLE)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
            set(CMAKE_INSTALL_PREFIX "/usr")
        elseif(${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
            set(CMAKE_INSTALL_PREFIX "/usr/pkg")
        else()
            set(CMAKE_INSTALL_PREFIX "/usr/local")
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

set(COMMON_INCLUDE_DIRS
        ${CURL_INCLUDE_DIR}
        ${FreeImage_INCLUDE_DIRS}
        ${FREETYPE_INCLUDE_DIRS}
        ${PUGIXML_INCLUDE_DIRS}
        ${RAPIDJSON_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIR}
        ${VLC_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        ${CMAKE_CURRENT_SOURCE_DIR}/es-core/src)

# For Windows we need to add local include files for the dependency packages.
if(WIN32)
    set(WIN32_INCLUDE_DIR "NOT_DEFINED" CACHE FILEPATH "")
    if(NOT EXISTS ${WIN32_INCLUDE_DIR})
        message(SEND_ERROR "Can't find WIN32 include directory: ${WIN32_INCLUDE_DIR}")
    endif()
    #include_directories(${WIN32_INCLUDE_DIR})
endif()

# Temporary solution until the VLC find module has been updated to work properly on macOS.
if(APPLE)
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
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    list(APPEND COMMON_INCLUDE_DIRS ${ALSA_INCLUDE_DIRS})
endif()

if(DEFINED BCMHOST)
    list(APPEND COMMON_INCLUDE_DIRS
            "${CMAKE_FIND_ROOT_PATH}/opt/vc/include"
            "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vcos"
            "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vmcs_host/linux"
            "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vcos/pthreads")
# Add Vero4k include directory.
elseif(DEFINED VERO4K)
    list(APPEND COMMON_INCLUDE_DIRS "${CMAKE_FIND_ROOT_PATH}/opt/vero3/include")
else()
    # Add OpenGL include directory.
    if(${GLSystem} MATCHES "Desktop OpenGL")
        list(APPEND COMMON_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR})
    else()
        # Add OpenGL ES include directory.
        list(APPEND COMMON_INCLUDE_DIRS ${OPENGLES_INCLUDE_DIR})
    endif()
endif()

#---------------------------------------------------------------------------------------------------
# Dependency libraries.

if(NOT WIN32)
    set(COMMON_LIBRARIES
            ${CURL_LIBRARIES}
            ${FreeImage_LIBRARIES}
            ${FREETYPE_LIBRARIES}
            ${PUGIXML_LIBRARIES}
            ${SDL2_LIBRARY}
            ${VLC_LIBRARIES}
            nanosvg)
elseif(WIN32)
    if(DEFINED MSVC)
        set(COMMON_LIBRARIES
                "${PROJECT_SOURCE_DIR}/FreeImage.lib"
                "${PROJECT_SOURCE_DIR}/glew32.lib"
                "${PROJECT_SOURCE_DIR}/libcurl-x64.lib"
                "${PROJECT_SOURCE_DIR}/freetype.lib"
                "${PROJECT_SOURCE_DIR}/pugixml.lib"
                "${PROJECT_SOURCE_DIR}/SDL2main.lib"
                "${PROJECT_SOURCE_DIR}/libvlc.lib"
                "${PROJECT_SOURCE_DIR}/SDL2.lib"
                "nanosvg"
                "Winmm.dll")
    else()
        set(COMMON_LIBRARIES
                "${PROJECT_SOURCE_DIR}/FreeImage.dll"
                "${PROJECT_SOURCE_DIR}/glew32.dll"
                "${PROJECT_SOURCE_DIR}/libcurl-x64.dll"
                "${PROJECT_SOURCE_DIR}/libfreetype.dll"
                "${PROJECT_SOURCE_DIR}/libpugixml.dll"
                "${PROJECT_SOURCE_DIR}/libSDL2main.a"
                "${PROJECT_SOURCE_DIR}/libvlc.dll"
                "${PROJECT_SOURCE_DIR}/SDL2.dll"
                "mingw32"
                "nanosvg"
                "Winmm.dll")
    endif()
endif()

if(APPLE)
    # See es-app/CMakeLists.txt for an explation for why an extra 'Resources' directory
    # has been added to the install prefix.
    set(CMAKE_INSTALL_PREFIX
            "/Applications/EmulationStation Desktop Edition.app/Contents/Resources")

    # Temporary solution until the VLC find module has been updated to work properly on macOS.
    set(COMMON_LIBRARIES ${COMMON_LIBRARIES}
            "/Applications/VLC.app/Contents/MacOS/lib/libvlc.dylib")

    # Set the same rpath links for the install executable as for the build executable.
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    if(NOT APPLE_SKIP_INSTALL_LIBS)
        set(CMAKE_INSTALL_RPATH @executable_path)
    endif()
endif()

# Add libCEC libraries.
if(DEFINED libCEC_FOUND)
if(DEFINED BCMHOST)
    list(APPEND COMMON_LIBRARIES vchiq_arm)
endif()
    list(APPEND COMMON_LIBRARIES dl ${libCEC_LIBRARIES})
endif()

# Add ALSA for Linux libraries.
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    list(APPEND COMMON_LIBRARIES ${ALSA_LIBRARY})
endif()

if(DEFINED BCMHOST)
    link_directories("${CMAKE_FIND_ROOT_PATH}/opt/vc/lib")
    list(APPEND COMMON_LIBRARIES bcm_host brcmEGL ${OPENGLES_LIBRARIES})
elseif(DEFINED VERO4K)
    link_directories("${CMAKE_FIND_ROOT_PATH}/opt/vero3/lib")
    list(APPEND COMMON_LIBRARIES EGL ${OPENGLES_LIBRARIES})
else()
    if(${GLSystem} MATCHES "Desktop OpenGL")
        list(APPEND COMMON_LIBRARIES ${OPENGL_LIBRARIES})
    else()
        list(APPEND COMMON_LIBRARIES EGL ${OPENGLES_LIBRARIES})
    endif()
endif()

#---------------------------------------------------------------------------------------------------
# Build directories.

set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)

# Add each component.
add_subdirectory("external")
add_subdirectory("es-core")
add_subdirectory("es-app")
