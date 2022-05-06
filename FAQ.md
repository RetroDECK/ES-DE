# EmulationStation Desktop Edition (ES-DE) - Frequently Asked Questions

## What is this project and how is it related to other EmulationStation forks?

This project started in 2020 as a fork of RetroPie EmulationStation and it has been in very active development ever since. Large parts of the application have been rewritten and much functionality has been added, so overall it's a quite different application by now. It's a completely independent project from all other EmulationStation forks.

## What's the correct name? EmulationStation, ES-DE, Emulation Station, EmuStation etc?

The correct name is EmulationStation Desktop Edition, which is often shortened to ES-DE for practical reasons. It's not spelled Emulation Station in the same manner as you don't write Sony Play Station or Nintendo Game Cube.

## Is this software available for free, and is it open source?

ES-DE is available for free, and will continue to be available for free. It's released under the MIT open source license with the source code being publicly and freely available. Voluntary donations to support the project are however very welcome.

## Which operating systems are supported?

ES-DE runs on Windows, macOS and BSD Unix as well as on multiple Linux distributions, including SteamOS (Steam Deck). The Raspberry Pi 4/400 is also supported.

## What is the relationship between ES-DE and EmuDeck?

ES-DE and EmuDeck are completely different projects, but we are friends and we collaborate to give the best possible user experience. EmuDeck is an installation script that downloads emulators and applies configuration to these, and it can also download and install ES-DE. EmuDeck is not needed to run ES-DE, but on the Steam Deck it provides a fast and convenient way of setting up an emulator environment, so its use is recommended.

## What systems/platforms and emulators are supported by ES-DE?

See the _Supported game systems_ section at the bottom of the [User guide](USERGUIDE.md#supported-game-systems) where there's a table listing all supported systems/platforms and emulators.

## I don't like the default emulator for a certain system, how can I choose an alternative?

ES-DE comes preconfigured with support for many alternative emulators, see the [What's new in ES-DE v1.2](https://www.youtube.com/watch?v=rpnMJqceSNk) video on the ES-DE YouTube channel for a brief overview of how this works. More emulators are continuously being added with each release.

## I'm on Windows and ES-DE can't find my emulators, what is wrong?

On Windows ES-DE is shipped as a portable installation and as a regular installer. If you're using the portable installation you need to drop your emulators inside the Emulators directory. Make sure to read the README.txt file directly in the EmulationStation-DE folder for more details. For the regular installer many emulators do not provide a method to inform ES-DE where they are installed, so you will need to add their installation directories to the Path environment variable in Windows. It's strongly recommended to read the _Specific notes for Windows_ section of the [User guide](USERGUIDE.md#specific-notes-for-windows) before attempting to setup and use ES-DE on Windows.

## The emulator configuration is all wrong, what's going on?

ES-DE acts strictly as a frontend, it does not interfere at all with the emulator configuration. So you need to configure your emulators separately. This includes your controllers as the controller configuration inside ES-DE will have no effect on the emulators. You could of course also use an automated tool to setup your emulator environment, such as using EmuDeck on the Steam Deck.

## How do I exit back from the emulators to ES-DE?

This is related to the previous question, ES-DE does not perform any emulator configuration or general system configuration. So you would either need to configure an exit button combination inside your emulator, or use a third party tool to map for instance Alt + F4 or Command + Q to a certain button combination on your controller. If you use RetroArch then it's easy to setup a button combination to exit back to ES-DE, which will apply to all cores. Refer to the RetroArch documentation or their support channels on how to accomplish this.

## How do I add more themes?

Most RetroPie EmulationStation theme sets will work with ES-DE, and there are numerous resources online on where to find these. How to install them is described in the _Themes_ section of the [User guide](USERGUIDE.md#themes). Just be aware that some of these themes do not include support for modern systems like PlayStation 3 and Nintendo Switch so those platforms may look a bit ugly depending on how the theme is written.

## The themes I've added don't seem to work, what's wrong?

Only RetroPie EmulationStation themes are supported, you can't use themes that were specifically developed for Batocera or Recalbox EmulationStation. A very few RetroPie themes like es-theme-carbon-2021 will not work either due to technical reasons.

## I used to be a Batocera/Recalbox user and ES-DE can't seem to find some of my games?

ES-DE uses the RetroPie naming conventions. In most cases the ROM directories are identical but for historical reasons some of them unfortunately don't match. For example Nintendo GameCube is called _gc_ in RetroPie and ES-DE while being named _gamecube_ in Batocera and Recalbox. The same is true for some other systems like n3ds vs. 3ds for Nintendo 3DS. See the _Supported game systems_ section at the bottom of the [User guide](USERGUIDE.md#supported-game-systems) where there's a table listing the system names that ES-DE expects.

## Can ES-DE update itself automatically when a new release becomes available?

This functionality is planned but not yet implemented. It will probably be rolled out in two steps, with the first step being a notitication that a new release is available, and at a later stage adding complete in-application update support. For the time being you therefore need to regularly check the https://es-de.org website or join the ES-DE [Discord](https://discord.gg/EVVX4DqWAP) server or [subreddit](https://www.reddit.com/r/EmulationStation_DE/) where new releases are announced.

## I can't find any game media links in the gamelist.xml files, where is this data stored?

ES-DE works completely different compared to all other EmulationStation forks when it comes to handling of game media. There are no links in the gamelist.xml files, instead media files are simply matched against the ROM/game file name which makes for a much simpler, faster and completely portable setup. So replacing a game media file manually just involves copying a new file in place to overwrite the old file, no further updates are needed. Also see the next question below for more information.

## Can I use an external scraper application instead of the built-in scraper?

Yes to a certain extent this is supported and already used by some people. Few if any dedicated scrapers are updated specifically to support ES-DE though, so you may need to do some manual renaming and moving of files and directories for the time being. See the _Manually copying game media files_ section of the [User guide](USERGUIDE.md#manually-copying-game-media-files) for more details about this.

## I'm missing a feature, how can I request to have it added?

First check the project [Kanban](https://gitlab.com/leonstyhre/emulationstation-de/-/boards/1823720) board which contains an overview of planned future features and changes and search for the functionality you would like to see added. Chances are there is already a card on the board describing precisely what you intended to request. You can also check the [Release roadmap](CONTRIBUTING.md) which includes the planned implementation of major features. If you can't find the feature you're looking for, you can request it either via adding an issue directly to the Kanban board, or by asking for it in our [Discord](https://discord.gg/EVVX4DqWAP) server or [subreddit](https://www.reddit.com/r/EmulationStation_DE/).

## I want to setup a gaming appliance based on the Raspberry Pi, can I use ES-DE for this?

While there is a release of ES-DE for the Raspberry Pi 4/400, this requires a desktop environment to run. So ES-DE can not be used as a drop-in replacement for RetroPie or Batocera EmulationStation. It's however still possible to get an appliance-like experience with ES-DE if the necessary setup is performed. But that's the case not only for the Raspberry Pi of course but for all supported operating systems.
