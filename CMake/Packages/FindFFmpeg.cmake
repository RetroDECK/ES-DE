# Locate ffmpeg
# This module defines
# FFMPEG_LIBRARIES
# FFMPEG_FOUND, if false, do not try to link to ffmpeg
# FFMPEG_INCLUDE_DIR, where to find the headers
#
# $FFMPEG_DIR is an environment variable that would
# correspond to the ./configure --prefix=$FFMPEG_DIR
#
# Created by Robert Osfield.
# Modified by Lukas Lalinsky.

# In ffmpeg code, old version use "#include <header.h>" and newer use "#include <libname/header.h>"
# In OSG ffmpeg plugin, we use "#include <header.h>" for compatibility with old version of ffmpeg

# We have to search the path which contain the header.h (usefull for old version)
# and search the path which contain the libname/header.h (usefull for new version)

# Then we need to include ${FFMPEG_libname_INCLUDE_DIRS}
# (in old version case, use by ffmpeg header and osg plugin code)
# (in new version case, use by ffmpeg header)
# and ${FFMPEG_libname_INCLUDE_DIRS/libname}
# (in new version case, use by osg plugin code)

# Macro to find header and lib directories
# example: FFMPEG_FIND(AVFORMAT avformat avformat.h)
MACRO(FFMPEG_FIND varname shortname headername)

    FIND_PATH(FFMPEG_${varname}_INCLUDE_DIRS lib${shortname}/${headername}
        PATHS
        ${FFMPEG_ROOT}/include
        $ENV{FFMPEG_DIR}/include
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/include
        /usr/include/
        /sw/include # Fink
        /opt/local/include # DarwinPorts
        /opt/csw/include # Blastwave
        /opt/include
        /usr/freeware/include
        NO_DEFAULT_PATH
        PATH_SUFFIXES ffmpeg
        DOC "Location of FFMPEG Headers"
    )
    FIND_PATH(FFMPEG_${varname}_INCLUDE_DIRS lib${shortname}/${headername}
        PATH_SUFFIXES ffmpeg
        DOC "Location of FFMPEG Headers"
    )

    pkg_check_modules(FFMPEG_${varname} lib${shortname})

    IF (FFMPEG_${varname}_LIBRARIES AND FFMPEG_${varname}_INCLUDE_DIRS)
        SET(FFMPEG_${varname}_FOUND 1)
    ENDIF(FFMPEG_${varname}_LIBRARIES AND FFMPEG_${varname}_INCLUDE_DIRS)

ENDMACRO(FFMPEG_FIND)

SET(FFMPEG_ROOT "$ENV{FFMPEG_DIR}" CACHE PATH "Location of FFMPEG")

# find stdint.h
FIND_PATH(FFMPEG_STDINT_INCLUDE_DIR stdint.h
    PATHS
    ${FFMPEG_ROOT}/include
    $ENV{FFMPEG_DIR}/include
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/include
    /usr/include
    /sw/include # Fink
    /opt/local/include # DarwinPorts
    /opt/csw/include # Blastwave
    /opt/include
    /usr/freeware/include
    PATH_SUFFIXES ffmpeg
    DOC "Location of FFMPEG stdint.h Header"
)

FFMPEG_FIND(LIBAVFORMAT avformat avformat.h)
FFMPEG_FIND(LIBAVCODEC avcodec avcodec.h)
FFMPEG_FIND(LIBAVCODEC_FFT avcodec avfft.h)
FFMPEG_FIND(LIBAVUTIL avutil avutil.h)
FFMPEG_FIND(LIBSWRESAMPLE swresample  swresample.h)
FFMPEG_FIND(LIBSWSCALE swscale swscale.h) # Not sure about the header to look for here.

SET(FFMPEG_FOUND "NO")

IF (FFMPEG_LIBAVFORMAT_FOUND AND FFMPEG_LIBAVCODEC_FOUND AND FFMPEG_LIBAVUTIL_FOUND AND
    FFMPEG_LIBSWRESAMPLE_FOUND AND FFMPEG_LIBSWSCALE_FOUND AND FFMPEG_STDINT_INCLUDE_DIR)

    SET(FFMPEG_FOUND "YES")
    SET(FFMPEG_INCLUDE_DIRS ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS})
    SET(FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBAVFORMAT_LIBRARY_DIRS})

    SET(FFMPEG_LIBRARIES
        ${FFMPEG_LIBAVFORMAT_LIBRARIES}
        ${FFMPEG_LIBAVCODEC_LIBRARIES}
        ${FFMPEG_LIBAVUTIL_LIBRARIES}
        ${FFMPEG_LIBSWRESAMPLE_LIBRARIES}
        ${FFMPEG_LIBSWSCALE_LIBRARIES})
ELSE ()

# MESSAGE(STATUS "Could not find FFMPEG")

ENDIF()
