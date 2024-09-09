# ES-DE Frontend - Translations

This document is intended for translators who want to contribute localizations to ES-DE.

Table of contents:

[[_TOC_]]

## Introduction

ES-DE has full localization support which means it can be translated to different languages. Adding support for a new locale does however require some minor code changes, so if you're interested in translating to a new locale you need to request support for it first. The best approach is to join our Discord server where we have a dedicated translations channel:

https://discord.gg/42jqqNcHf9

Translation updates are handled manually via this Discord server. As some translators are not familiar with using tools such as Git it was deemed simplest to coordinate all translations there. It means you can upload your translations to the channel and they will be incorporated into the ES-DE source repository.

## License and copyright

ES-DE is released under the MIT license which is a permissive license that allows commercial use. Any translation work will as such be MIT licensed too. This is clearly indicated in the .po message catalog files that are used as the basis for the translation work. By contributing translations to ES-DE you'll also agree to transferring your copyright to the project and its owning company Northwestern Software AB. The copyright owner is also clearly indicated in the .po message catalog files.

## High level approach

There are two types of translations in ES-DE, the first one is specific to Android and contains strings for the onboarding configurator and the second is using [gettext and libintl](https://www.gnu.org/software/gettext) to provide translations to the overall application.

The Android-specific part is quite limited with only a few strings. You can find the latest English version of this file here:

https://gitlab.com/es-de/emulationstation-de/-/blob/master/locale/android_strings.xml

There is not much more to it when it comes to these strings, just translate them and provide the XML file via Discord and they will be added to the Android release.

However the overwhelming majority of the translations are done using gettex/libintl, and the way this works is via so-called _message catalog files_ where there's one such file per supported locale. When adding support for a new locale such a file will be added to the ES-DE repository. These files which have the .po file extension (for _Portable Object_) can be found here:

https://gitlab.com/es-de/emulationstation-de/-/tree/master/locale/po

Note that all .po files are named after the locale. This is always in the form of _language code_ plus _country code_. For example _sv_SE.po_ where _sv_ is the language code for Swedish and _SE_ is the country code for Sweden. There are often country-specific variations. For example there's also an sv_FI locale for Swedish (Finland). If you want to add translations for a specific locale such as German (Austria) or English (United Kingdom) then this is therefore possible.

When using ES-DE the specific locale you have configured in your operating systems will be searched for and applied, and if this does not exist then the default locale for your language will be selected such as falling back to sv_SE if you have sv_FI set as your language. If there is no support at all for your language then a fallback will take place to the default application language _English (United States)_.

You can test your translations quite easily as explained later in this document, and when you want to have your updates added to the ES-DE repository you can share the updated .po file in the Discord server.

When working on translations it's also a good idea to refer to existing translations for other languages as they may provide useful insights for best approaches and such.

## Tools

It's highly recommended to use Poedit when working on translations. This tool is free and open source and is available on Linux, macOS and Windows:

https://poedit.net

Poedit can also compile the .mo files needed by ES-DE to apply the actual translations, so it's required in order to test your translations (unless you use the gettext utilities directly to compile the .mo file).

## Translations in practice

When support has been added to ES-DE for a certain language a corresponding .po file will be added to the ES-DE repository at the following location:

https://gitlab.com/es-de/emulationstation-de/-/tree/master/locale/po

You simply download this file and open it in Poedit to start working on your translations.

The way gettext works is that there's a pair of _msgid_ and _msgstr_ entries per text string, and these will be presented as such inside Poedit. The _msgid_ string is the literal string in the default _English (United States)_ locale as it's presented inside ES-DE. There is a slight exception for hinting as explained later in this document but in general you simply see the literal text that needs translations and then you add your own translation following this. An entry inside the .po file would look something like this:
```
msgid "Permission problems?"
msgstr "Åtkomstproblem?"
```

This is for the Swedish translation in the sv_SE.po file.

In addition to this some strings contain a _format specifier_. This makes it possible to define where a certain value should be placed inside a string. As the order of words differ between languages this is important. But most often it's simply used to parse the actual string that will be visible inside the application. Here's an example to clarify:
```
#, c-format
msgid "ERROR LAUNCHING GAME '%s' (ERROR CODE %i)"
msgstr "KUNDE INTE STARTA SPELET '%s' (FELKOD %i)"
```

The amount and types of format specifiers in the translated msgstr string must match the source msgid string exactly, or otherwise you'll not be able to compile to .po file and your translations won't work.

Finally there are plural entries where there are different translations based on the numerical amount parsed into the string. The following example will select _%i VALD_ if it's singular and _%i VALDA_ if it's plural in Swedish, even though there is no distinction between the two in the English language:
```
#, c-format
msgid "%i SELECTED"
msgid_plural "%i SELECTED"
msgstr[0] "%i VALD"
msgstr[1] "%i VALDA"
```

If you're translating to a language where there is no distinction between the two then you simply set the same value for both entries. If using Poedit all this will be easily handled by the user interface where you'll have separate tabs for the singular and plural entries.

As a general remark the correct letter case is very important for the translated text. Although there are a few instances where text is for example automatically converted to uppercase, in most instances such conversions are not made. This means that in most cases the translated text will appear exactly as entered in the .po file. This approach provides maximum flexibility and of course a number of languages don't even have the concept of letter case so automatic case conversions wouldn't make sense.

## Context information

As there is sometimes ambiguity regarding translated strings, such as the same word having different meanings depending on the context, there is contextual hinting added to a number of the translation strings. Similarly some strings may need short versions for some languages as they may otherwise not fit inside the user interface. For the latter it's really a per-case thing and you'll need to test your translations to see what fits inside the interface and what doesn't. If you need context information added for a string then bring it up in the Discord server and it will get added to the application.

If you are translating to a language with excessively long words (Swedish is such a language) then it may be required to adjust the overall font sizes in ES-DE for this specific language. At the moment this is only applicable to the menu titles as these are quite restricted in length. If you find that you're constantly running out of space for your text then bring it up in the Discord server and a font size adjustment can be made in ES-DE for your specific locale.

Here's an example of a context information that is applicable for the Swedish language:
```
msgid "COMPLETED"
msgstr "SLUTFÖRD"
```

```
msgctxt "metadata"
msgid "COMPLETED"
msgstr "KLARAT"
```

In general _completed_ is translated as _slutförd_ but for example when having played through an entire game (as indicated in the metadata editor for the game) the word _klarat_ makes more sense. Although you could use _slutförd_ for a completed game this sounds pretty strange in Swedish.

However the English translations for this would be identical as there is no real distinction there:
```
msgid "COMPLETED"
msgstr "COMPLETED"
```

```
msgctxt "metadata"
msgid "COMPLETED"
msgstr "COMPLETED"
```

Here's also an example of a short version string:
```
msgid "GAMES DEFAULT SORT ORDER"
msgstr "GAMES DEFAULT SORT ORDER"
```

```
msgctxt "short"
msgid "GAMES DEFAULT SORT ORDER"
msgstr "DEFAULT SORT ORDER"
```

The short version of this string was required as it would otherwise not fit inside the menu header. Note that short strings may only be required for some specific languages, so again you need to test it to see whether you actaully need to provide a short translation or not.

Whenever there's a _msgctxt_ line for a message it will be clearly indicated in Poedit so it's very easy to work with this context information.

## Fuzzy entries

Sometimes when changes are made to translation strings this will cause _fuzzy_ entries to get added to the .po file. This means that gettext detected something has changed but is not sure what to do. In these cases the translator needs to make an explicit decision on how to handle the change. Using Poedit makes the whole process simple as each fuzzy entry is clearly indicated with a _Needs Work_ flag in its user interface.

Say there was the following string in ES-DE:
```
msgid "THEME ASPECT RATIOS"
msgstr "TEMA BILDFÖRHÅLLANDEN"
```

And then it was decided that this should change to _THEME ASPECT RATIO_ instead. When the corresponding code change was done, new .po files were also automatically generated for all languages and committed to the ES-DE repository. However as the string was changed slightly gettext marked it as fuzzy in the .po files, like so:
```
#, fuzzy
msgid "THEME ASPECT RATIO"
msgstr "TEMA BILDFÖRHÅLLANDEN"
```

When an entry is marked as fuzzy it's excluded when compiling the .po file, or in other words it's not getting translated at all.

In this case a slightly updated translation was required, but other times it simply needs to be marked as OK in Poedit. When marking a translation as OK in Poedit or when updating it, the fuzzy flag is removed and the end result would look something like the following for our example:
```
msgid "THEME ASPECT RATIO"
msgstr "TEMA BILDFÖRHÅLLANDE"
```

## Continuous translations

As ES-DE is constantly worked on, translations also need continuous updates. When translation strings are updated or added they will be committed to the ES-DE repository, and such changes will also be discussed and communicated in the Discord server. As well sometimes major features may get added that require additional translation work. To check the status for your translations you can always download the latest .po file from here:

https://gitlab.com/es-de/emulationstation-de/-/tree/master/locale/po

If opening the file in Poedit it will tell you the percentage of translated messages, and if any entries are marked as fuzzy.

## Testing your translations

You can have Poedit compile the binary .mo file whenever you save a .po file. The .mo file (for _Machine Object_) is what ES-DE actually uses to load the translations, i.e. the source .po file is not used when running the application. If not enabled for your setup then you can find this setting inside the Poedit Preferences screen, where it's named _Automatically compile MO file when saving_.

In order to have the .mo file loaded in ES-DE simply create the following directory in your ES-DE application data directory:
```
ES-DE/resources/locale/<locale code>/LC_MESSAGES
```

Then place your .po file there and open it using Poedit. Whenever you save the .po file the .mo file will get generated in the same directory, such as the following example:
```
ES-DE/resources/locale/sv_SE/LC_MESSAGES/sv_SE.mo
ES-DE/resources/locale/sv_SE/LC_MESSAGES/sv_SE.po
```

When there's an .mo file stored there it will override the bundled .mo file and ES-DE will use your local copy instead. This way you can easily test your own translations without having to build ES-DE from source code. Note that you need to restart ES-DE anytime you've compiled a new .mo file.

Also note that this will not work unless support for your language has already been explicitly added to ES-DE.

## Theme translations

In addition to what has been described above there is translation work needed for the actual themes as well. Some portion of the text displayed by the theme engine comes from the application .po files (like the system view game counter and the custom collection summary), but most text is contained within each theme's configuration files.

So to have a fully translated experience it's important to work together with the theme developers so they can incorporate full localization support into their themes. There is also an official system metadata repository available for theme developers to easily include things like game system descriptions and various other information, and the goal is to have this translated to all languages as well.

This repository can be found here:\
https://gitlab.com/es-de/themes/system-metadata

And here is a link to the language section of the theme engine documentation which contains some further relevant information:\
[THEMES.md](THEMES.md#languages)

Discussions regarding theme translations are also covered in the ES-DE Discord server.
