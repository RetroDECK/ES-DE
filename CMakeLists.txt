cmake_minimum_required(VERSION 3.13)
project(emulationstation-de)

# Set this to ON to show verbose compiler output (e.g. compiler flags, include directories etc.)
set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "Show verbose compiler output" FORCE)

# Add local find modules to CMAKE path.
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Packages)

# Options.
option(GLES "Set to ON if targeting Embedded OpenGL" ${GLES})
option(GL "Set to ON if targeting Desktop OpenGL" ${GL})
option(RPI "Set to ON to enable the Raspberry PI video player (omxplayer)" ${RPI})
option(CEC "Set to ON to enable CEC" ${CEC})

#---------------------------------------------------------------------------------------------------
# Set up OpenGL system variable.
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

# Set up compiler flags.
if(DEFINED BCMHOST OR RPI)
    add_definitions(-D_RPI_)
endif()

if(DEFINED VERO4K)
    add_definitions(-D_VERO4K_)
endif()

if(DEFINED libCEC_FOUND)
    add_definitions(-DHAVE_LIBCEC)
endif()

#---------------------------------------------------------------------------------------------------
# Check for compiler type and version and apply specific compiler settings.
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    message("-- Compiler is Clang/LLVM")
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE CLANG_VERSION)
    if(CLANG_VERSION VERSION_LESS 6.0.0)
        message(SEND_ERROR "You need at least Clang 6.0.0 to compile EmulationStation-DE!")
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message("-- Compiler is GNU/GCC")
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpfullversion OUTPUT_VARIABLE G++_VERSION)
    if(G++_VERSION VERSION_LESS 4.8)
        message(SEND_ERROR "You need at least GCC 4.8 to compile EmulationStation-DE!")
    endif()
    if(WIN32)
        set(CMAKE_CXX_FLAGS "-mwindows ${CMAKE_CXX_FLAGS}")
#        set(CMAKE_CXX_FLAGS "-static-libstdc++ -static-libgcc ${CMAKE_CXX_FLAGS}")
    endif()
endif()

# Set up compiler flags for debug or release builds.
if(CMAKE_BUILD_TYPE MATCHES Debug)
    # Enable the C++11 standard and disable optimizations as it's a debug build.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-attributes -O0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0")

    # If using Clang, then add additional debug data needed by GDB.
    # Comment this out if you're using LLDB for debugging as this flag makes
    # the binary much larger and the application much slower.
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG")
    endif()
else()
    # Enable the C++11 standard and enable optimizations as it's a release build.
    # Also disable the assert() macros and strip the binary.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-attributes -O2 -DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O2 -s")
endif()

if(${GLSystem} MATCHES "Desktop OpenGL")
    add_definitions(-DUSE_OPENGL_21)
else()
    add_definitions(-DUSE_OPENGLES_10)
endif()

# For Unix systems, assign the installation prefix to local $ES_INSTALL_PREFIX variable.
if(NOT WIN32)
    add_definitions(-DES_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
endif()

# Enable additional defines for the Debug build configuration.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")

# Handle additional (required) include files for dependency packages.
if(WIN32)
    set(WIN32_INCLUDE_DIR "NOT_DEFINED" CACHE FILEPATH "")
    if(NOT EXISTS ${WIN32_INCLUDE_DIR})
        message(SEND_ERROR "Can't find WIN32 include directory: ${WIN32_INCLUDE_DIR}")
    endif()
    #include_directories(${WIN32_INCLUDE_DIR})
endif()

#---------------------------------------------------------------------------------------------------
# Add include directories.
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

if(WIN32)
    set(COMMON_INCLUDE_DIRS ${COMMON_INCLUDE_DIRS} ${WIN32_INCLUDE_DIR})
endif()

# Add libCEC include directory.
if(DEFINED libCEC_FOUND)
    list(APPEND COMMON_INCLUDE_DIRS ${libCEC_INCLUDE_DIR})
endif()

# Add ALSA for Linux include directory.
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

# Define libraries and directories.
if(DEFINED BCMHOST)
    link_directories("${CMAKE_FIND_ROOT_PATH}/opt/vc/lib")
elseif(DEFINED VERO4K)
    link_directories("${CMAKE_FIND_ROOT_PATH}/opt/vero3/lib")
endif()

if (NOT WIN32)
    set(COMMON_LIBRARIES
        ${CURL_LIBRARIES}
        ${FreeImage_LIBRARIES}
        ${FREETYPE_LIBRARIES}
        ${PUGIXML_LIBRARIES}
        ${SDL2_LIBRARY}
        ${VLC_LIBRARIES}
        nanosvg)
elseif(WIN32)
    set(COMMON_LIBRARIES
        "${PROJECT_SOURCE_DIR}/libpugixml.dll"
        "${PROJECT_SOURCE_DIR}/FreeImage.dll"
        "${PROJECT_SOURCE_DIR}/libcurl-x64.dll"
        "${PROJECT_SOURCE_DIR}/libvlc.dll"
        "${PROJECT_SOURCE_DIR}/libfreetype.dll"
        "Winmm.dll"
        "mingw32"
        "${PROJECT_SOURCE_DIR}/libSDL2main.a"
        "${PROJECT_SOURCE_DIR}/SDL2.dll"
        "nanosvg")
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
    list(APPEND COMMON_LIBRARIES bcm_host brcmEGL ${OPENGLES_LIBRARIES})
elseif(DEFINED VERO4K)
    list(APPEND COMMON_LIBRARIES EGL ${OPENGLES_LIBRARIES})
else()
    if(${GLSystem} MATCHES "Desktop OpenGL")
        list(APPEND COMMON_LIBRARIES ${OPENGL_LIBRARIES})
    else()
        list(APPEND COMMON_LIBRARIES EGL ${OPENGLES_LIBRARIES})
    endif()
endif()

#---------------------------------------------------------------------------------------------------
# Set up build directories.
set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)

# Add each component.
add_subdirectory("external")
add_subdirectory("es-core")
add_subdirectory("es-app")
