
# Find the pugixml XML parsing library.
#
# Sets the usual variables expected for find_package scripts:
#
# Looks for both the include files (via pkgconfig) and the shared library.
#
# PUGIXML_INCLUDE_DIR - header location
# PUGIXML_LIBRARIES - library to link against
# PUGIXML_FOUND - true if pugixml was found.

include(FindPkgMacros)

if (NOT WIN32)
  find_package(PkgConfig)
  pkg_check_modules(PUGIXML REQUIRED pugixml>=1.09)
endif (NOT WIN32)

if (WIN32)
find_path(PUGIXML_INCLUDE_DIR pugixml.hpp)

# Support the REQUIRED and QUIET arguments, and set PUGIXML_FOUND if found.
include (FindPackageHandleStandardArgs)

find_package_handle_standard_args(PUGIXML DEFAULT_MSG PUGIXML_INCLUDE_DIR)

if (NOT PUGIXML_INCLUDE_DIR)
    message(FATAL_ERROR "PUGIXML include files not found!")
endif()

endif (WIN32)

find_library(PUGIXML_LIBRARY pugixml)

if (NOT PUGIXML_LIBRARY)
    message(FATAL_ERROR "libpugixml library not found!")
endif()

# Support the REQUIRED and QUIET arguments, and set PUGIXML_FOUND if found.
include (FindPackageHandleStandardArgs)

find_package_handle_standard_args(pugixml DEFAULT_MSG PUGIXML_LIBRARY)
