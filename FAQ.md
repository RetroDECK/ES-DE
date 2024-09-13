# ES-DE Frontend - Frequently Asked Questions

## What's the correct name for the application? ES-DE, EmulationStation etc?

As of the 3.0.0 release the official name for the project and application is ES-DE Frontend or simply ES-DE. This originally stood for EmulationStation Desktop Edition as the project originated from the RetroPie fork of EmulationStation in 2020, and was originally intended for desktop computers. However the application has now changed so much that it would just cause confusion to keep the EmulationStation name. During a transition period EmulationStation Desktop Edition will be included as a subtitle on the splash screen and mentioned in some documentation entries, but long term this will be dropped entirely. The red version of the EmulationStation logo will however remain as it's part of the application legacy.

## Is this software available for free, and is it open source?

ES-DE is available for free on Windows, macOS and Linux and it's released under the MIT open source license. For Android it's a paid app available via [Patreon](https://www.patreon.com/es_de), the [Samsung Galaxy Store](https://galaxystore.samsung.com/detail/org.es_de.frontend.galaxy) and [Huawei AppGallery](https://appgallery.huawei.com/#/app/C111315115) and this port is partially closed source.

## Which operating systems are supported?

ES-DE is officially supported on Android, Windows, macOS and multiple Linux distributions including Ubuntu, Fedora, Arch, Manjaro, SteamOS etc. It's also semiofficially supported on some secondary platforms like FreeBSD, the Raspberry Pi and Haiku although these ports may not always be up to date and you may have to build ES-DE yourself.

## What is the relationship between ES-DE and RetroDECK?

ES-DE and [RetroDECK](http://retrodeck.net) are completely separate projects, but we collaborate to give the best possible user experience. RetroDECK bundles ES-DE with all emulators in the same Flatpak so you don't need to update emulators separately or set Flatpak permissions manually. It's a good idea to read the _Specific notes for Steam Deck_ section of the [User guide](USERGUIDE.md#specific-notes-for-steam-deck) if ES-DE has been installed via RetroDECK. Note however that RetroDECK can also be used on regular Linux desktops and not only on the Steam Deck.

## What is the relationship between ES-DE and EmuDeck?

EmuDeck is an installer that installs ES-DE and a number of emulators. There is no relationship between the two projects apart from this, and it's not recommended to use EmuDeck as this will lead to a non-standard installation and can cause a lot of other issues as well. Instead it's recommended to setup your emulators manually which will allow you to configure everything exactly to your liking.

## What game systems/platforms and emulators are supported by ES-DE?

See the _Supported game systems_ section at the bottom of the [User guide](USERGUIDE.md#supported-game-systems) where there's a table listing all supported systems/platforms and emulators.

## Why can't I press the up button in menus to jump to the bottom row and vice versa?

Menus in ES-DE are not lists but grids, sometimes there is only a list but sometimes there are buttons beneath the list. Enabling the up and down buttons to wrap around would therefore not work consistently as it would sometimes jump to the last row of the list and sometimes to a button, requiring a different number of button presses depending on the menu layout. This type of contextual navigation feels very weird in practice, especially when you have to press the up button twice to get to the bottom of a list. The solution is instead to use the shoulder buttons (which will jump six rows), or the trigger buttons (which will jump to the first and last row). These buttons work consistently throughout the application and avoid the strange side effects just mentioned.

## I'm using SteamOS and my language settings are not reflected in ES-DE, how can I get language auto-detection to work?

SteamOS does not setup the environment correctly in game mode so it's not possible for ES-DE to detect your configured language. As such you need to manually select your language inside ES-DE using the _Application Language_ option in the _UI Settings_ menu.

## Can I change the system sorting to not sort by full system names?

Yes the systems sorting configuration file can be selected via the _Systems sorting_ option in the _UI Settings_ menu. There are four such files bundled with ES-DE to sort by _"Release year", "Manufacturer, release year", "Hardware type, release year"_ and _"Manufacturer, hardware type, release year"_. If you don't want to use any of the bundled files then you can create your own custom sorting file and place it into the ~/ES-DE/custom_systems/ directory. More details about this setup can be found in the _es_systems_sorting.xml_ section of the [Building and advanced configuration](INSTALL.md#es_systems_sortingxml) document.

There is also a handy custom sorting generator available at this location to help you create the XML file:\
https://docs.google.com/spreadsheets/d/1aCmCpE08ooFd_Ea0soEjYEyPPpy2pvly7-ZW6xykhFQ/edit#gid=511646442

## I'm missing some systems like SNES MSU-1 and WiiWare, could those get added to ES-DE?

To keep the system count within reason hack systems and e-shop systems are not included and there are no plans to add them in. This would include things like _PSP Minis, PlayStation Store, WiiWare, SNES MSU-1, Sega Mega Drive MSU-MD, ROM Hacks_ and so on. It's possible to add such games to the regular systems though, for example by placing them inside their own folders if you want to clearly separate them from the rest of the games. You could also create custom collections for these games.

## I don't like the default emulator for a certain system, how can I choose an alternative?

ES-DE comes preconfigured with support for many alternative emulators, see the [What's new in ES-DE v1.2](https://www.youtube.com/watch?v=rpnMJqceSNk) video on the ES-DE YouTube channel for a brief overview of how this works. More emulators are continuously being added with each release.

## I'm on Windows and ES-DE can't find my emulators, what is wrong?

On Windows ES-DE is shipped as a portable installation and as a regular installer. If you're using the portable installation you need to drop your emulators inside the Emulators directory. Make sure to read the README.txt file directly in the ES-DE folder for more details. For the regular installer many emulators do not provide a method to inform ES-DE where they are installed, so you will need to add their installation directories to the Path environment variable in Windows. It's strongly recommended to read the _Specific notes for Windows_ section of the [User guide](USERGUIDE.md#specific-notes-for-windows) before attempting to setup and use ES-DE on Windows.

## I'm on Windows and ES-DE refuses to start, is the application broken?

You're probably missing the OpenGL drivers required to run ES-DE. Try to download and install the latest drivers for your graphics card. If you have a really old GPU this may not work though, and you may have to go for OpenGL software rendering instead. How this is setup is described in the _Specific notes for Windows_ section of the [User guide](USERGUIDE.md#specific-notes-for-windows). There have also been a few reports of ES-DE refusing to start or displaying excessive graphics corruption when using Intel Iris Xe GPUs. This is seemingly caused by driver bugs and will have to be resolved by Intel. Running the same hardware on Linux seems to work fine.

## I'm on Windows and there is only a black screen shown on startup or when launching a game, is there a way to fix this?

This behavior has been observed for some specific AMD GPUs in the past. In some instances there is only a black screen on startup and in some instances the application starts and runs correctly but launching a game only shows a black screen. The issue is seemingly caused by GPU driver bugs and it only affects Windows as Linux works fine with the same hardware. The workaround is to make ES-DE run in windowed mode. You accomplish this by using the --resolution flag and setting the width to one pixel wider than your screen resolution. So if for instance running at a 1280x800 display resolution, run ES-DE such as this: `ES-DE.exe --resolution 1281 800`

## The emulators don't seem to be properly configured?

ES-DE acts strictly as a frontend, it does not interfere with the emulator configuration. The only instance when ES-DE passes specific options to an emulator is when there is no choice, like when fullscreen mode can't be enabled except via a command line option, or when a flag is needed to place the emulator in batch mode to work properly with a frontend. So you need to configure your emulators separately. This includes your controllers as well since the controller configuration inside ES-DE will have no effect on the emulators.

## On game launch the screen just flashes black and the game won't start, how do I solve this?

This is almost always caused by either corrupt ROMs/disc images or by missing emulator BIOS files (which have to be installed separately from ES-DE).

## How do I exit back from the emulators to ES-DE?

This is related to the two questions above, ES-DE does not perform any emulator configuration or general system configuration. So you would either need to configure an exit button combination inside your emulator, or use a third party tool to map for instance Alt + F4 or Command + Q to a certain button combination on your controller. If you use RetroArch then it's easy to setup a button combination to exit back to ES-DE, which will apply to all cores. Refer to the RetroArch documentation or their support channels on how to accomplish this.

## Why does it take a long time for ES-DE to resume after I've exited a game?

With a few notable exceptions like the Valve Steam system, ES-DE will wait for emulators and games to fully exit before it resumes. Some emulators take quite some time to fully exit (sometimes even after their application windows have been destroyed). This may make it seem as if ES-DE is hanging although it's actually waiting for the launched child process to exit. Another reason for a delayed resume is if a custom event script is executed on game end which itself takes a long time to run. If you have such a custom setup then there is a way to work around this by executing scripts as background processes. How this is accomplished is documented in the _Custom event scripts_ section of the [Building and advanced configuration](INSTALL.md#custom-event-scripts) document.

## I have many games with multiple files, is there a way to show these as single entries?

Yes this is supported for both single-disc games using a .bin/.cue structure for instance, or for multi-file/multi-disc games where .m3u files are used for emulator disc swapping. See the _Directories interpreted as files_ section of the [User guide](USERGUIDE.md#directories-interpreted-as-files) for details on how to configure this.

## I see both .bin and .cue files for many of my games, how can I hide the unnecessary files?

See the question above for a possible solution. Another approach would be to hide the game files you don't want to see using the metadata editor. Yet another solution which is actually recommended is to convert your games to the .chd format. This combines the .bin/.cue files into a single file that is also compressed, leading to space savings. Just be aware that you can't combine multi-disc games into a single .chd file. A custom systems configuration entry could also be created, but that is not really recommended or required due to the solutions just mentioned. The reason why .bin files are included in the first place is that some emulators can launch these files directly, and some users have game collections comprised of such files. If the .bin files were filtered out in the bundled configuration, then all these users would have their games removed from within ES-DE next time they upgrade the application.

## When I hide a game using the metadata editor it's not really getting hidden, is this a bug?

No, by default games are not removed from the gamelists when they are hidden and are instead only marked with a much lower opacity. You need to disable the setting _Show hidden games_ from the _Other Settings_ menu to make them disappear entirely. The reason this option is not disabled by default is that new users could very easily make a mistake by hiding some files accidentally without realizing it, only to have the entries being immediately removed from the gamelist view. It's also good practice to hide all your games with this option enabled and verify that it's all correct before going ahead and disabling it.

## I can't find a ROM directory setting in the user interface, so how do I relocate my games?

There is no need for such a setting as ES-DE will not start unless it finds at least one game. So you simply need to move your ROM directory to its new location using your operating system's file manager or terminal and then the next time you start ES-DE you will be shown a dialog where you can enter the new directory. Optionally you can manually change the ROMDirectory setting in ~/ES-DE/es_settings.xml

## What are miximages precisely and why don't they get updated when I change my miximage settings?

As the name implies these are images, and more specifically they are generated by ES-DE from scraped or manually downloaded media. Depending on what is available for a certain game the miximage is built from a combination of a screenshot, marquee, 3D box (or box cover as fallback) and physical media. If no screenshot is available then no miximage will be created, all other image types are optional. By default miximages are generated when scraping, but it's also possible to update them using the built-in offline generator. This tool can be found at the bottom of the _Miximage Settings_ menu. So after changing any miximage settings, either rescrape or run the offline generator and your miximages will be updated accordingly. More details about this topic can be found in the _Miximage Settings_ section of the [User guide](https://gitlab.com/es-de/emulationstation-de/-/blob/master/USERGUIDE.md#miximage-settings).

## Is there a way to customize existing systems, and/or to add more systems than those shipped by default?

Yes it's possible to both customize existing systems that are part of the bundled configuration as well as to add more systems than those shipped with ES-DE. Almost nothing is hardcoded in the application so there is a huge flexibility when it comes to such configuration. How this is done is covered in the _Game system customizations_ section of the [User guide](USERGUIDE.md#game-system-customizations). Just make sure to never modify the es_systems.xml and es_find_rules.xml files included in the application installation directory as these will be overwritten when upgrading ES-DE in the future. Always place your customizations in ~/ES-DE/custom_systems/ as is also described in the user guide.

## Can I use the Steam release of RetroArch?

This release of RetroArch has multiple technical issues so it's not officially supported and its use is not recommended. If you still insist on using it you can make custom configuration entries for your game systems which is pretty straightforward as it's partly prepared for in the bundled configuration. Refer to the _Using the Steam release of RetroArch_ section of the [User guide](USERGUIDE.md#using-the-steam-release-of-retroarch) for how to accomplish this. There you can also read about the technical issues you can expect to experience when using this release of RetroArch.

## How do I add more themes?

You would normally use the built-in theme downloader to install additional themes. This utility can be found in the _UI Settings_ menu. There is also a [web version](https://gitlab.com/es-de/themes/themes-list) of the themes list which contains a number of additional themes not available via the downloader interface. Themes can also be updated via the downloader which is a recommended activity to perform every now and then, especially after upgrading to a newer ES-DE release as there may have been new systems added.

## I added some EmulationStation themes manually but they don't seem to show up inside ES-DE?

EmulationStation themes are not supported by ES-DE. If you want to use a theme from Batocera, Recalbox, RetroBat, RetroPie etc. then it first needs to be ported to the ES-DE theme engine. If you place a non-supported theme in the ES-DE/themes/ directory then this will be ignored on startup, meaning it will not be selectable from the _UI Settings_ menu.

## I used to be a Batocera/Recalbox user and ES-DE can't seem to find some of my games?

ES-DE uses mostly the same system names as these other frontends, but there are some exceptions for historical reasons. For example Nintendo GameCube is called _gc_ ES-DE while being named _gamecube_ in Batocera and Recalbox. The same is true for some other systems like _n3ds_ vs. _3ds_ for Nintendo 3DS. See the _Supported game systems_ section at the bottom of the [User guide](USERGUIDE.md#supported-game-systems) where there's a table listing the system names that ES-DE expects.

## Can ES-DE update itself automatically when a new release becomes available?

There is a built-in application updater that works with the Linux AppImage releases. And if using the AUR release updates are handled via the operating system's package manager. Likewise if using RetroDECK, then ES-DE is updated as part of the overall RetroDECK Flatpak. For Windows and macOS the application updater will download the latest version (as of ES-DE 2.2.0) but you need to manually perform the upgrade. The process to upgrade ES-DE is covered in the _Upgrading to a newer release_ section of the [User guide](USERGUIDE.md#upgrading-to-a-newer-release). If you find the update notification messages annoying you can change the frequency of update checks between _Always_, _Daily_, _Weekly_, _Monthly_ or _Never_ from the _Other Settings_ menu.

## I can't find any game media links in the gamelist.xml files, where is this data stored?

ES-DE works differently compared to EmulationStation when it comes to handling of game media. There are no links in the gamelist.xml files, instead media files are matched against the ROM/game file names which makes for a much simpler, faster and completely portable setup. Migrating game media from EmulationStation (and potentially from other frontends as well) can be accomplished quite easily. Make sure to also read the _Migrating from EmulationStation_ section of the [User guide](USERGUIDE.md#migrating-from-emulationstation) to avoid data loss if running ES-DE with existing data from EmulationStation.

## It seems like gamelist.xml files in the ROM directory tree are not getting loaded?

These files are not loaded by default, only files placed in ES-DE/gamelists/ are processed. If you insist on also looking for gamelist.xml files in the ROM directory tree then you can enable the LegacyGamelistFileLocation setting in es_settings.xml as explained in the _Settings not configurable via the GUI_ section of the [Building and advanced configuration](INSTALL.md#settings-not-configurable-via-the-gui) document.

## Can I use an external scraper application instead of the built-in scraper?

Yes you could for example use [Skyscraper](https://github.com/Gemba/skyscraper), [Skraper](https://www.skraper.net) or [ARRM](http://www.jujuvincebros.fr/wiki/arrm/doku.php?id=arrm_relooked_en:start-en).

There is a video on the official ES-DE YouTube channel on how to use Skraper with ES-DE:\
https://www.youtube.com/watch?v=23LLH96P7PE

It's also recommended to read the _Manually copying game media files_ section of the [User guide](USERGUIDE.md#manually-copying-game-media-files) for more details on where ES-DE expects game media to be stored.

## My controller isn't working in ES-DE, is there a way to fix this?

If the controller works in other applications and games but not in ES-DE, then you may be able to get it to run in ES-DE as well. The required setup is described in detail in the _Adding custom controller profiles_ section of the [Building and advanced configuration](INSTALL.md#adding-custom-controller-profiles) document. If this procedure does not work, then support for your controller is probably missing in the SDL library that ES-DE uses for low-level input management. In this case you can request that it gets added via the SDL [issue tracker](https://github.com/libsdl-org/SDL/issues), which will ensure that it will work in future ES-DE releases. Although not guaranteed to work, an alternative solution is to have Steam running in parallel to ES-DE and using the _Steam Controller_ functionality to present the controller as a virtual device in ES-DE. You don't need to launch ES-DE via Steam for this to work, it's enough if Steam is running in the background. If going for this, make sure to read the next question below, and also note that the Steam approach only seems to work on Linux and Windows, and not on macOS.

## Why is every controller button press registered twice in ES-DE?

There are two main reasons why double input is received in ES-DE, either because Steam is running and Steam Input with Desktop Layout mappings is enabled, or due to buggy controller drivers where two devices are registered in parallel by the operating system. In the former case, enabling the Steam Input functionality will by default also enable keyboard input to be sent whenever a button is pressed on the controller. As ES-DE by default reads both the keyboard and controller events this will be registered as double or conflicting input. To disable this functionality, go into the Steam _Settings_ interface, then select the _Controller_ tab followed by the _Edit_ button for the _Desktop Layout_ option. Make sure to remove all keyboard mappings and the problem should disappear. An alternative solution is to add ES-DE as a Non-Steam game and launch it via the Steam application in which case the keyboard events will be automatically disabled (that's how it's normally done on the Steam Deck for instance). Making sure Steam is shut down while ES-DE is running is another possible solution. Yet another solution is to enable the _Ignore keyboard input_ option in the _Input Device Settings_ menu, although this is generally not recommended.

The second reason for double input is buggy controller drivers. This seems to only occur with wireless controllers but it's possible that it also happens with wired devices. As ES-DE auto-configures all devices, every button press will in practice be received twice. The easiest solution to this problem is to enable the option _Only accept input from first controller_ in the _Input device settings_ menu, but the drawback of this is that all other attached controllers will also be ignored. A more proper workaround is to blacklist the redundant controller device, see the previous question above as blacklisting is essentially a custom controller profile entry. Note that neither of these approaches will affect the emulators/games launched from ES-DE, it only applies to the ES-DE application itself.

## I'm missing a feature, how can I make a request to have it added?

First check the project [Kanban](https://gitlab.com/es-de/emulationstation-de/-/boards/1823720) board which contains an overview of planned future features and search for the functionality you would like to see added. Chances are there is already a card on the board describing what you intended to request. You can also check the [Roadmap](ROADMAP.md) document which contains a list of planned major features. If you can't find the feature you're looking for, then you can request it either via adding an issue directly to the Kanban board, or by asking for it in our [Discord](https://discord.gg/42jqqNcHf9) server.

## I want to setup a gaming appliance based on a single-board computer, can I use ES-DE for this?

Yes this is possible, although this is not the primary use case for ES-DE. As the application is not intended to be the sole user interface for the computer there is no built-in functionality to manage operating system settings and emulator settings for instance. ES-DE also generally requires a desktop environment to run, which means a window manager and a sound server. You can however use KMS/direct framebuffer access on Linux if you build with the DEINIT_ON_LAUNCH flag as explained in the _Building on Unix_ section of the [INSTALL.md](INSTALL.md#building-on-unix) document. This would in theory allow ES-DE to be a drop-in replacement for RetroPie EmulationStation for example.
