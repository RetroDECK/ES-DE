#!/bin/sh
#
# macOS_change_dylib_rpaths.sh
# Update the dylib paths to use rpaths instead of absolute paths.
#
# This script does not do much error checking so the results should be verified manually
# afterwards using the "otool -L" command.
# Changes are needed to this script when moving to a new version for any of the libraries.
#
# Leon Styhre
# 2021-05-13
#

export FREETYPE_FILENAME=libfreetype.6.dylib
export AVCODEC_FILENAME=libavcodec.58.dylib
export AVFORMAT_FILENAME=libavformat.58.dylib
export SWRESAMPLE_FILENAME=libswresample.3.dylib
export SWSCALE_FILENAME=libswscale.5.dylib

if [ -f $FREETYPE_FILENAME ]; then
  echo Found file $FREETYPE_FILENAME - changing rpaths
  chmod 755 $FREETYPE_FILENAME
  install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib @rpath/libpng16.16.dylib $FREETYPE_FILENAME
  install_name_tool -add_rpath @executable_path $FREETYPE_FILENAME
fi

if [ -f $AVCODEC_FILENAME ]; then
  echo Found file $AVCODEC_FILENAME - changing rpaths
  chmod 755 $AVCODEC_FILENAME
  install_name_tool -change /usr/local/lib/libswresample.3.dylib @rpath/libswresample.3.dylib $AVCODEC_FILENAME
  install_name_tool -change /usr/local/lib/libavutil.56.dylib @rpath/libavutil.56.dylib $AVCODEC_FILENAME
  install_name_tool -change /usr/local/opt/fdk-aac/lib/libfdk-aac.2.dylib @rpath/libfdk-aac.2.dylib $AVCODEC_FILENAME
  install_name_tool -add_rpath @executable_path $AVCODEC_FILENAME
fi

if [ -f $AVFORMAT_FILENAME ]; then
  echo Found file $AVFORMAT_FILENAME - changing rpaths
  chmod 755 $AVFORMAT_FILENAME
  install_name_tool -change /usr/local/lib/libavcodec.58.dylib @rpath/libavcodec.58.dylib $AVFORMAT_FILENAME
  install_name_tool -change /usr/local/lib/libswresample.3.dylib @rpath/libswresample.3.dylib $AVFORMAT_FILENAME
  install_name_tool -change /usr/local/lib/libavutil.56.dylib @rpath/libavutil.56.dylib $AVFORMAT_FILENAME
  install_name_tool -change /usr/local/opt/fdk-aac/lib/libfdk-aac.2.dylib @rpath/libfdk-aac.2.dylib $AVFORMAT_FILENAME
  install_name_tool -add_rpath @executable_path $AVFORMAT_FILENAME
fi

if [ -f $SWRESAMPLE_FILENAME ]; then
  echo Found file $SWRESAMPLE_FILENAME - changing rpaths
  chmod 755 $SWRESAMPLE_FILENAME
  install_name_tool -change /usr/local/lib/libavutil.56.dylib @rpath/libavutil.56.dylib $SWRESAMPLE_FILENAME
  install_name_tool -add_rpath @executable_path $SWRESAMPLE_FILENAME
fi

if [ -f $SWSCALE_FILENAME ]; then
  echo Found file $SWSCALE_FILENAME - changing rpaths
  chmod 755 $SWSCALE_FILENAME
  install_name_tool -change /usr/local/lib/libavutil.56.dylib @rpath/libavutil.56.dylib $SWSCALE_FILENAME
  install_name_tool -add_rpath @executable_path $SWSCALE_FILENAME
fi
