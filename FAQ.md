# EmulationStation Desktop Edition (ES-DE) - Frequently Asked Questions

## What is this project and how is it related to other EmulationStation forks?

This project started in 2020 as a fork of RetroPie EmulationStation and it has been in very active development ever since. Large parts of the application have been rewritten and much functionality has been added, so overall it's a quite different application by now.

## What's the correct name? EmulationStation, ES-DE, Emulation Station, EmuStation etc?

The correct name is EmulationStation Desktop Edition, which is for practical reasons often shortened to EmulationStation-DE or more commonly ES-DE. It's not spelled Emulation Station (i.e. two separate words) in the same manner as you don't write Sony Play Station or Nintendo Game Cube.

## Is this software available for free, and is it open source?

ES-DE is available for free, and will continue to be available for free. It's released under the MIT open source license with the source code being publicly and freely available. Voluntary donations to support the project are however very welcome.

## Which operating systems are supported?

ES-DE runs on Windows, macOS and BSD Unix as well as on multiple Linux distributions including Ubuntu, Fedora, Arch, Manjaro, SteamOS etc.

## What is the relationship between ES-DE and EmuDeck?

ES-DE and [EmuDeck](http://www.emudeck.com) are completely separate projects, but we collaborate to give the best possible user experience. EmuDeck is an installer that downloads emulators and applies configuration to these, and it can also download and install ES-DE. EmuDeck is not needed to run ES-DE, but on the Steam Deck it provides a convenient way of setting up an emulator environment. It's a good idea to read the _Specific notes for Steam Deck_ section of the [User guide](USERGUIDE.md#specific-notes-for-steam-deck) if ES-DE has been installed using EmuDeck.

## What is the relationship between ES-DE and RetroDECK?

ES-DE and [RetroDECK](http://retrodeck.net) are completely separate projects, but we collaborate to give the best possible user experience. RetroDECK bundles ES-DE with all emulators in the same Flatpak so you don't need to update emulators separately or set Flatpak permissions manually. It's a good idea to read the _Specific notes for Steam Deck_ section of the [User guide](USERGUIDE.md#specific-notes-for-steam-deck) if ES-DE has been installed via RetroDECK.

## What game systems/platforms and emulators are supported by ES-DE?

See the _Supported game systems_ section at the bottom of the [User guide](USERGUIDE.md#supported-game-systems) where there's a table listing all supported systems/platforms and emulators.

## Why can't I press the up button in menus to jump to the bottom row and vice versa?

Menus in ES-DE are not lists but grids, sometimes there is only a list but sometimes there are buttons beneath the list. Enabling the up and down buttons to wrap around would therefore not work consistently as it would sometimes jump to the last row of the list and sometimes to a button, requiring a different number of button presses depending on the menu layout. This type of contextual navigation feels very weird in practice, especially when you have to press the up button twice to get to the bottom of a list. The solution is instead to use the shoulder buttons (which will jump six rows), or the trigger buttons (which will jump to the first and last row). These buttons work consistently throughout the application and avoid the strange side effects just mentioned.

## I don't like the default emulator for a certain system, how can I choose an alternative?

ES-DE comes preconfigured with support for many alternative emulators, see the [What's new in ES-DE v1.2](https://www.youtube.com/watch?v=rpnMJqceSNk) video on the ES-DE YouTube channel for a brief overview of how this works. More emulators are continuously being added with each release.

## I'm on Windows and ES-DE can't find my emulators, what is wrong?

On Windows ES-DE is shipped as a portable installation and as a regular installer. If you're using the portable installation you need to drop your emulators inside the Emulators directory. Make sure to read the README.txt file directly in the EmulationStation-DE folder for more details. For the regular installer many emulators do not provide a method to inform ES-DE where they are installed, so you will need to add their installation directories to the Path environment variable in Windows. It's strongly recommended to read the _Specific notes for Windows_ section of the [User guide](USERGUIDE.md#specific-notes-for-windows) before attempting to setup and use ES-DE on Windows.

## I'm on Windows and ES-DE refuses to start, is the application broken?

You're probably missing the OpenGL drivers required to run ES-DE. Try to download and install the latest drivers for your graphics card. If you have a really old GPU this may not work though, and you may have to go for OpenGL software rendering instead. How this is setup is described in the _Specific notes for Windows_ section of the [User guide](USERGUIDE.md#specific-notes-for-windows). There have also been a few reports of ES-DE refusing to start or displaying excessive graphics corruption when using Intel Iris Xe GPUs. This is seemingly caused by driver bugs and will have to be resolved by Intel. Running the same hardware on Linux seems to work fine.

## I'm on Windows and there is only a black screen shown on startup or when launching a game, is there a way to fix this?

This behavior has been observed for some specific AMD GPUs in the past. In some instances there is only a black screen on startup and in some instances the application starts and runs correctly but launching a game only shows a black screen. The issue is seemingly caused by GPU driver bugs and it only affects Windows as Linux works fine with the same hardware. The workaround is to make ES-DE run in windowed mode. You accomplish this by using the --resolution flag and setting the width to one pixel wider than your screen resolution. So if for instance running at a 1280x800 display resolution, run ES-DE such as this: `EmulationStation.exe --resolution 1281 800`

## The emulators don't seem to be properly configured?

ES-DE acts strictly as a frontend, it does not interfere with the emulator configuration. The only instance when ES-DE passes specific options to an emulator is when there is no choice, like when fullscreen mode can't be enabled except via a command line option, or when a flag is needed to place the emulator in batch mode to work properly with a frontend. So you need to configure your emulators separately. This includes your controllers as well since the controller configuration inside ES-DE will have no effect on the emulators.

## On game launch the screen just flashes black and the game won't start, how do I solve this?

This is almost always caused by either corrupt ROMs/disc images or by missing emulator BIOS files (which have to be installed separately from ES-DE).

## How do I exit back from the emulators to ES-DE?

This is related to the two questions above, ES-DE does not perform any emulator configuration or general system configuration. So you would either need to configure an exit button combination inside your emulator, or use a third party tool to map for instance Alt + F4 or Command + Q to a certain button combination on your controller. If you use RetroArch then it's easy to setup a button combination to exit back to ES-DE, which will apply to all cores. Refer to the RetroArch documentation or their support channels on how to accomplish this.

## Why does it take a long time for ES-DE to resume after I've exited a game?

With a few notable exceptions like the Valve Steam system on all platforms and the MAME standalone emulator on Windows, ES-DE will wait for emulators and games to fully exit before it resumes. Some emulators take quite some time to fully exit (sometimes even after their application windows have been destroyed). This may make it seem as if ES-DE is hanging although it's actually waiting for the launched child process to exit. Another reason for a delayed resume is if a custom event script is executed on game end which itself takes a long time to run. If you have such a custom setup then there is a way to work around this by executing scripts as background processes, which is documented [here](INSTALL.md#custom-event-scripts).

## I have many games with multiple files, is there a way to show these as single entries?

Yes this is supported for both single-disc games using a .bin/.cue structure for instance, or for multi-file/multi-disc games where .m3u files are used for emulator disc swapping. See the _Directories interpreted as files_ section of the [User guide](USERGUIDE.md#directories-interpreted-as-files) for details on how to configure this.

## I see both .bin and .cue files for many of my games, how can I hide the unnecessary files?

See the question above for a possible solution. Another approach would be to hide the game files you don't want to see using the metadata editor. Yet another solution which is actually recommended is to convert your games to the .chd format. This combines the .bin/.cue files into a single file that is also compressed, leading to space savings. Just be aware that you can't combine multi-disc games into a single .chd file. A custom systems configuration entry could also be created, but that is not really recommended or required due to the solutions just mentioned. The reason why .bin files are included in the first place is that some emulators can launch these files directly, and some users have game collections comprised of such files. If the .bin files were filtered out in the bundled configuration, then all these users would have their games removed from within ES-DE next time they upgrade the application.

## When I hide a game using the metadata editor it's not really getting hidden, is this a bug?

No, by default games are not removed from the gamelists when they are hidden and are instead only marked with a much lower opacity. You need to disable the setting _Show hidden games (requires restart)_ from the _Other settings_ menu to make them disappear entirely. The reason this option is not disabled by default is that new users could very easily make a mistake by hiding some files accidentally without realizing it, only to have the entries being immediately removed from the gamelist view. It's also good practice to hide all your games with this option enabled and verify that it's all correct before going ahead and disabling it.

## I'm using Linux or macOS and I can't find the .emulationstation directory, where is it located?

The .emulationstation directory is normally located in your home directory, but on these Unix-based operating systems files and directories starting with a dot are hidden by default. So you need to enable hidden files and directories in your file manager. On macOS this is done in Finder using the Shift + Command + . (a dot) keyboard combination. On Linux it depends on which file manager you're using, but in Dolphin it's accomplished by using the Alt + . (a dot) keyboard combination or via the corresponding entry in the hamburger menu.

## Is there a way to customize existing systems, and/or to add more systems than those shipped by default?

Yes it's possible to both customize existing systems that are part of the bundled configuration as well as to add more systems than those shipped with ES-DE. Almost nothing is hardcoded in the application so there is a huge flexibility when it comes to such configuration. How this is done is covered in the _Game system customizations_ section of the [User guide](USERGUIDE.md#game-system-customizations). Just make sure to never modify the es_systems.xml and es_find_rules.xml files included in the application installation directory as these will be overwritten when upgrading ES-DE in the future. Always place your customizations in ~/.emulationstation/custom_systems/ as is also described in the user guide.

## Can I use the Steam release of RetroArch?

This release of RetroArch has multiple technical issues so it's not officially supported and its use is not recommended. If you still insist on using it you can make custom configuration entries for your game systems which is pretty straightforward as it's partly prepared for in the bundled configuration. Refer to the _Using the Steam release of RetroArch_ section of the [User guide](USERGUIDE.md#using-the-steam-release-of-retroarch) for how to accomplish this. There you can also read about the technical issues you can expect to experience when using this release of RetroArch.

## How do I add more themes?

Refer to the official list of [recommended theme sets](https://gitlab.com/es-de/themes/themes-list) for a selection of high-quality themes. There are also some brief instructions there on how to download and install them. More comprehensive documentation is available in the _Themes_ section of the [User guide](USERGUIDE.md#themes). A built-in theme downloader is also planned for a future release.

## The themes I've added don't seem to work?

Most themes from Batocera, Recalbox and similar EmulationStation forks can't be used as ES-DE has a different theme engine than those applications. For the time being RetroPie themes can be used, but support for these legacy themes will be removed in a future version.

## I used to be a Batocera/Recalbox user and ES-DE can't seem to find some of my games?

ES-DE uses mostly the same system names as these other frontends, but there are some exceptions for historical reasons. For example Nintendo GameCube is called _gc_ ES-DE while being named _gamecube_ in Batocera and Recalbox. The same is true for some other systems like _n3ds_ vs. _3ds_ for Nintendo 3DS. See the _Supported game systems_ section at the bottom of the [User guide](USERGUIDE.md#supported-game-systems) where there's a table listing the system names that ES-DE expects.

## Can ES-DE update itself automatically when a new release becomes available?

ES-DE includes a check for new versions and will show a notification window on startup if there is a new release available for download. As a second step downloading of the new version will be implemented and on some platforms automatic updating will be added as well. It's however unlikely that automatic upgrading will be feasible for all package formats. The process to manually upgrade ES-DE is covered in the _Upgrading to a newer release_ section of the [User guide](USERGUIDE.md#upgrading-to-a-newer-release). If you find the update notification messages annoying you can change the frequency of update checks between _Always_, _Daily_, _Weekly_, _Monthly_ or _Never_ from the _Other settings_ menu.

## I can't find any game media links in the gamelist.xml files, where is this data stored?

ES-DE works very differently compared to all other EmulationStation forks when it comes to handling of game media. There are no links in the gamelist.xml files, instead media files are simply matched against the ROM/game file names which makes for a much simpler, faster and completely portable setup. Migrating game media from other EmulationStation forks (and potentially from other frontends as well) can be accomplished quite easily. See the next question below for more information. Make sure to also read the _Migrating from other EmulationStation forks_ section of the [User guide](USERGUIDE.md#migrating-from-other-emulationstation-forks) to avoid data loss if running ES-DE with existing data from another EmulationStation fork.

## It seems like gamelist.xml files in the ROMs directory tree are no longer getting loaded as of ES-DE 2.0?

Yes, to optionally read gamelist.xml files from the ROMs directory tree in previous releases was a mistake as it has caused a lot of confusion as well as invalid bug reports. As such the logic has now been changed to only read these files from .emulationstation/gamelists/ which is where they belong. If you insist on retaining the old logic you can do so by manually setting LegacyGamelistFileLocation to true in es_settings.xml as explained [here](INSTALL-DEV.md#settings-not-configurable-via-the-gui), but it's definitely not recommended.

## Why do I sometimes get error messages when scraping stating that files are less than 350 bytes in size?

This issue can occur occassionally as the ScreenScraper servers sometimes return invalid responses, in this case simply pressing the _RETRY_ button often works. But there is also a ScreenScraper bug where their cache could include entries that no longer exist. When a media file is removed from the ScreenScraper database, the cached link to that file is retained for some time and will be returned as a valid media file URL to ES-DE. However, when attempting to scrape such a file, it will only contain the text string _NOMEDIA_ which will trigger this error in ES-DE. The cache bug only affects the multi-scraper API call, so a workaround is to manually scrape such games using the single-game scraper (reachable via the metadata editor). The invalid cache entries seem to disappear within 24 hours so waiting for a while and rescraping should also resolve the problem. The issue has been reported to the ScreenScraper team but it's unclear if and when it will be resolved.

## Can I use an external scraper application instead of the built-in scraper?

Yes to a certain extent this is supported and at least [Skraper](https://www.skraper.net) and [Skyscraper](https://github.com/detain/skyscraper) have been used by some people. Make sure to read the _Manually copying game media files_ section of the [User guide](USERGUIDE.md#manually-copying-game-media-files) for more details of where ES-DE expects game media to be stored.

## My controller isn't working in ES-DE, is there a way to fix this?

If the controller works in other applications and games but not in ES-DE, then you may be able to get it to run in ES-DE as well. The required setup is described in detail in the _Adding custom controller profiles_ section of the [Building and advanced configuration](INSTALL.md#adding-custom-controller-profiles) document. Although not guaranteed to work, an alternative solution is to have Steam running in parallel to ES-DE and using the _Steam Controller_ functionality to present the controller as a virtual device in ES-DE. You don't need to launch ES-DE via Steam for this to work, it's enough if Steam is running in the background. If going for this, make sure to read the next question below, and also note that the Steam approach only seems to work on Linux and Windows, and not on macOS.

## Why is every controller button press registered twice in ES-DE?

There are two reasons why double input is received in ES-DE, either because Steam is running and the _Desktop Configuration_ of the Steam Controller functionality is enabled, or due to buggy controller drivers where two devices are registered in parallel by the operating system. In the former case, enabling the Steam Controller functionality will by default also enable keyboard input to be sent whenever a button is pressed on the controller. As ES-DE reads both the keyboard and controller events this will be registered as double or conflicting input. To disable this functionality, go into the Steam settings interface, then select the _Controller_ entry followed by the _DESKTOP CONFIGURATION_ button. Make sure to remove all keyboard mappings and the problem should disappear. An alternative solution is to add ES-DE as a Non-Steam game and launch it via the Steam application in which case the keyboard events will be automatically disabled (that's how it's normally done on the Steam Deck for instance). Making sure Steam is shut down while ES-DE is running is another possible solution.

The second reason for double input is buggy controller drivers. This seems to only occur with wireless controllers but it's possible that it also happens with wired devices. As ES-DE auto-configures all devices, every button press will in practice be received twice. The easiest solution to this problem is to enable the option _Only accept input from first controller_ in the _Input device settings_ menu, but the drawback of this is that all other attached controllers will also be ignored. A more proper workaround is to blacklist the redundant controller device, see the previous question above as blacklisting is essentially a custom controller profile entry. Note that neither of these approaches will affect the emulators/games launched from ES-DE, it only applies to the ES-DE application itself.

## I'm missing a feature, how can I make a request to have it added?

First check the project [Kanban](https://gitlab.com/es-de/emulationstation-de/-/boards/1823720) board which contains an overview of planned future features and changes and search for the functionality you would like to see added. Chances are there is already a card on the board describing precisely what you intended to request. You can also check the [Release roadmap](ROADMAP.md) which includes the planned implementation of major features. If you can't find the feature you're looking for, you can request it either via adding an issue directly to the Kanban board, or by asking for it in our [Discord](https://discord.gg/EVVX4DqWAP) server or [subreddit](https://www.reddit.com/r/EmulationStation_DE/).

## I want to setup a gaming appliance based on the Raspberry Pi, can I use ES-DE for this?

This is not the goal of ES-DE, the application requires a desktop environment to run. If you want to run a frontend on a single-board computer or similar then there are better options out there. But many people run ES-DE on arcade cabinets and similar which is definitely possible, it just requires a desktop-class operating system.
