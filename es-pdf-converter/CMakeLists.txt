#  SPDX-License-Identifier: MIT
#
#  ES-DE Frontend
#  CMakeLists.txt (es-pdf-converter)
#
#  CMake configuration for es-pdf-convert
#

project(es-pdf-convert)

if(WIN32)
    set(POPPLER_CPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/poppler/Library/include/poppler/cpp)
    set(POPPLER_CPP_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/poppler-cpp.lib)
    # Disable DLL interface warnings.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251")
elseif(APPLE)
    set(POPPLER_CPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/poppler/cpp ${CMAKE_CURRENT_SOURCE_DIR}/../external/poppler/build/cpp)
    set(POPPLER_CPP_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/../libpoppler-cpp.0.dylib)
elseif(ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -llog")
    set(POPPLER_CPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/poppler/cpp ${CMAKE_CURRENT_SOURCE_DIR}/../external/poppler/build/cpp)
    set(POPPLER_CPP_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/../android/libs/${ANDROID_CPU_ARCH}/libpoppler-cpp.so)
else()
    find_package(Poppler REQUIRED COMPONENTS cpp)
endif()

include_directories(${POPPLER_CPP_INCLUDE_DIR})
if (ANDROID)
    set(CONVERTER_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/ConvertPDF.cpp
                               ${CMAKE_CURRENT_SOURCE_DIR}/src/ConvertPDF.h)
    add_library(es-pdf-convert SHARED ${CONVERTER_SOURCE_FILES})
else()
    set(CONVERTER_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                               ${CMAKE_CURRENT_SOURCE_DIR}/src/ConvertPDF.cpp
                               ${CMAKE_CURRENT_SOURCE_DIR}/src/ConvertPDF.h)
    add_executable(es-pdf-convert WIN32 ${CONVERTER_SOURCE_FILES})
endif()
target_link_libraries(es-pdf-convert PRIVATE ${POPPLER_CPP_LIBRARY})

if(WIN32)
    set_target_properties(es-pdf-convert PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/es-pdf-converter" INSTALL_RPATH_USE_LINK_PATH TRUE)
else()
    set_target_properties(es-pdf-convert PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()
