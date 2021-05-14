#!/usr/bin/bash
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  mame_merge_index_files.sh
#
#  This script merges older ROM index files with newly generated ones.
#  As the MAME project drops and renames ROM files from time to time, we need to
#  include the old ROM names as well even if not supported by the newest MAME version.
#  The user may very well run an older MAME version and may therefore have an old ROM set.
#
#  There is not much error checking going on here, this script is not intended to be run
#  by the end user.
#
#  This script is only intended to be used on Linux systems.
#

if [ $# -ne 3 ]; then
  echo "Usage: ./mame_merge_index_files.sh <old ROM index file> <new ROM index file> <target filename>"
  echo "For example:"
  echo "./mame_merge_index_files.sh mamebioses.xml_OLD mamebioses.xml_NEW mamebioses.xml"
  echo "or"
  echo "./mame_merge_index_files.sh mamedevices.xml_OLD mamedevices.xml_NEW mamedevices.xml"
  exit
fi

if [ ! -f $1 ]; then
  echo "Can't find old ROM index file" $1
  exit
fi

if [ ! -f $2 ]; then
  echo "Can't find new ROM index file" $1
  exit
fi

MAME_OLD_FILE=$1
MAME_NEW_FILE=$2
MAME_TARGET_FILE=$3
TEMPFILE=tempfile_$(date +%H%M%S)

HEADER=$(grep "<\!--" $MAME_NEW_FILE)

grep -v "Last updated with information from MAME driver file" $MAME_OLD_FILE > $TEMPFILE
grep -v "Last updated with information from MAME driver file" $MAME_NEW_FILE >> $TEMPFILE
echo $HEADER > $MAME_TARGET_FILE
sort -u $TEMPFILE | sed '/^[[:space:]]*$/d' >> $MAME_TARGET_FILE
rm $TEMPFILE
