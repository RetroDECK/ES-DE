#  SPDX-License-Identifier: MIT
#
#  ES-DE Frontend
#  CMakeLists.txt (locale)
#
#  CMake configuration for compiling localization message catalog files using msgfmt
#

add_custom_target(locale ALL COMMENT "Compiling localization message catalogs")

if(APPLE)
    set(MSGFMT_BINARY ${PROJECT_SOURCE_DIR}/external/local_install/bin/msgfmt)
else()
    find_program(MSGFMT_BINARY msgfmt)
endif()

if(NOT MSGFMT_BINARY)
    message (FATAL_ERROR "Couldn't find msgfmt binary")
endif()

file(GLOB PO_FILES ${PROJECT_SOURCE_DIR}/locale/po/*.po)

foreach (PO_FILE ${PO_FILES})
    get_filename_component(PO_FILENAME ${PO_FILE} NAME)
    get_filename_component(PO_BASENAME ${PO_FILENAME} NAME_WE)
    add_custom_command(TARGET locale COMMAND echo "Compiling message catalog for locale" ${PO_BASENAME})
    add_custom_command(TARGET locale COMMAND mkdir -p ${PROJECT_SOURCE_DIR}/resources/locale/${PO_BASENAME}/LC_MESSAGES)
    add_custom_command(TARGET locale COMMAND ${MSGFMT_BINARY} -c --statistics -o ${PROJECT_SOURCE_DIR}/resources/locale/${PO_BASENAME}/LC_MESSAGES/${PO_BASENAME}.mo ${PO_FILE})
endforeach()

add_custom_command(TARGET locale COMMAND echo)
