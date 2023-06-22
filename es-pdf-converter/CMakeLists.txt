#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  CMakeLists.txt (es-pdf-converter)
#
#  CMake configuration for es-pdf-convert
#

project(es-pdf-convert)

if(WIN32)
   set(POPPLER_CPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/poppler-23.05.0/Library/include/poppler/cpp)
   set(POPPLER_CPP_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/poppler-cpp.lib)
   else()
    find_package(Poppler REQUIRED COMPONENTS cpp)
endif()

include_directories(${POPPLER_CPP_INCLUDE_DIR})
add_executable(es-pdf-convert ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(es-pdf-convert ${POPPLER_CPP_LIBRARY})

if(WIN32)
    set_target_properties(es-pdf-convert PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/es-pdf-converter" INSTALL_RPATH_USE_LINK_PATH TRUE)
else()
    set_target_properties(es-pdf-convert PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()
