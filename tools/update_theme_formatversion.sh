#!/usr/bin/bash
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  update_theme_formatversion.sh
#
#  Updates the format version of all the XML files in a theme set.
#
#  This script is only intended to be used on Linux systems.
#

if [ $# -ne 2 ]; then
  echo "Usage: ./update_theme_formatversion.sh <old theme version> <new theme version>"
  echo "For example:"
  echo "./update_theme_formatversion.sh 5 6"
  exit
fi

TEMPFILE_FILELIST=tempfile_filelist_$(date +%H%M%S)
TEMPFILE_PROCESS=tempfile_process_$(date +%H%M%S)
OLDVERSION=$1
NEWVERSION=$2

find . -name '*.xml' > $TEMPFILE_FILELIST

for file in $(cat $TEMPFILE_FILELIST); do
  echo "Processing file:" $file
  cat $file | sed s/"<formatVersion>${OLDVERSION}"/"<formatVersion>${NEWVERSION}"/g > \
      $TEMPFILE_PROCESS
  mv $TEMPFILE_PROCESS $file
done

rm $TEMPFILE_FILELIST
