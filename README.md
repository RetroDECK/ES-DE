EmulationStation Desktop Edition (ES-DE)
========================================

EmulationStation Desktop Edition is a cross-platform graphical front-end for emulators with controller and keyboard navigation.

Maybe you're familiar with EmulationStation: yes there are multiple forks available for this software! However this version is intended for use primarily on desktop computers where it's not the primary interface for the computer. As such, the aim is not to provide full control over emulator settings or emulator button mappings, or include system administration functions and similar. Instead it's assumed that the emulators and the overall environment has been properly configured upfront.

The goal is to make the software as easy as possible to install and use, and to suppport the major desktop operating systems: Unix/Linux, macOS and Windows.

The following operating systems have been tested and confirmed as working with ES-DE v1.0:

* Kubuntu 20.04
* macOS 10.11.6 (El Capitan)
* Windows 10 (x86)
* Windows 8.1 (x86)

The application probably also works fine on devices such as the Raspberry Pi, but this is beyond the scope of this software fork.

The software comes preconfigured for use primarily with [RetroArch](https://www.retroarch.com), although this can be changed as all emulator settings are fully configurable, even on a per-game basis.

A comprehensive theme set, `rbsimple-DE` (which is based on Recalbox Multi from the Recalbox community) is bundled with the application.

Check out the [User Guide](USERGUIDE.md) for how to quickly get the application up and running! \
(It will also show how to use some of its more advanced features.)


### Help needed:

If you would like to contribute to the development of EmulationStation Desktop Edition, please read how to participate [here](CONTRIBUTING.md)! (You can also read about planned future features there).


Other information
=================

[NEWS.md](NEWS.md) contains information about the current release as well as previous releases. This includes features, improvements and bug fixes.

[INSTALL.md](INSTALL.md) provides details on how to build the application from source code, and also discusses more advanced configuration topics.

[CREDITS.md](CREDITS.md) is an attempt to credit the individuals and projects which made this application possible.

[THEMES.md](THEMES.md) is a guide on how theming works and is useful for those who would like to develop a new theme, or perhaps customize an existing theme.

Some feature highlights
=======================

Here are some highlights of what EmulationStation Desktop Edition provides, displayed using the default theme set rbsimple-DE. There are of course many more features available, please refer to the [User Guide](USERGUIDE.md) for a comprehensive overview of all options and functionality!

![alt text](images/v1.0/es-de_v1.0_system_view.png "ES-DE System View")
_The 'System view', which is the default starting point for the application, it's here that you browse through your game systems._

![alt text](images/v1.0/es-de_v1.0_gamelist_view.png "ES-DE Gamelist View")
_The 'Gamelist view', it's here that you browse the games for a specific system. Note the support for mixing files and folders, and as well the favorite games are marked with stars. There is a game counter to the upper right, displaying the total number of games and the number of favorite games for this system._

![alt text](images/v1.0/es-de_v1.0_folder_support.png "ES-DE Folder Support")
_Another example of the gamelist view, displaying advanced folder support. You can scrape folders for game info and game media, sort folders as you would files, mark them as favorites etc. In this example ES-DE has been configured to sort favorite games above non-favorites._

![alt text](images/v1.0/es-de_v1.0_custom_collections.png "ES-DE Custom Collections")
_Games can be grouped into your own custom collections, in this example they're defined as game genres but you can name them to anything you like. All gamelist views including the custom collections support both game images or game videos. By default the rbsimple-DE theme will display the game image for a short moment before starting to play the game video._

![alt text](images/v1.0/es-de_v1.0_scraper_running.png "ES-DE Scraper Running")
_This is a view of the built-in scraper which downloads game info and game media from either [screenscraper.fr](https://screenscraper.fr) or [thegamesdb.net](https://thegamesdb.net). It's possible to scrape a single game, or to run the multi-scraper which can scrape a complete game system or even your entire collection._

![alt text](images/v1.0/es-de_v1.0_scraper_settings.png "ES-DE Scraper Settings")
_There are many settings for the scraper including options to define which type of info and media to download. The above screenshot shows only a portion of all these settings. Also note the blurred and darkened background which is rendered by ES-DE's GLSL shaders whenever a menu is opened._

![alt text](images/v1.0/es-de_v1.0_metadata_editor.png "ES-DE Metadata Editor")
_In addition to the scraper there is a fully-featured metadata editor that can be used to modify information on a per-game basis. Here you can also toggle some additional flags which the scraper does not set, such as if the game is a favorite, or if you have completed it. Some of these flags can then be filtered in the gamelist view, letting you for instance only list games that you have not played through._

![alt text](images/v1.0/es-de_v1.0_screensaver.png "ES-DE Screensaver")
_There are four types of built-in screensavers available, including a slideshow and the video screensaver showed in action above. These screensavers start after a configurable number of minutes of inactivity, and randomly display game media that you have previously scraped. If the corresponding option has been enabled, you can jump to the game from the screensaver, or even start it directly. There is shader support in ES-DE to render scanlines and screen blur on top of the videos (for the slideshow, scanline rendering is provided)._

![alt text](images/v1.0/es-de_v1.0_ui_theme_support.png "ES-DE Theme Support")
_ES-DE is fully themeable, so if you prefer another look than what the default theme rbsimple-DE gives you, it's possible to apply another theme. In the example above a modified version of the [Fundamental](https://github.com/G-rila/es-theme-fundamental) theme is showed. Be aware though that although ES-DE is backwards compatible with older EmulationStation themes, some newer features which are specific to ES-DE will not work, at least not until the theme authors update their themes._

![alt text](images/v1.0/es-de_v1.0_ui_easy_setup.png "ES-DE Easy Setup")
_A lot of effort has been spent on trying to make ES-DE easy to setup and use. The above screenshot shows the dialog if starting the application without any game files present in the default ROM directory. Also, ES-DE ships with a very comprehensive game systems configuration file that is automatically installed upon first startup. Note though that the emulator setup is outside the scope of what ES-DE does, and as RetroArch is mostly used, please refer to their [website](https://www.retroarch.com) for more information about that part of the configuration._