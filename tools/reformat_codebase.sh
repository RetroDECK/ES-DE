#!/usr/bin/bash
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  reformat_codebase.sh
#
#  Automatically reformats the codebase using clang-format.
#  The .clang-format style configuration file in the root of the repository will be
#  used to apply the code formatting to the "es-app/src" and "es-core/src" trees.
#  All files will be formatted in-place meaning they will be overwritten with any changes.
#
#  The purpose of this script is primarily to apply updated formatting as clang-format
#  improves over time, but potentially also to apply any changes to the ES-DE coding style.
#
#  This script is only intended to be used on Linux systems.
#

if [ ! $(which clang-format 2>/dev/null) ]; then
  echo "Can't find clang-format which is required to run this script"
  exit
fi

if [ ! -d ../es-app/src ]; then
  echo "Can't find the ../es-app/src directory, this script must run from the tools directory"
  exit
fi

APP_CPP_NUM_FILES=$(find ../es-app/src -name '*.cpp' | wc -l)
APP_H_NUM_FILES=$(find ../es-app/src -name '*.h' | wc -l)

find ../es-app/src -name '*.cpp' -exec echo clang-format -i {} \;
find ../es-app/src -name '*.cpp' -exec clang-format -i {} \;
find ../es-app/src -name '*.h' -exec echo clang-format -i {} \;
find ../es-app/src -name '*.h' -exec clang-format -i {} \;

CORE_CPP_NUM_FILES=$(find ../es-core/src -name '*.cpp' | wc -l)
CORE_H_NUM_FILES=$(find ../es-core/src -name '*.h' | wc -l)

find ../es-core/src -name '*.cpp' -exec echo clang-format -i {} \;
find ../es-core/src -name '*.cpp' -exec clang-format -i {} \;
find ../es-core/src -name '*.h' -exec echo clang-format -i {} \;
find ../es-core/src -name '*.h' -exec clang-format -i {} \;

echo "Processed" $APP_CPP_NUM_FILES ".cpp files and" $APP_H_NUM_FILES ".h files in es-app/src"
echo "Processed" $CORE_CPP_NUM_FILES ".cpp files and" $CORE_H_NUM_FILES ".h files in es-core/src"
