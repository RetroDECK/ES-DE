#!/usr/bin/bash
#
# generate_man_page.sh
# Generate a Unix manual page for EmulationStation Desktop Edition.
#
# This script only takes an optional argument which is the location to the ES binary.
# If not provided, the default path will be searched for the binary 'emulationstation'.
#
# The man page will be generated in the same directory as where the script is executed.
#
# The command help2man must be installed, or the script will fail.
#
# Leon Styhre
# 2020-07-16
#

if [ $# -ne 1 ]; then
  ESBINARY=emulationstation
else
  ESBINARY=$1
fi

TEMPFILE_INPUT=tempfile_input_$(date +%H%M%S)
TEMPFILE_OUTPUT=tempfile_output_$(date +%H%M%S)
TARGET_FILENAME=emulationstation.6

MAN_INCLUDE="
[NAME]
emulationstation - EmulationStation Desktop Edition

[DESCRIPTION]
EmulationStation Desktop Edition is a feature-rich front-end for browsing and launching games from your multi-platform retro game collection.

It's intended to be used in conjunction with emulators such as the RetroArch cores.

[AUTHOR]
Alec \"Aloshi\" Lofquist (original version)

RetroPie Community (RetroPie fork)

Leon Styhre (Desktop Edition fork)

[SEE ALSO]
Full documentation is available at: <https://gitlab.com/leonstyhre/emulationstation-de/>
"

echo "${MAN_INCLUDE}" > $TEMPFILE_INPUT

help2man --section 6 --no-info --include $TEMPFILE_INPUT $ESBINARY > $TEMPFILE_OUTPUT

# Manual string replacements, these may need to be modified if changes are made to the
# command line --help output.
cat $TEMPFILE_OUTPUT | sed s/"EmulationStation Desktop Edition, Emulator Front\\\-end"/""/g | \
sed s/"Set to at least"/".br\nSet to at least"/ > $TARGET_FILENAME

gzip $TARGET_FILENAME

rm $TEMPFILE_INPUT
rm $TEMPFILE_OUTPUT
