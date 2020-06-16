#!/usr/bin/bash
#
# mame_create_index_files.sh
# EmulationStation MAME index files creation.
#
# As input, this script takes the MAME driver information XML file from the official
# MAME release and generates the files mamebioses.xml, mamedevices.xml and mamenames.xml.
#
# There is not much error checking going on here, this script is not intended to be
# used by the end user.
#
# xmlstarlet must be installed or this script will fail.
#
# Download the driver file from here:
# https://www.mamedev.org/release.php
# It's enough to download the driver information, not the complete emulator.
#
# Leon Styhre
# 2020-06-16
#

if [ $# -ne 1 ]; then
  echo "Usage: ./mame_create_index_files.sh <MAME driver file>"
  echo "For example:"
  echo "./mame_create_index_files.sh mame0221.xml"
  exit
fi

if [ ! -f $1 ]; then
  echo "Can't find MAME driver file" $1 
  exit
fi

MAME_XML_FILE=$1
MAMEBIOSFILE=mamebioses.xml
MAMEDEVICEFILE=mamedevices.xml
MAMENAMEFILE=mamenames.xml

echo "<!-- Latest updates from MAME driver file" $1 "-->" > $MAMEBIOSFILE 

for bios in $(xmlstarlet sel -t -m "/mame/machine[@isbios=\"yes\"]" -v "@name" -n $MAME_XML_FILE); do
  echo "<bios>"${bios}"</bios>" >> $MAMEBIOSFILE
done

echo "<!-- Latest updates from MAME driver file" $1 "-->" > $MAMEDEVICEFILE

for device in $(xmlstarlet sel -t -m "/mame/machine[@isdevice=\"yes\"][rom]" -v "@name" -n $MAME_XML_FILE); do
  echo "<device>"${device}"</device>" >> $MAMEDEVICEFILE
done

echo "<!-- Generated from MAME driver file" $1 "-->" > $MAMENAMEFILE

xmlstarlet sel -t -m "/mame/machine[not(@isbios=\"yes\")][not(@isdevice=\"yes\")][rom]" -v "@name" -o " " -v description -n $MAME_XML_FILE | \
awk '{ print "<mamename>" $1 "</mamename>"; print $1=""; print "<realname>" $0 "</realname>"}' | \
sed s/"realname> "/"realname>"/g | sed '/^[[:space:]]*$/d' | sed s/"<mamename"/"<game>\n\t<mamename"/g | \
sed s/"<realname"/"\t<realname"/g | sed s/"<\/realname>"/"<\/realname>\n<\/game>"/g >> $MAMENAMEFILE

