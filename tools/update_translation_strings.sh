#!/usr/bin/bash
#  SPDX-License-Identifier: MIT
#
#  ES-DE Frontend
#  update_translation_strings.sh
#
#  Extracts translation strings from all C++ files, generates the es-de.pot PO template file
#  and merges any changes with the per-language PO files.
#
#  To add a new language append it to locale/languages and run msginit, for example:
#  msginit -i es-de.pot --locale=sv_SE -o po/sv_SE.po
#
#  This script is only intended to be used on Linux systems.
#

if [ ! -f ../es-app/CMakeLists.txt ]; then
  echo "You need to run this script from within the tools directory."
  exit
fi

if [ ! $(which xgettext 2>/dev/null) ]; then
  echo "Can't find xgettext which is required to run this script"
  exit
fi

find ../es-app/src/ ../es-core/src -name '*.cpp' -o -name '*.h' | xgettext -f - -o ../locale/es-de.pot -k_ -k_n:1,2 -k_p:1c,2 -k_np:1c,2,3 --no-location \
--copyright-holder="Northwestern Software AB" --package-name="ES-DE Frontend" --msgid-bugs-address "info@es-de.org"

sed -i "1s/.*/# ES-DE Frontend translation strings./" ../locale/es-de.pot
sed -i "2s/.*/# Copyright (c) 2024 Northwestern Software AB/" ../locale/es-de.pot
sed -i "4s/.*/# Northwestern Software <info@es-de.org>, 2024./" ../locale/es-de.pot
sed -i "s/Language-Team: LANGUAGE <LL@li.org>/Language-Team: LANGUAGE <info@es-de.org>/" ../locale/es-de.pot

for language in $(cat ../locale/languages); do
  echo Merging strings for locale $language
  msgmerge ../locale/po/${language}.po ../locale/es-de.pot -o ../locale/po/${language}.po
  echo Compiling message catalog for locale $language
  mkdir -p ../resources/locale/${language}/LC_MESSAGES
  msgfmt -c --statistics -o ../resources/locale/${language}/LC_MESSAGES/${language}.mo ../locale/po/${language}.po
  echo
done
