# Find the git2 library.
#
# GIT2_INCLUDE_DIRS - Where to find git2.h, etc.
# GIT2_LIBRARIES    - List of libraries when using libgit2.
# GIT2_FOUND        - True if libgit2 is found.

# GIT2_INCLUDE_PATH
find_path(GIT2_INCLUDE_PATH NAMES git2.h)
# GIT2_LIBRARY
find_library(GIT2_LIBRARY NAMES git2)

# Handle the QUIETLY and REQUIRED arguments and set GIT2_FOUND to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libgit2 REQUIRED_VARS GIT2_LIBRARY GIT2_INCLUDE_PATH)

if(GIT2_FOUND)
  set(GIT2_INCLUDE_DIR  ${GIT2_INCLUDE_PATH})
  set(GIT2_INCLUDE_DIRS ${GIT2_INCLUDE_PATH})
  set(GIT2_LIBRARIES    ${GIT2_LIBRARY})
endif()

mark_as_advanced(
  GIT2_INCLUDE_PATH
  GIT2_LIBRARY
)