# EmulationStation Desktop Edition (ES-DE) - Frequently Asked Questions

## What is this project and how is it related to other EmulationStation forks?

This project started in 2020 as a fork of RetroPie EmulationStation and it has been in very active development ever since. Large parts of the application have been rewritten and much functionality has been added, so overall it's a quite different application by now. It's a completely independent project from all other EmulationStation forks.

## What's the correct name? EmulationStation, ES-DE, Emulation Station, EmuStation etc?

The correct name is EmulationStation Desktop Edition, which is for practical reasons often shortened to EmulationStation-DE or more commonly ES-DE. It's not spelled Emulation Station (i.e. two separate words) in the same manner as you don't write Sony Play Station or Nintendo Game Cube.

## Is this software available for free, and is it open source?

ES-DE is available for free, and will continue to be available for free. It's released under the MIT open source license with the source code being publicly and freely available. Voluntary donations to support the project are however very welcome.

## Which operating systems are supported?

ES-DE runs on Windows, macOS and BSD Unix as well as on multiple Linux distributions, including SteamOS (Steam Deck). The Raspberry Pi 4/400 is also supported.

## What is the relationship between ES-DE and EmuDeck?

ES-DE and EmuDeck are completely separate projects, but we collaborate to give the best possible user experience. EmuDeck is an installation script that downloads emulators and applies configuration to these, and it can also download and install ES-DE. EmuDeck is not needed to run ES-DE, but on the Steam Deck it provides a convenient way of setting up an emulator environment.

## What systems/platforms and emulators are supported by ES-DE?

See the _Supported game systems_ section at the bottom of the [User guide](USERGUIDE.md#supported-game-systems) where there's a table listing all supported systems/platforms and emulators.

## Why can't I press the up button in menus to jump to the bottom row and vice versa?

Menus in ES-DE are not lists but grids, sometimes there is only a list but sometimes there are buttons beneath the list. Enabling the up and down buttons to wrap around would therefore not work consistently as it would sometimes jump to the last row of the list and sometimes to a button, depending on the menu layout. This type of contextual navigation feels very weird in practice. The solution is instead to use the shoulder buttons (which will jump six rows), or the trigger buttons (which will jump to the first and last row). These buttons work consistently throughout the application and avoid the strange side effects just mentioned. If you observe this more closely you will also realize that "wrap around" is very rare in GUI design, you don't jump to the bottom of a web page when you scroll up from the top in your web browser and likewise the settings menus of your mobile phone will not wrap around.

## I don't like the default emulator for a certain system, how can I choose an alternative?

ES-DE comes preconfigured with support for many alternative emulators, see the [What's new in ES-DE v1.2](https://www.youtube.com/watch?v=rpnMJqceSNk) video on the ES-DE YouTube channel for a brief overview of how this works. More emulators are continuously being added with each release.

## I'm using a Steam Deck and ES-DE seems to crash when I attempt to open the main menu?

You've probably mapped your _Back_ button to F4 which is the default application exit key in ES-DE. To fix this, temporarily unmap the button or plug in an external keyboard into your Steam Deck, start ES-DE, open the menu (via the _Escape_ key if using a keyboard) and find the _Exit button combo_ entry in the _Other settings_ menu. You can set this to either F4, Alt + F4 or Alt + Q.

## I'm on Windows and ES-DE can't find my emulators, what is wrong?

On Windows ES-DE is shipped as a portable installation and as a regular installer. If you're using the portable installation you need to drop your emulators inside the Emulators directory. Make sure to read the README.txt file directly in the EmulationStation-DE folder for more details. For the regular installer many emulators do not provide a method to inform ES-DE where they are installed, so you will need to add their installation directories to the Path environment variable in Windows. It's strongly recommended to read the _Specific notes for Windows_ section of the [User guide](USERGUIDE.md#specific-notes-for-windows) before attempting to setup and use ES-DE on Windows.

## I'm on Windows and ES-DE refuses to start, is the application broken?

You're probably missing the OpenGL drivers required to run ES-DE. Try to download and install the latest drivers for your graphics card. If you have a really old GPU this may not work though, and you may have to go for OpenGL software rendering instead. How this is setup is described in the _Specific notes for Windows_ section of the [User guide](USERGUIDE.md#specific-notes-for-windows).

## The emulators don't seem to be properly configured?

ES-DE acts strictly as a frontend, it does not interfere with the emulator configuration. The only instance when ES-DE passes specific options to an emulator is when there is no choice, like when fullscreen mode can't be enabled except via a command line option, or when a flag is needed to place the emulator in batch mode to work properly with a frontend. So you need to configure your emulators separately. This includes your controllers as well since the controller configuration inside ES-DE will have no effect on the emulators.

## On game launch the screen just flashes black and the game won't start, how do I solve this?

This is almost always caused by either corrupt ROMs/disc images or by missing emulator BIOS files (which have to be installed separately from ES-DE).

## How do I exit back from the emulators to ES-DE?

This is related to the two questions above, ES-DE does not perform any emulator configuration or general system configuration. So you would either need to configure an exit button combination inside your emulator, or use a third party tool to map for instance Alt + F4 or Command + Q to a certain button combination on your controller. If you use RetroArch then it's easy to setup a button combination to exit back to ES-DE, which will apply to all cores. Refer to the RetroArch documentation or their support channels on how to accomplish this.

## I have many games with multiple files, is there a way to show these as single entries?

Yes this is supported for both single-disc games using a .bin/.cue structure for instance, or for multi-file/multi-disc games where .m3u files are used for emulator disc swapping. See the _Directories interpreted as files_ section of the [User guide](USERGUIDE.md#directories-interpreted-as-files) for details on how to configure this.

## I see both .bin and .cue files for many of my games, how can I hide the unnecessary files?

See the question above for a possible solution. Another approach would be to hide the game files you don't want to see using the metadata editor. Yet another solution which is actually recommended is to convert your games to the .chd format. This combines the .bin/.cue files into a single file that is also compressed, leading to space savings. Just be aware that you can't combine multi-disc games into a single .chd file. A custom systems configuration entry could also be created, but that is not really recommended or required due to the solutions just mentioned. The reason why .bin files are included in the first place is that some emulators can launch these files directly, and some users have game collections comprised of such files. If the .bin files were filtered out in the bundled configuration, then all these users would have their games removed from within ES-DE next time they upgrade the application.

## When I hide a game using the metadata editor it's not really getting hidden, is this a bug?

No, by default games are not removed from the gamelists when they are hidden and are instead only marked with a much lower text opacity. You need to disable the setting _Show hidden games (requires restart)_ from the _Other settings_ menu to make them disappear entirely. The reason this option is not disabled by default is that new users could very easily make a mistake by hiding some files accidentally without realizing it, only to have the entries being immediately removed from the gamelist view. It's also good practice to hide all your games with this option enabled and verify that it's all correct before going ahead and disabling it.

## I'm using Linux or macOS and I can't find the .emulationstation directory, where is it located?

The .emulationstation directory is normally located in your home directory, but on these Unix-based operating systems files and directories starting with a dot are hidden by default. So you need to enable hidden files and directories in your file manager. On macOS this is done in Finder using the Shift + Command + . (a dot) keyboard combination. On Linux it depends on which file manager you're using, but in Dolphin it's accomplished by using the Alt + . (a dot) keyboard combination or via the corresponding entry in the hamburger menu.

## Is there a way to customize existing systems, and/or to add more systems than those shipped by default?

Yes it's possible to both customize existing systems that are part of the bundled configuration as well as to add more systems than those shipped with ES-DE. Almost nothing is hardcoded in the application so there is a huge flexibility when it comes to such configuration. How this is done is covered in the _Game system customizations_ section of the [User guide](USERGUIDE.md#game-system-customizations). Just make sure to never modify the es_systems.xml and es_find_rules.xml files included in the application installation directory as these will be overwritten when upgrading ES-DE in the future. Always place your customizations in ~/.emulationstation/custom_systems/ as is also described in the user guide.

## How do I add more themes?

Most RetroPie EmulationStation theme sets will work with ES-DE, and there are numerous resources online on where to find these. How to install them is described in the _Themes_ section of the [User guide](USERGUIDE.md#themes). Just be aware that some of these themes do not include support for modern systems like PlayStation 3 and Nintendo Switch so those platforms may look a bit ugly depending on how the theme is written.

## The themes I've added don't seem to work?

Only RetroPie EmulationStation themes are supported, you can't use themes that were specifically developed for Batocera or Recalbox EmulationStation. A very few RetroPie themes like es-theme-carbon-2021 will not work either due to technical reasons.

## I used to be a Batocera/Recalbox user and ES-DE can't seem to find some of my games?

ES-DE uses the RetroPie naming conventions. In most cases the ROM directories are identical but for historical reasons some of them unfortunately don't match. For example Nintendo GameCube is called _gc_ in RetroPie and ES-DE while being named _gamecube_ in Batocera and Recalbox. The same is true for some other systems like n3ds vs. 3ds for Nintendo 3DS. See the _Supported game systems_ section at the bottom of the [User guide](USERGUIDE.md#supported-game-systems) where there's a table listing the system names that ES-DE expects.

## Can ES-DE update itself automatically when a new release becomes available?

This functionality is planned but not yet implemented. It will probably be rolled out in two steps, with the first step being a notitication that a new release is available, and at a later stage adding complete in-application update support. For the time being you therefore need to regularly check the https://es-de.org website or join the ES-DE [Discord](https://discord.gg/EVVX4DqWAP) server or [subreddit](https://www.reddit.com/r/EmulationStation_DE/) where new releases are announced. The process to manually upgrade ES-DE is covered in the _Upgrading to a newer release_ section of the [User guide](USERGUIDE.md#upgrading-to-a-newer-release).

## I can't find any game media links in the gamelist.xml files, where is this data stored?

ES-DE works very differently compared to all other EmulationStation forks when it comes to handling of game media. There are no links in the gamelist.xml files, instead media files are simply matched against the ROM/game file names which makes for a much simpler, faster and completely portable setup. Migrating game media from other EmulationStation forks (and potentially from other frontends as well) can be accomplished quite easily. See the next question below for more information. Make sure to also read the _Migrating from other EmulationStation forks_ section of the [User guide](USERGUIDE.md#migrating-from-other-emulationstation-forks) to avoid data loss if running ES-DE with existing data from another EmulationStation fork.

## Can I use an external scraper application instead of the built-in scraper?

Yes to a certain extent this is supported and at least [Skraper](https://www.skraper.net) and [Skyscraper](https://github.com/muldjord/skyscraper) have been used by some people. Few if any dedicated scraper applications are yet updated specifically to support ES-DE though, so you may need to do some manual renaming and moving of files and directories. See the _Manually copying game media files_ section of the [User guide](USERGUIDE.md#manually-copying-game-media-files) for more details about this.

## My controller isn't working in ES-DE, is there a way to fix this?

If the controller works in other applications and games but not in ES-DE, then you may be able to get it to run in ES-DE as well. The required setup is described in detail in the _Adding custom controller profiles_ section of the [Building and advanced configuration](INSTALL.md#adding-custom-controller-profiles) document.

## I'm missing a feature, how can I make a request to have it added?

First check the project [Kanban](https://gitlab.com/es-de/emulationstation-de/-/boards/1823720) board which contains an overview of planned future features and changes and search for the functionality you would like to see added. Chances are there is already a card on the board describing precisely what you intended to request. You can also check the [Release roadmap](CONTRIBUTING.md) which includes the planned implementation of major features. If you can't find the feature you're looking for, you can request it either via adding an issue directly to the Kanban board, or by asking for it in our [Discord](https://discord.gg/EVVX4DqWAP) server or [subreddit](https://www.reddit.com/r/EmulationStation_DE/).

## I want to setup a gaming appliance based on the Raspberry Pi, can I use ES-DE for this?

While there is a release of ES-DE for the Raspberry Pi 4/400, this requires a desktop environment to run. So ES-DE can not be used as a drop-in replacement for RetroPie or Batocera EmulationStation. It's however still possible to get an appliance-like experience with ES-DE if the necessary setup is performed. But that's the case not only for the Raspberry Pi of course but for all supported operating systems.
