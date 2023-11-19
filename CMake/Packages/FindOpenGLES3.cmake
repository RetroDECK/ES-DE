# FindOpenGLES
# ------------
# Finds the OpenGLES3 library
#
# This will define the following variables::
#
# OPENGLES3_FOUND - system has OpenGLES
# OPENGLES3_INCLUDE_DIRS - the OpenGLES include directory
# OPENGLES3_LIBRARIES - the OpenGLES libraries

if(NOT HINT_GLES_LIBNAME)
    set(HINT_GLES_LIBNAME GLESv3)
endif()

find_path(OPENGLES3_INCLUDE_DIR GLES3/gl3.h
    PATHS "${CMAKE_FIND_ROOT_PATH}/usr/include"
    HINTS ${HINT_GLES_INCDIR}
)

find_library(OPENGLES3_gl_LIBRARY
    NAMES ${HINT_GLES_LIBNAME}
    HINTS ${HINT_GLES_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGLES3 REQUIRED_VARS OPENGLES3_gl_LIBRARY OPENGLES3_INCLUDE_DIR)

if(OPENGLES3_FOUND)
    set(OPENGLES3_LIBRARIES ${OPENGLES3_gl_LIBRARY})
    set(OPENGLES3_INCLUDE_DIRS ${OPENGLES3_INCLUDE_DIR})
    mark_as_advanced(OPENGLES3_INCLUDE_DIR OPENGLES3_gl_LIBRARY)
endif()
