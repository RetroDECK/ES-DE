#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  CMakeLists.txt (es-pdf-converter)
#
#  CMake configuration for es-pdf-convert
#

project(es-pdf-convert)

find_package(Poppler REQUIRED COMPONENTS cpp)

include_directories(${POPPLER_CPP_INCLUDE_DIR})
add_executable(es-pdf-convert ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(es-pdf-convert ${POPPLER_CPP_LIBRARY})
set_target_properties(es-pdf-convert PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
