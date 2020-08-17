
# Find the RapidJSON parsing library.
#
# Sets the usual variables expected for find_package scripts:
#
# RAPIDJSON_INCLUDE_DIR - header location
# RAPIDJSON_FOUND - true if RAPIDJSON was found.

include(FindPkgMacros)

if (NOT WIN32)
  find_package(PkgConfig)
  pkg_check_modules(RAPIDJSON REQUIRED RapidJSON>=1.0.0)
endif (NOT WIN32)

if (WIN32)
  find_path(RAPIDJSON_INCLUDE_DIR rapidjson/rapidjson.h)

# Support the REQUIRED and QUIET arguments, and set RAPIDJSON_FOUND if found.
include (FindPackageHandleStandardArgs)

find_package_handle_standard_args(RAPIDJSON DEFAULT_MSG RAPIDJSON_INCLUDE_DIR)

if (NOT RAPIDJSON_INCLUDE_DIR)
  message(FATAL_ERROR "RapidJSON include files not found!")
endif()

endif (WIN32)
