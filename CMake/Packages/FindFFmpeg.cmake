# Locate FFmpeg
#
# This module defines:
# FFMPEG_FOUND
# FFMPEG_INCLUDE_DIRS
# FFMPEG_LIBRARIES
#
# Created by Robert Osfield
# Modified by Lukas Lalinsky
# Modified by Leon Styhre
#

# Macro to find headers and library directories.
# Example: FFMPEG_FIND(AVFORMAT avformat avformat.h)
macro(FFMPEG_FIND varname shortname headername)
    find_path(FFMPEG_${varname}_INCLUDE_DIRS lib${shortname}/${headername}
        PATHS
        ${FFMPEG_ROOT}/include
        $ENV{FFMPEG_DIR}/include
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/include
        /usr/pkg/include # NetBSD
        /usr/include
        /sw/include # Fink
        /opt/local/include # DarwinPorts
        /opt/csw/include # Blastwave
        /opt/include
        /usr/freeware/include
        NO_DEFAULT_PATH
        PATH_SUFFIXES
        ffmpeg
        ffmpeg4 # NetBSD
        DOC "Location of FFMPEG Headers"
    )

    if(${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
        set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/pkg/lib/ffmpeg4/pkgconfig")
    endif()

    pkg_check_modules(FFMPEG_${varname} lib${shortname} REQUIRED)

    if(FFMPEG_${varname}_LIBRARIES AND FFMPEG_${varname}_INCLUDE_DIRS)
        set(FFMPEG_${varname}_FOUND 1)
        message("--   " ${FFMPEG_${varname}_LINK_LIBRARIES})
    endif(FFMPEG_${varname}_LIBRARIES AND FFMPEG_${varname}_INCLUDE_DIRS)

endmacro(FFMPEG_FIND)

set(FFMPEG_ROOT "$ENV{FFMPEG_DIR}" CACHE PATH "Location of FFMPEG")

FFMPEG_FIND(LIBAVCODEC avcodec avcodec.h)
FFMPEG_FIND(LIBAVFILTER avfilter avfilter.h)
FFMPEG_FIND(LIBAVFORMAT avformat avformat.h)
FFMPEG_FIND(LIBAVUTIL avutil avutil.h)

set(FFMPEG_FOUND "NO")

if(FFMPEG_LIBAVCODEC_FOUND AND FFMPEG_LIBAVFILTER_FOUND AND
    FFMPEG_LIBAVFORMAT_FOUND AND FFMPEG_LIBAVUTIL_FOUND)

    set(FFMPEG_FOUND "YES")
    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS})
    set(FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBAVFORMAT_LIBRARY_DIRS})

    set(FFMPEG_LIBRARIES
        ${FFMPEG_LIBAVCODEC_LINK_LIBRARIES}
        ${FFMPEG_LIBAVFILTER_LINK_LIBRARIES}
        ${FFMPEG_LIBAVFORMAT_LINK_LIBRARIES}
        ${FFMPEG_LIBAVUTIL_LINK_LIBRARIES})
endif()
