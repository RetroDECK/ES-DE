#!/usr/bin/bash
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  update_version_string.sh
#
#  Updates the version string for EmulationStation Desktop Edition.
#  This script takes as arguments the major, minor and patch numbers as well as an optional
#  alphanumeric suffix and updates all the necessary files to indicate a new software version.
#  The script has to be run from within the tools directory.
#
#  Example use:
#  ./update_version_string.sh 1 2 0 beta1
#
#  The following files are updated by this script:
#  es-app/CMakeLists.txt
#  es-app/src/EmulationStation.h
#  es-app/assets/EmulationStation-DE_Info.plist
#
#  This script is only intended to be used on Linux systems.
#

if [ ! -f ../es-app/CMakeLists.txt ]; then
  echo "You need to run this script from within the tools directory."
  exit
fi

if [ $# -ne 3 ] && [ $# -ne 4 ]; then
  echo "Usage: ./update_version_string.sh <major version> <minor version> <patch version> [<suffix>]"
  echo "For example:"
  echo "./update_version_string.sh 2 0 0 beta"
  exit
fi

if [ $# -eq 4 ]; then
  SUFFIX=-$4
else
  SUFFIX=""
fi

TEMPFILE=update_version_string.tmp

##### CMakeLists.txt

MODIFYFILE=../CMakeLists.txt
MODIFYSTRING=$(grep "set(ES_VERSION" $MODIFYFILE)
NEWSTRING="set(ES_VERSION ${1}.${2}.${3}${SUFFIX})"

cat $MODIFYFILE | sed s/"${MODIFYSTRING}"/"${NEWSTRING}"/ > $TEMPFILE
mv $TEMPFILE $MODIFYFILE

##### EmulationStation.h

MODIFYFILE=../es-app/src/EmulationStation.h

MODIFYSTRING=$(grep "PROGRAM_VERSION_MAJOR     " $MODIFYFILE)
NEWSTRING="#define PROGRAM_VERSION_MAJOR        ${1}"

cat $MODIFYFILE | sed s/"${MODIFYSTRING}"/"${NEWSTRING}"/ > $TEMPFILE
mv $TEMPFILE $MODIFYFILE

MODIFYSTRING=$(grep "PROGRAM_VERSION_MINOR     " $MODIFYFILE)
NEWSTRING="#define PROGRAM_VERSION_MINOR        ${2}"

cat $MODIFYFILE | sed s/"${MODIFYSTRING}"/"${NEWSTRING}"/ > $TEMPFILE
mv $TEMPFILE $MODIFYFILE

MODIFYSTRING=$(grep "PROGRAM_VERSION_MAINTENANCE  " $MODIFYFILE)
NEWSTRING="#define PROGRAM_VERSION_MAINTENANCE  ${3}"

cat $MODIFYFILE | sed s/"${MODIFYSTRING}"/"${NEWSTRING}"/ > $TEMPFILE
mv $TEMPFILE $MODIFYFILE

MODIFYSTRING=$(grep "PROGRAM_VERSION_STRING" $MODIFYFILE)
NEWSTRING="#define PROGRAM_VERSION_STRING \"${1}.${2}.${3}${SUFFIX}\""

cat $MODIFYFILE | sed s/"${MODIFYSTRING}"/"${NEWSTRING}"/ > $TEMPFILE
mv $TEMPFILE $MODIFYFILE

MODIFYSTRING=$(grep "RESOURCE_VERSION_STRING" $MODIFYFILE)
MODIFYSTRING=$(echo $MODIFYSTRING | sed s/"...$"//)
NEWSTRING="#define RESOURCE_VERSION_STRING \"${1},${2},${3}"

cat $MODIFYFILE | sed s/"${MODIFYSTRING}"/"${NEWSTRING}"/ > $TEMPFILE
mv $TEMPFILE $MODIFYFILE

MODIFYSTRING=$(grep "PROGRAM_RELEASE_NUMBER" $MODIFYFILE)
OLDRELEASE=$(grep "PROGRAM_RELEASE_NUMBER" $MODIFYFILE | sed "s/[^0-9]//g")
((NEWRELEASE=OLDRELEASE+1))
echo "Increased release number from ${OLDRELEASE} to ${NEWRELEASE}"
NEWSTRING=$(grep "PROGRAM_RELEASE_NUMBER" $MODIFYFILE | sed "s/$OLDRELEASE/$NEWRELEASE/")

cat $MODIFYFILE | sed s/"${MODIFYSTRING}"/"${NEWSTRING}"/ > $TEMPFILE
mv $TEMPFILE $MODIFYFILE

##### EmulationStation-DE_Info.plist

MODIFYFILE=../es-app/assets/EmulationStation-DE_Info.plist
MODIFYSTRING=$(grep "<string>EmulationStation Desktop Edition " $MODIFYFILE)
OLDVERSION=$(echo $MODIFYSTRING | cut -f4 -d" " | sed s/".........$"//)
MODIFYSTRING=$(echo $MODIFYSTRING | sed s/".........$"//)
NEWSTRING="<string>EmulationStation Desktop Edition ${1}.${2}.${3}"

cat $MODIFYFILE | sed s/"${MODIFYSTRING}"/"${NEWSTRING}"/ > $TEMPFILE
mv $TEMPFILE $MODIFYFILE

MODIFYSTRING=$(grep -m1 "<string>${OLDVERSION}" $MODIFYFILE)
MODIFYSTRING=$(echo $MODIFYSTRING | sed s/".........$"//)
# Adding the suffix is not fully compliant with the Apple documentation but seems to be working.
# It's not used for the release builds anyway so it should hopefully not be an issue.
NEWSTRING="<string>${1}.${2}.${3}${SUFFIX}"

cat $MODIFYFILE | sed s/"${MODIFYSTRING}"/"${NEWSTRING}"/ > $TEMPFILE
mv $TEMPFILE $MODIFYFILE

echo "Done updating, don't forget to run generate_man_page.sh once the binary has been compiled with the new version string."
