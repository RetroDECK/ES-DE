# EmulationStation Desktop Edition (ES-DE)

EmulationStation Desktop Edition is a frontend for browsing and launching games from your multi-platform game collection.

Web site:\
[https://es-de.org](https://es-de.org)

YouTube channel with installation instruction videos:\
[https://www.youtube.com/channel/UCosLuC9yIMQPKFBJXgDpvVQ](https://www.youtube.com/channel/UCosLuC9yIMQPKFBJXgDpvVQ)

Twitter:\
[https://twitter.com/ESDE_Frontend](https://twitter.com/ESDE_Frontend)

Discord server:\
[https://discord.gg/3jmbRXFe](https://discord.gg/3jmbRXFe)

Although there are multiple forks of EmulationStation in existence, the goal of this edition is to make the software as easy as possible to install and use as a standalone application, and to support the major desktop operating systems, i.e. Unix/Linux, macOS and Windows. Since it's intended to be used as a desktop application rather than as the primary interface for the computer, there are no plans to provide system administration utilities or control over emulator settings from within ES-DE.

The software comes preconfigured for use primarily with [RetroArch](https://www.retroarch.com), although this can be changed as all emulator settings are fully configurable, even on a per-game basis.

A comprehensive theme set, **rbsimple-DE** which is based on Recalbox Multi from the [Recalbox](https://www.recalbox.com) community is bundled with the application. (Only assets created by them prior to their change to a more restrictive license in 2018 are included).

The following operating systems have been tested with ES-DE (all for the x86 architecture unless otherwise stated):

* Ubuntu 20.04 to 21.04
* Linux Mint 20
* Manjaro 21
* Fedora 33 Workstation
* FreeBSD 12.2
* NetBSD 9.1
* OpenBSD 6.8
* macOS 10.14 "Mojave" to 11 "Big Sur" (ARM via Rosetta 2 is supported)
* macOS 10.11 "El Capitan" (v1.0 release only)
* Windows 10
* Windows 8.1

**Note:** If using a Mac with an ARM CPU (e.g. M1) you need to install the x86 version of RetroArch and any other emulators, or you won't be able to launch any games. This will be fixed whenever a native macOS ARM build of ES-DE is released.

### Download

The latest version is 1.1.0 (released 2021-08-10)

| Package             | Operating systems                                       | Architecture | Download link  |
| :------------------ | :------------------------------------------------------ | :----------- | :------------- |
| Debian DEB package  | Ubuntu 20.04 to 21.04, Linux Mint 20, possibly others   | x64 (x86)    | [emulationstation-de-1.1.0-x64.deb](https://gitlab.com/leonstyhre/emulationstation-de/-/package_files/14892301/download)|
| Fedora RPM package  | Fedora Workstation 33, possibly others                  | x64 (x86)    | [emulationstation-de-1.1.0-x64.rpm](https://gitlab.com/leonstyhre/emulationstation-de/-/package_files/14892436/download)|
| Debian DEB package  | Raspberry Pi OS (Raspian) - preview release             | ARM          | [emulationstation-de-1.1.0-preview-armv7l.deb](https://gitlab.com/leonstyhre/emulationstation-de/-/package_files/14892276/download)|
| macOS DMG installer | macOS 10.14 "Mojave" to 11 "Big Sur"                    | x64 (x86)    | [EmulationStation-DE-1.1.0-x64.dmg](https://gitlab.com/leonstyhre/emulationstation-de/-/package_files/14892342/download)|
| Windows installer   | Windows 10 and 8.1                                      | x64 (x86)    | [EmulationStation-DE-1.1.0-x64.exe](https://gitlab.com/leonstyhre/emulationstation-de/-/package_files/14892429/download)|

The Raspberry Pi 4 release is a preview only, there are some bugs such as graphical glitches on this platform and some features like GLSL shaders are missing. Official support will probably be added for this device as of v1.2.

Unfortunately due to technical reasons, v1.0.1 (released 2021-05-01) will be the last version for legacy macOS.

| Package             | Operating systems                                       | Architecture | Download link  |
| :------------------ | :------------------------------------------------------ | :----------- | :------------- |
| macOS DMG installer | Legacy macOS 10.11 "El Capitan" to 10.13 "High Sierra"  | x64 (x86)    | [EmulationStation-DE-1.0.1-x64_legacy.dmg](https://gitlab.com/leonstyhre/emulationstation-de/-/package_files/14888836/download)|

For some of the rarer operating systems listed at the top you need to build ES-DE from source. The plan is to have ES-DE available in the software repositories of all supported operating systems (where this is applicable), so in the long term building from source should not be required.

Please refer to [CHANGELOG.md](CHANGELOG.md) for more information about the releases, including known issues.


### Contributing

If you would like to contribute to the development of ES-DE, then that's great! Please read how to participate [here](CONTRIBUTING.md). (You can also read about planned future releases and features there).


# Additional information

[USERGUIDE.md](USERGUIDE.md) is a comprehensive guide on how to use ES-DE and it also contains a complete reference to all application settings.

[CHANGELOG.md](CHANGELOG.md) contains information about the current release as well as previous releases. This covers the features, improvements, bug fixes and known issues.

[INSTALL.md](INSTALL.md) provides details on how to build the application from source code, and also discusses some more advanced configuration topics.

[CREDITS.md](CREDITS.md) is an attempt to credit the individuals and projects which made ES-DE possible.

[THEMES.md](THEMES.md) is a guide on how theming works which is useful for those who would like to develop a new theme, or perhaps customize an existing theme.

# Some feature highlights

Here are some highlights of what EmulationStation Desktop Edition provides, displayed using the default theme set rbsimple-DE. There are of course many more features available, as covered in the  [User guide](USERGUIDE.md).

![alt text](images/current/es-de_system_view.png "ES-DE System View")
_The **System view**, which is the default starting point for the application, it's here that you browse through your game systems._

![alt text](images/current/es-de_gamelist_view.png "ES-DE Gamelist View")
_The **Gamelist view**, it's here that you browse the games for a specific system. Note the support for mixing files and folders, and as well that favorite games are marked with stars. There is a game counter to the upper right, displaying the total number of games and the number of favorite games for this system._

![alt text](images/current/es-de_folder_support.png "ES-DE Folder Support")
_Another example of the gamelist view, displaying advanced folder support. You can scrape folders for game info and game media, sort folders as you would files, mark them as favorites etc. In this example ES-DE has been configured to sort favorite games above non-favorites._

![alt text](images/current/es-de_custom_collections.png "ES-DE Custom Collections")
_Games can be grouped into your own custom collections, in this example they're defined as game genres but you can name them anything you like. All gamelist views including the custom collections support both game images or game videos. By default the rbsimple-DE theme will display the game image for a short moment before starting to play the video._

![alt text](images/current/es-de_scraper_running.png "ES-DE Scraper Running")
_This is a view of the built-in scraper which downloads game info and game media from either [screenscraper.fr](https://screenscraper.fr) or [thegamesdb.net](https://thegamesdb.net). It's possible to scrape a single game, or to run the multi-scraper which can scrape a complete game system or even your entire collection._

![alt text](images/current/es-de_scraper_settings.png "ES-DE Scraper Settings")
_There are many settings for the scraper including options to define which type of info and media to download. The above screenshot shows only a portion of these settings._

![alt text](images/current/es-de_metadata_editor.png "ES-DE Metadata Editor")
_In addition to the scraper there is a fully-featured metadata editor that can be used to modify information on a per-game basis. Here you can also toggle some additional flags which the scraper does not set, such as if the game is a favorite or if you have completed it. Some of these flags can then be filtered in the gamelist view, letting you for instance only display games that you have not played through._

![alt text](images/current/es-de_screensaver.png "ES-DE Screensaver")
_There are four types of built-in screensavers available, including a slideshow and the video screensaver shown in action above. These screensavers start after a configurable number of minutes of inactivity, and randomly display game media that you have previously scraped. If the corresponding option has been enabled, you can jump to the game from the screensaver, or even start it directly. There is shader support in ES-DE to render scanlines and screen blur on top of the videos (for the slideshow screensaver, scanline rendering is provided)._

![alt text](images/current/es-de_ui_theme_support.png "ES-DE Theme Support")
_ES-DE is fully themeable, so if you prefer another look than what the default theme rbsimple-DE gives you, it's possible to apply another theme set. In the example above a modified version of the [Fundamental](https://github.com/G-rila/es-theme-fundamental) theme is used. Be aware though that although ES-DE is backwards compatible with older EmulationStation themes, some newer features which are specific to ES-DE will not work, at least not until the theme authors update their themes._

![alt text](images/current/es-de_ui_easy_setup.png "ES-DE Easy Setup")
_A lot of effort has been spent on making ES-DE easy to setup and use. The above screenshot shows the dialog if starting the application without any game files present in the default ROM directory. ES-DE also ships with a comprehensive game systems configuration file, so unless you really want to customize your setup, you should not need to tinker with the configuration._