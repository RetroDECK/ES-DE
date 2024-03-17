# ES-DE (EmulationStation Desktop Edition) 3.0 (development version) - Android documentation

This document contains information specific to the Android release, for more general ES-DE documentation refer to the [User guide](USERGUIDE.md) as well as the general [FAQ](FAQ.md).

It's also generally recommended to read the [Frequently Asked Questions (FAQ) for Android](FAQ-ANDROID.md) document prior to diving into the information in this document.

You can buy the Android APK from our Patreon page: \
https://www.patreon.com/es_de

Table of contents:

[[_TOC_]]

## First startup and onboarding

When you first start ES-DE you will be greeted by a welcome screen, this is part of the _configurator_, the ES-DE onboarding interface. The configurator is easy to use and will guide you through the necessary setup steps.

As a first step you need to give ES-DE the required storage access permission or it will not be able to function. Just enable the setting and the configurator will proceed to the next step. Next you will need to define a application data directory where your settings, scraped media, custom collections and so on will be stored. By default this will be placed in the _ES-DE_ directory in the root of your device's internal storage, and this directory will be created for you automatically.

After this step you need to select a ROMs directory where your game files will be stored, by default this will be named _ROMs_ and will be located in the root of you device's internal storage. You can however choose to place this on an SD card if you want to, just change the path using the Android file selector GUI. If you do change the path to the SD card you will however need to manually create the ROMs directory as well as to delete the empty directory that was created for you in the built-in storage.

The next step is optional, and it's whether to create the game systems directory structure inside your ROMs folder. Performing this will also create _systeminfo.txt_ files in each system directory. These files contain information about the system such as what file extensions and emulators that are supported. They are not mandatory for the app to function, they are only there for your convenience. In general it's recommended to create the system directories, although you could remove the ones you don't need afterwards for a slightly faster app startup speed.

This is basically the onboarding process, and ES-DE should now start up. Just be aware that you need to place at least one game with a supported file extension in the ROMs directory tree or ES-DE will only show an information dialog about missing games.

Also note that ES-DE does not install any emulators, you need to install those separately. There is more information about that topic later in this document.

If you need to re-run the configurator for some reason then the easiest way is to go into the Android Apps setting screen and revoke the storage access permissions under _Special app access_. This will make the configurator run automatically next time you start ES-DE. Another way to force it to start is to clear the app's storage under _Storage & cache_ but this is normally not recommended as it also deletes all themes you have downloaded using the theme downloader. A third option would be to rename either the ES-DE or ROMs directory as this will also trigger the configurator on next app startup.

## Touch input overlay

By default the touch input overlay will be enabled which makes it possible to use ES-DE without a controller or physical keyboard by overlaying virtual buttons on top of the ES-DE interface. If you are using a device which has a built-in controller you may however want to disable this feature. That is done via the _Enable touch overlay_ option in the _Input device settings_ menu on the main menu. Just be aware that disabling this option on a device where you have no other input method than touch will lock you out of the application.

If you accidentally disable the touch overlay you can force the configurator to run as explained in the previous section above, this will always reset the touch overlay setting. Another option would of course be to temporarily plug in a controller or keyboard to enable the setting via its menu entry. A third option would be to manually edit the es_settings.xml file in the ES-DE application data directory. The setting you are after is named _InputTouchOverlay_ which should be changed from _false_ to _true_.

Apart from this there are numerous options for the touch overlay, like the ability to change its size, opacity and fade-out time. Setting the fade-out to zero will make it permanently visible. See the [User guide](USERGUIDE-DEV.md) for a complete reference of all app settings and features.

## Retention of files and data

Almost all files saved and used by ES-DE are kept in the shared storage on either the device's built-in storage or on the SD card. This means that uninstalling the ES-DE app will not remove any of that data. The only thing that will be deleted are themes that have been downloaded using the built-in theme downloader, as it's not possible to store these in the ES-DE application data directory for technical reasons.

## Emulation on Android in general

There are a few challenges with emulation on Android. Some emulators on the Google Play store have not been updated for a long time, and some emulators are not available on the Play store at all. For these reasons you will need to sideload some manually downloaded APKs for a good emulation setup. There is a section later in this document describing the best place to get hold of each supported emulator.

Thankfully sideloading emulators is easy to do, the exact producedure for how to install APKs manually is not covered here but there are many resources available online on how to accomplish this.

There is also the [F-Droid](https://f-droid.org/) app store as an alternative to Google Play, and this service contains a couple of emulators that are not present on the Play store, or that are present there but haven't been updated for a very long time.

A number of emulators support the [FileProvider](https://developer.android.com/reference/androidx/core/content/FileProvider) API which makes it possible for ES-DE to temporarily provide storage access to the game file on launch. This means that most of the time no access permission needs to be setup in the emulator upfront. Access can however only be passed for single files, so for systems that support multi-file games such as disc-based games in .bin/.cue format SAF URIs are often used instead. For those emulators you will therefore generally need to manually provide scoped storage access to each game system directory. Note that it's not supported to give access to the root of the entire ROM directory for emulators that use scoped storage, it has to be for the specific system. For instance `/storage/emulated/0/ROMs/n64` rather than `/storage/emulated/0/ROMs`.

Adding to the FileProvider API confusion is the fact that some emulators will only launch games when using this API even though they need to have scoped storage access setup within the emulator upfront. FPseNG and FPse are two examples of this. So there is unfortunately no definitive rule regarding the use of the FileProvider API, it all depends on how the emulator has implemented the functionality.

Some emulators like RetroArch are still using an older storage access method and for those emulators this is not something you need to consider.

The following emulators are configured for FileProvider access:
* 2600.emu
* FPseNG (still needs scoped storage to be setup in emulator)
* FPse (still needs scoped storage to be setup in emulator)
* GBA.emu
* GBC.emu
* Lynx.emu
* MD.emu (genesis, mastersystem, megadrive, megadrivejp)
* MAME4droid 2024
* MAME4droid
* NES.emu
* NGP.emu
* PCE.emu (pcengine, supergrafx and tg16 systems)
* Ruffle
* Swan.emu

Some of these emulators require BIOS files, so they still need to be configured before they can be used with ES-DE.

## Splitting system directories across multiple storage devices

While it's possible to split the game system directories across multiple storage devices this is not recommended. First it's tedious to setup, but more importantly it breaks portability for the installation. For instance you can't easily migrate between the different operating systems that ES-DE support and your custom collections will not be portable at all, as they will instead contain absolute paths to your games.

The way ES-DE works is that you have a defined ROMs directory which corresponds to the %ROMPATH% variable that is used throughout the es_systems.xml file and the custom collections files. For example this is the system configuration for the samcoupe system:
```xml
<system>
    <name>samcoupe</name>
    <fullname>MGT SAM Coupé</fullname>
    <path>%ROMPATH%/samcoupe</path>
    <extension>.dsk .DSK .mgt .MGT .sad .SAD .sbt .SBT .7z .7Z .zip .ZIP</extension>
    <command label="Speccy (Standalone)">%EMULATOR_SPECCY% %ACTION%=android.intent.action.VIEW %DATA%=%ROMSAF%</command>
    <platform>samcoupe</platform>
    <theme>samcoupe</theme>
</system>
```

Here the path tag is using the %ROMPATH% variable to keep it relative to the base ROMs directory as selected via the onboarding configurator when you first installed ES-DE. If you relocate your ROMs directory to a different storage device, or copy it to another device altogether or if you synchronize your games across Android and Linux, macOS or Windows then everything will still work correctly.

Similarly custom collection files contain the %ROMPATH% variable too, such as this:
```
%ROMPATH%/amiga/OoopsUp.lha
%ROMPATH%/amiga/PacMania.lha
%ROMPATH%/samcoupe/Manic Miner.zip
%ROMPATH%/samcoupe/Prince of Persia.zip
```

This makes your custom collections portable if you move your ROMs directory and you can also transfer the collections between various devices and operating systems while keeping everything working seamlessly.

If you still insist on relocating some game system directories to another storage device then you need to make custom system configuration entries for them. See the _Game system customization_ section of the [User guide](USERGUIDE.md#game-system-customizations) for details on how this is accomplished. In short you need to create an es_systems.xml file in the ES-DE/custom_systems directory and replace the %ROMPATH% variable with an absolute path for the specific systems you want to relocate.

You can find the bundled es_systems.xml file for Android here (which contains configuration for all supported systems):\
https://gitlab.com/es-de/emulationstation-de/-/tree/stable-3.0/resources/systems/android

Here's an example of a custom es_systems.xml file that relocates the samcoupe system:

```xml
<?xml version="1.0"?>
<systemList>
    <system>
        <name>samcoupe</name>
        <fullname>MGT SAM Coupé</fullname>
        <path>/storage/719F-3A7F/ROMs/samcoupe</path>
        <extension>.dsk .DSK .mgt .MGT .sad .SAD .sbt .SBT .7z .7Z .zip .ZIP</extension>
        <command label="Speccy (Standalone)">%EMULATOR_SPECCY% %ACTION%=android.intent.action.VIEW %DATA%=%ROMSAF%</command>
        <platform>samcoupe</platform>
        <theme>samcoupe</theme>
    </system>
</systemList>
```

This example points the samcoupe directory to the external storage device /storage/719F-3A7F which may for instance be an SD card.

Note that doing the opposite, i.e. placing your primary ROMs directory on external storage and relocating a specific system to internal storage requires you to use the /storage/emulated/0 path, you can't use /sdcard in the path tag.

Here's again an example for the samcoupe system:

```xml
<?xml version="1.0"?>
<systemList>
    <system>
        <name>samcoupe</name>
        <fullname>MGT SAM Coupé</fullname>
        <path>/storage/emulated/0/ROMs/samcoupe</path>
        <extension>.dsk .DSK .mgt .MGT .sad .SAD .sbt .SBT .7z .7Z .zip .ZIP</extension>
        <command label="Speccy (Standalone)">%EMULATOR_SPECCY% %ACTION%=android.intent.action.VIEW %DATA%=%ROMSAF%</command>
        <platform>samcoupe</platform>
        <theme>samcoupe</theme>
    </system>
</systemList>
```

If going for this configuration, adding samcoupe games to a custom collection would end up with something like the following:

```
%ROMPATH%/amiga/OoopsUp.lha
%ROMPATH%/amiga/PacMania.lha
/storage/emulated/0/ROMs/samcoupe/Manic Miner.zip
/storage/emulated/0/ROMs/samcoupe/Prince of Persia.zip
```

This is obviously a non-portable collection.

You can relocate as many systems as you want, you just need to place them all within the systemList tag pairs in ES-DE/custom_systems/es_systems.xml.

## Issues with the Ayn Odin 2

There are two serious issues that seem to be specific to the Ayn Odin 2, although it remains to be seen whether the problem exists also on other devices.

The first problem is that some emulators refuse to run games that you place inside directories that contain dots in their names. This is quite problematic as the [directories interpreted as files](USERGUIDE.md#directories-interpreted-as-files) functionality depends on the ability to add file extensions to directory names.

This has been observed with M64Plus FZ, Play!, Saturn.emu, FPse and FPseNG and it's working fine with RetroArch, NetherSX2, ePSXe and DuckStation. Note however that this is not a complete list as not all emulators have been tested for this problem.

If you run into this issue you can use the _folder link_ functionality as an alternative to the _directories interpreted as files_ functionality. How to use folder links is described in the [User guide](USERGUIDE.md).

The second problem is that a number of emulators can't be launched from ES-DE at all. When attempting to run such an emulator an error popup with the game name followed by "ERROR CODE -1" is displayed. The affected emulators are ColEm, fMSX, iNES, MasterGear, My Boy!, My OldBoy!, Redream and Speccy.

The following devices have been tested and do **not** experience either of these two problems:

* Ayn Odin Lite (Android 11)
* Retroid Pocket 4 Pro (Android 13)
* Google Pixel 4a (Android 13)
* Google Pixel Tablet (Android 14)

There are also some issues with sound quality on the Odin 2, such as large fluctuations in volume where some sounds are quite loud and some are quite silent. There are also some strange aliasing effects when playing samples rapidly.

## Known ES-DE problems

In addition to the issues specific to the Ayn Odin 2 there are a couple of other problems that will hopefully be resolved in the near future:

* Poor performance/low frame rate after startup on some devices, which seems to happen randomly and is usually resolved by itself within 10 to 30 seconds.
* The Android soft keyboard causes rendering issues when navigating using a controller or physical keyboard, as such the ES-DE built-in keyboard is enabled by default for the time being. For testing purposes the Android soft keyboard can be enabled via the _Enable virtual keyboard_ option in the _UI settings_ menu. If only using touch input the issue is not present. This problem is believed to be caused by a bug in the SDL library so it probably needs to be resolved there.
* Using a mounted USB storage device for the ES-DE and/or ROMs directories will lead to the configurator exiting after finishing the setup instead of launching ES-DE. Restarting ES-DE manually will lead to a successful startup. If the option to create the system directories was selected in the configurator then this will have to be executed again from inside ES-DE. Note that using a mounted USB storage device leads to a very crippled setup anyway, as for example RetroArch can't read any games from such devices (i.e. from the /mnt/media_rw/ directory tree). Only emulators supporting scoped storage will be usable in such a setup.

## Emulator installation and setup

Below are specific instructions and considerations for all supported emulators.

### RetroArch

The RetroArch release from the Google Play store is problematic. It does not contain all emulator cores and a number of people have reported issues launching games from ES-DE (apparently it doesn't work at all on some devices). For these reasons it's strongly recommended to use the 64-bit release from the RetroArch website instead, or to install it from the Amazon Appstore or the F-Droid store.

https://retroarch.com \
https://www.amazon.com/dp/B09753XRVF \
https://f-droid.org/en/packages/com.retroarch

Be aware that you need to manually install every core you want to use from inside the RetroArch user interface, and you also need to install all necessary BIOS files. The Android release of RetroArch is pretty unforgiving and will usually just present a black screen on game launch if the core file or the BIOS file is missing, and it will hang there until Android realizes the app is not responding and displays a popup where you can choose to kill the process.

### AetherSX2 / NetherSX2

Although the emulator entry is named AetherSX2 the recommended release of this emulator is actually the NetherSX2 patched version as the AetherSX2 release on the Google Play store doesn't work correctly and probably can't be used with ES-DE at all. You'll need to search for this APK online, the filename you'll want is `15210-v1.5-4248-noads.apk`

If you prefer to apply the NetherSX2 patch yourself (i.e. build the APK) then you can find all relevant information here:

https://github.com/Trixarian/NetherSX2-patch

### Citra

The Citra emulator is no longer in active development and it's unclear where it can be obtained and whether it will be worked on in the future. The Citra MMJ fork is still available for download from their GitHub site.

https://github.com/weihuoya/citra/releases

### ColEm

This emulator can be installed from the Play store. There is a paid version as well named ColEm Deluxe (ColEm+ ColecoVision Emulator is the store listing name).

Although this emulator supports both the Adam and ColecoVision systems it can unfortunately not do both interchangeably. In order to play Adam games you need to go into the Emulation settings in ColEm and tick the _Coleco Adam_ box. And likewise you'll need to untick it any time you want to play a ColecoVision game. This is true for launching games from ES-DE as well as starting them from inside the emulator GUI.

https://play.google.com/store/apps/details?id=com.fms.colem \
https://play.google.com/store/apps/details?id=com.fms.colem.deluxe

### Dolphin

The Play store version is somehow up to date and could be used, otherwise the F-Droid store version is up to date, or you could download the latest release directly from their website.

https://play.google.com/store/apps/details?id=org.dolphinemu.dolphinemu \
https://f-droid.org/en/packages/org.dolphinemu.dolphinemu \
https://dolphin-emu.org/download/

### Dolphin MMJR and MMJR2

Although it's not normally recommended to use the unofficial Dolphin forks because they are mostly ancient and have been surpassed by the mainline Dolphin releases, there could still be situations where it's useful. For example on weaker devices where the older release would work better.

There are multiple MMJR forks in existence, but the ones supported by ES-DE are `Dolphin.MMJR.v11505.apk` and `MMJR.v2.0-17878.apk` which can be downloaded from here:

https://archive.org/details/dolphin-and-citra-fork-backup

### DraStic

This emulator can be installed from the Play store as a paid app. Note that it does not support launching of zipped game files.

https://play.google.com/store/apps/details?id=com.dsemu.drastic

### DuckStation

The Play store version of this emulator is getting frequent updates and is therefore recommended.

https://play.google.com/store/apps/details?id=com.github.stenzek.duckstation

### EKA2L1

This emulator can be downloaded from their GitHub site.

https://github.com/EKA2L1/EKA2L1/releases

There does not seem to be a way to launch individual EKA2L1 games from a frontend application on Android, instead ES-DE will simply launch the EKA2L1 user interface and you'll have to manually start your game from there.

### ePSXe

This emulator can be installed from the Play store as a paid app.

https://play.google.com/store/apps/details?id=com.epsxe.ePSXe

### EX Plus Alpha emulators

These set of emulators also known as the "Robert Broglia" emulators consist of 2600.emu, C64.emu, GBA.emu, GBC.emu, Lynx.emu, NEO.emu, NES.emu, NGP.emu, MD.emu, MSX.emu, PCE.emu, Snes9x EX+, Saturn.emu and Swan.emu

You can install them via Google Play (as paid apps) or download them from their GitHub automatic build system.

https://play.google.com/store/apps/developer?id=Robert+Broglia \
https://github.com/Rakashazi/emu-ex-plus-alpha/actions

There are also some BIOS files and similar that are needed to run these emulators, and which can be downloaded from their website.

https://www.explusalpha.com/

### Fake-08

This RetroArch core is a good port of the official PICO-8 game engine which does not exist on Android. It's not shipped with RetroArch by default though so you need to manually install it. After downloading it you'll need to place the core inside's RetroArch's downloads directory and then install it from the RetroArch app.

You must use the 64-bit version with the filename `libfake08-arm64.so` and it has to be renamed to `fake08_libretro_android.so` before you install it into RetroArch. Details on how to manually install cores in RetroArch can be found on the Internet, but the short version is to use the _Install or Restore a Core_ entry in the _Load Core_ menu. Fake-08 can be downloaded from their GitHub site.

https://github.com/jtothebell/fake-08/releases

### Flycast

Flycast is not available on the Play store or the F-Droid store, but it can be downloaded from their GitHub site.

https://github.com/flyinghead/flycast/releases

### fMSX

This emulator can be installed from the Play store. There is a paid version as well named fMSX Deluxe (fMSX+ MSX/MSX2 Emulator is the store listing name).

https://play.google.com/store/apps/details?id=com.fms.fmsx \
https://play.google.com/store/apps/details?id=com.fms.fmsx.deluxe

### FPseNG and FPse

These emulators can be installed from the Play store as a paid apps. FPseNG is the more modern version so it's probably best to go for that. Note that these emulators do not support .chd files.

https://play.google.com/store/apps/details?id=com.emulator.fpse64 \
https://play.google.com/store/apps/details?id=com.emulator.fpse

### iNES

This emulator can be installed from the Play store.

https://play.google.com/store/apps/details?id=com.fms.ines.free

### MAME4droid 2024 and MAME4droid

These emulators can be installed from the Play store. It's strongly recommended to go for the _MAME4droid 2024_ version as this is updated with a recent MAME release while the older _MAME4droid_ is using an ancient MAME release.

https://play.google.com/store/apps/details?id=com.seleuco.mame4d2024 \
https://play.google.com/store/apps/details?id=com.seleuco.mame4droid

### MasterGear

This emulator can be installed from the Play store as a paid app.

https://play.google.com/store/apps/details?id=com.fms.mg

### melonDS

This emulator can be installed from the Play store but it's quite buggy. Every time you add a new game to the ROM directory you need to start the emulator and manually refresh the game list or you won't be able to launch the game from ES-DE.

https://play.google.com/store/apps/details?id=me.magnum.melonds

### M64Plus FZ

This emulator can be installed from the Amazon Appstore or the Google Play store. The Pro version is recommended to avoid annoying ads.

https://www.amazon.com/dp/B09L5FB7T4 \
https://play.google.com/store/apps/details?id=org.mupen64plusae.v3.fzurita.pro \
https://play.google.com/store/apps/details?id=org.mupen64plusae.v3.fzurita

### Mupen64Plus AE

This emulator is very similar to M64Plus FZ and it can be downloaded from their GitHub automatic build system.

https://github.com/mupen64plus-ae/mupen64plus-ae/actions

### My Boy! and My OldBoy!

These emulators can be installed from the Play store as paid apps. There are also free/Lite versions availble for these emulators but they have not been updated in years and don't run on modern devices. As such they are not supported by ES-DE.

https://play.google.com/store/apps/details?id=com.fastemulator.gba \
https://play.google.com/store/apps/details?id=com.fastemulator.gbc

### Nesoid

Nesoid is not available on the Play store but it can be installed from the F-Droid store, or it can be downloaded from their GitHub site.

https://f-droid.org/en/packages/com.androidemu.nes \
https://github.com/proninyaroslav/nesoid/releases

### OpenBOR

Although OpenBOR is working fine on Android it's not possible to properly integrate it with a frontend, you'll instead need to install your game PAKs into the `/sdcard/OpenBOR/Paks` directory and create dummy .openbor files for your games in `ROMs/openbor` and after launching a game from ES-DE you need to manually start it from inside the OpenBOR GUI. There are more detailed setup instructions in the _OpenBOR_ section of the [User guide](USERGUIDE-DEV.md#openbor).

You can download OpenBOR from their GitHub site, the version named _OpenBOR v3.0 Build 6391_ has for example been proven to work well.

https://github.com/DCurrent/openbor/releases

### Panda3DS (pandroid)

This emulator is in early development and there currently seems to be no way to run a game directly from ES-DE. Instead the emulator GUI will be displayed on game launch and you will need to manually select the game from there. Note that the Android build of this emulator is named _pandroid_, but as the overall project name is Panda3DS it will be referred to as such in ES-DE. This emulator can be downloaded from their GitHub site.

https://github.com/wheremyfoodat/Panda3DS/releases

### Pizza Boy GBA and Pizza Boy GBC

The Pizza Boy GBA and Pizza Boy GBC emulators used to be available on the Play store but have since been taken down. There used to be Basic (free) versions and Pro (paid) versions. It's unclear what the status is for these emulators and if they are still in active development. It's also unclear where they can be downloaded.

As of writing this, the latest available Basic version of the GBA emulator does not seem to be able to launch games from ES-DE, but the Pro version is working fine. Both the Basic and Pro versions of the GBC emulator are working correctly.

### Play!

This PlayStation 2 emulator can be downloaded from their website.

https://www.purei.org/downloads.php

### PPSSPP

The Play store version of this emulator is getting frequent updates and is therefore recommended. There is a paid Gold version as well which is functionally identical to the free version.

https://play.google.com/store/apps/details?id=org.ppsspp.ppsspp \
https://play.google.com/store/apps/details?id=org.ppsspp.ppssppgold

Make sure that you press the _Browse_ button in PPSSPP when you're adding scoped storage access to your games directory or you will not be able to launch any games from ES-DE.

### Ruffle

This emulator can be downloaded from their GitHub site.

https://github.com/torokati44/ruffle-android/releases

### Real3DOPlayer

This 3DO Interactive Multiplayer emulator can be downloaded from their website.

http://www.arts-union.ru/node/23

### Redream

This emulator can be installed for free from the Play store and can later be upgraded to the Premium version from inside the application.

https://play.google.com/store/apps/details?id=io.recompiled.redream


### Skyline

This emulator is not in active development any longer, but the latest release can be found on archive.org.

https://archive.org/details/skyline-edge-all-versions

### Speccy

This emulator can be installed from the Play store. There is a paid version as well named Speccy Deluxe (Speccy+ ZX Spectrum Emulator is the store listing name).

Although this emulator supports both the Sinclar ZX Spectrum and MGT SAM Coupé systems it can unfortunately not do both interchangeably. In order to play SAM Coupé games you need to go into the Emulation settings in Speccy and select _Sam Coupe_ from the _Computer Model_ selection screen. And likewise you'll need to change it back any time you want to play a ZX Spectrum game. This is true for launching games from ES-DE as well as starting them from inside the emulator GUI.

https://play.google.com/store/apps/details?id=com.fms.speccy \
https://play.google.com/store/apps/details?id=com.fms.speccy.deluxe

### Vita3K

This PlayStation Vita emulator can be downloaded from their GitHub site. Refer to the User guide for detailed game setup instructions.

https://github.com/Vita3K/Vita3K-Android/releases

### Yaba Sanshiro 2

This emulator can be installed from the Play store, there is a paid Pro version as well. At the time of writing only the Pro version works when launching games from ES-DE.

https://play.google.com/store/apps/details?id=org.devmiyax.yabasanshioro2 \
https://play.google.com/store/apps/details?id=org.devmiyax.yabasanshioro2.pro

## Device compatibility

This is clearly not a complete list of Android devices, but rather those we know have been tested with ES-DE and for which there is a known status.

| Manufacturer | Model            | Android release | Supported | Known issues        | Comment                    |
| :----------- | :--------------- | :-------------- | :-------- | :------------------ | :------------------------- |
| Anbernic     | RG556            | 13              | Yes       | None                |                            |
| Ayn          | Odin             | 10              | Yes       | None                |                            |
| Ayn          | Odin Lite        | 11              | Yes       | None                |                            |
| Ayn          | Odin 2           | 13              | Yes       | Can't launch some emulators,<br>Can't have dots in directory names,<br>Minor audio issues | Bugs in the firmware/OS image |
| Google       | Pixel 4a         | 13, 14          | Yes       | None                |                            |
| Google       | Pixel Tablet     | 14              | Yes       | None                |                            |
| Honor        | 20 lite          | 10              | Yes       | None                |                            |
| Honor        | Magic5 Pro       | 13              | Yes       | None                |                            |
| Huawei       | Mate 10 Pro      | 10              | Yes       | None                |                            |
| Meta         | Quest 3          | 12 ?            | Yes       | None                |                            |
| Nokia        | 5.4              | 12              | Yes       | None                |                            |
| Nokia        | 7 Plus           | 10              | Yes       | None                |                            |
| Nvidia       | Shield Pro       | 11 (TV)         | Yes       | None                | Limited RAM capacity on this device makes it unsuitable for demanding themes and large game collections |
| OnePlus      | 6T               | 11              | Yes       | None                |                            |
| Oppo         | A15              | 10              | Yes       | None                |                            |
| REDMAGIC     | 8 Pro            | 13              | Yes       | None                |                            |
| Retroid      | Pocket 2s        | 11              | Yes       | None                |                            |
| Retroid      | Pocket 3+        | 11              | Yes       | None                |                            |
| Retroid      | Pocket 4 Pro     | 13              | Yes       | None                |                            |
| Samsung      | Galaxy Note 20   | 13              | No        | Fails at configurator/onboarding | Has a non-standard app permission screen, possibly this breaks the configurator |
| Samsung      | Galaxy S10       | 12              | Yes       | None                |                            |
| Samsung      | Galaxy S20       | 13              | Yes       | None                |                            |
| Samsung      | Galaxy S22 Ultra | 14              | Yes       | None                |                            |
| Samsung      | Galaxy S23 Ultra | 14              | Yes       | None                |                            |
| Samsung      | Galaxy S24 Ultra | 14              | Yes       | None                |                            |
| Samsung      | Galaxy Tab A7    | 12              | Yes       | None                |                            |
| Ulefone      | Note 6P          | 11              | Yes       | None                |                            |
| Wiko         | Voix             | 12              | No        | Fails at configurator/onboarding | Probably a bug in the firmware/OS image as a libc system call fails |
| Xiaomi       | Pad 5            | 13              | Yes       | None                |                            |
| Xiaomi       | Redmi Note 11    | 11              | Yes       | None                |                            |

## Supported game systems

All emulators are RetroArch cores unless marked as **(Standalone)**

The **@** symbol indicates that the emulator is _deprecated_ and will be removed in a future ES-DE release.

| System name           | Full name                                      | Default emulator                  | Alternative emulators             | Needs BIOS   | Recommended game setup               |
| :-------------------- | :--------------------------------------------- | :-------------------------------- | :-------------------------------- | :----------- | :----------------------------------- |
| 3do                   | 3DO Interactive Multiplayer                    | Opera                             | Real3DOPlayer **(Standalone)**    | Yes          |                                      |
| adam                  | Coleco Adam                                    | ColEm **(Standalone)**            |                                   | No           |                                      |
| ags                   | Adventure Game Studio Game Engine              | _Placeholder_                     |                                   |              |                                      |
| amiga                 | Commodore Amiga                                | PUAE                              | PUAE 2021                         | Yes          |                                      |
| amiga1200             | Commodore Amiga 1200                           | PUAE                              | PUAE 2021                         | Yes          |                                      |
| amiga600              | Commodore Amiga 600                            | PUAE                              | PUAE 2021                         | Yes          |                                      |
| amigacd32             | Commodore Amiga CD32                           | PUAE                              | PUAE 2021                         | Yes          |                                      |
| amstradcpc            | Amstrad CPC                                    | Caprice32                         | CrocoDS                           | No           | Single archive or disk file          |
| android               | Google Android                                 | _Placeholder_                     |                                   |              |                                      |
| apple2                | Apple II                                       | _Placeholder_                     |                                   |              |                                      |
| apple2gs              | Apple IIGS                                     | _Placeholder_                     |                                   |              |                                      |
| arcade                | Arcade                                         | MAME - Current                    | MAME 2010,<br>MAME 2003-Plus,<br>MAME 2000,<br>MAME4droid 2024 **(Standalone)**,<br>MAME4droid **(Standalone)**,<br>NEO.emu **(Standalone)**,<br>FinalBurn Neo,<br>FB Alpha 2012,<br>Geolith,<br>Flycast,<br>Flycast **(Standalone)** | Depends      |                                      |
| arcadia               | Emerson Arcadia 2001                           | _Placeholder_                     |                                   |              |                                      |
| archimedes            | Acorn Archimedes                               | _Placeholder_                     |                                   |              |                                      |
| arduboy               | Arduboy Miniature Game System                  | Arduous                           |                                   | No           | Single archive or .hex file          |
| astrocde              | Bally Astrocade                                | _Placeholder_                     |                                   |              |                                      |
| atari2600             | Atari 2600                                     | Stella                            | Stella 2014,<br>2600.emu **(Standalone)** | No           | Single archive or ROM file           |
| atari5200             | Atari 5200                                     | a5200                             | Atari800                          | Yes          | Single archive or ROM file           |
| atari7800             | Atari 7800 ProSystem                           | ProSystem                         |                                   | Yes          | Single archive or ROM file           |
| atari800              | Atari 800                                      | Atari800                          |                                   | Yes          |                                      |
| atarijaguar           | Atari Jaguar                                   | Virtual Jaguar                    |                                   | No           |                                      |
| atarijaguarcd         | Atari Jaguar CD                                | _Placeholder_                     |                                   |              |                                      |
| atarilynx             | Atari Lynx                                     | Handy                             | Beetle Lynx,<br>Lynx.emu **(Standalone)** | No           | Single archive or ROM file           |
| atarist               | Atari ST [also STE and Falcon]                 | Hatari                            |                                   | Yes          | Single archive or image file for single-diskette games, .m3u playlist for multi-diskette games |
| atarixe               | Atari XE                                       | Atari800                          |                                   | Yes          |                                      |
| atomiswave            | Sammy Corporation Atomiswave                   | Flycast                           | Flycast **(Standalone)**          | Depends      | Single archive  file                 |
| bbcmicro              | Acorn Computers BBC Micro                      | _Placeholder_                     |                                   |              |                                      |
| c64                   | Commodore 64                                   | VICE x64sc Accurate               | VICE x64 Fast,<br>VICE x64 SuperCPU,<br>VICE x128,<br>C64.emu **(Standalone)** | No           | Single archive or image file for tape, cartridge or single-diskette games, .m3u playlist for multi-diskette games |
| cdimono1              | Philips CD-i                                   | SAME CDi                          |                                   | Yes          | Single .bin/.cue pair                |
| cdtv                  | Commodore CDTV                                 | PUAE                              | PUAE 2021                         | Yes          |                                      |
| chailove              | ChaiLove Game Engine                           | ChaiLove                          |                                   |              |                                      |
| channelf              | Fairchild Channel F                            | FreeChaF                          |                                   | Yes          | Single archive or ROM file           |
| coco                  | Tandy Color Computer                           | _Placeholder_                     |                                   |              |                                      |
| colecovision          | Coleco ColecoVision                            | blueMSX                           | Gearcoleco,<br>MSX.emu **(Standalone)**,<br>ColEm **(Standalone)** | Yes          | Single archive or ROM file           |
| consolearcade         | Console Arcade Systems                         | _Placeholder_                     |                                   |              |                                      |
| cps                   | Capcom Play System                             | MAME - Current                    | MAME 2010,<br>MAME 2003-Plus,<br>MAME 2000,<br>MAME4droid 2024 **(Standalone)**,<br>MAME4droid **(Standalone)**,<br>FinalBurn Neo,<br>FB Alpha 2012,<br>FB Alpha 2012 CPS-1,<br>FB Alpha 2012 CPS-2,<br>FB Alpha 2012 CPS-3 | Depends      |                                      |
| cps1                  | Capcom Play System I                           | MAME - Current                    | MAME 2010,<br>MAME 2003-Plus,<br>MAME 2000,<br>MAME4droid 2024 **(Standalone)**,<br>MAME4droid **(Standalone)**,<br>FinalBurn Neo,<br>FB Alpha 2012,<br>FB Alpha 2012 CPS-1 | Depends      |                                      |
| cps2                  | Capcom Play System II                          | MAME - Current                    | MAME 2010,<br>MAME 2003-Plus,<br>MAME 2000,<br>MAME4droid 2024 **(Standalone)**,<br>MAME4droid **(Standalone)**,<br>FB Alpha 2012,<br>FB Alpha 2012 CPS-2 | Depends      |                                      |
| cps3                  | Capcom Play System III                         | MAME - Current                    | MAME 2010,<br>MAME 2003-Plus,<br>MAME 2000,<br>MAME4droid 2024 **(Standalone)**,<br>MAME4droid **(Standalone)**,<br>FB Alpha 2012,<br>FB Alpha 2012 CPS-3 | Depends      |                                      |
| crvision              | VTech CreatiVision                             | _Placeholder_                     |                                   |              |                                      |
| daphne                | Daphne Arcade LaserDisc Emulator               | DirkSimple                        |                                   | No           |                                      |
| desktop               | Desktop Applications                           | _Placeholder_                     |                                   |              |                                      |
| doom                  | Doom                                           | PrBoom                            |                                   | No           |                                      |
| dos                   | DOS (PC)                                       | DOSBox-Pure                       | DOSBox-Core,<br>DOSBox-SVN        | No           |                                      |
| dragon32              | Dragon Data Dragon 32                          | _Placeholder_                     |                                   |              |                                      |
| dreamcast             | Sega Dreamcast                                 | Flycast                           | Flycast **(Standalone)**,<br>Redream **(Standalone)** | No           | In separate folder interpreted as a file, with .m3u playlist if multi-disc game |
| easyrpg               | EasyRPG Game Engine                            | EasyRPG                           |                                   | No           |                                      |
| electron              | Acorn Electron                                 | _Placeholder_                     |                                   |              |                                      |
| emulators             | Emulators                                      | _Placeholder_                     |                                   |              |                                      |
| epic                  | Epic Games Store                               | _Placeholder_                     |                                   |              |                                      |
| famicom               | Nintendo Family Computer                       | Mesen                             | Nestopia UE,<br>FCEUmm,<br>QuickNES,<br>NES.emu **(Standalone)**,<br>iNES **(Standalone)**,<br>Nesoid **(Standalone)** | No           | Single archive or ROM file           |
| fba                   | FinalBurn Alpha                                | FB Alpha 2012                     | FB Alpha 2012 Neo Geo,<br>FB Alpha 2012 CPS-1,<br>FB Alpha 2012 CPS-2,<br>FB Alpha 2012 CPS-3 | Yes          |                                |
| fbneo                 | FinalBurn Neo                                  | FinalBurn Neo                     |                                   | Yes          |                                      |
| fds                   | Nintendo Famicom Disk System                   | Mesen                             | Nestopia UE,<br>FCEUmm,<br>NES.emu **(Standalone)**,<br>iNES **(Standalone)**,<br>Nesoid **(Standalone)** | Yes          | Single archive or ROM file |
| flash                 | Adobe Flash                                    | Ruffle **(Standalone)**           |                                   | No           | Single .swf file                     |
| fm7                   | Fujitsu FM-7                                   | _Placeholder_                     |                                   |              |                                      |
| fmtowns               | Fujitsu FM Towns                               | _Placeholder_                     |                                   |              |                                      |
| fpinball              | Future Pinball                                 | _Placeholder_                     |                                   |              |                                      |
| gamate                | Bit Corporation Gamate                         | _Placeholder_                     |                                   |              |                                      |
| gameandwatch          | Nintendo Game and Watch                        | Multi (MESS)                      | MAME4droid 2024 **(Standalone)**,<br>Handheld Electronic (GW) | No           | Single archive or ROM file           |
| gamecom               | Tiger Electronics Game.com                     | _Placeholder_                     |                                   |              |                                      |
| gamegear              | Sega Game Gear                                 | Genesis Plus GX                   | Genesis Plus GX Wide,<br>Gearsystem,<br>SMS Plus GX,<br>PicoDrive,<br>MasterGear **(Standalone)** | No           | Single archive or ROM file |
| gb                    | Nintendo Game Boy                              | Gambatte                          | SameBoy,<br>Gearboy,<br>TGB Dual,<br>DoubleCherryGB,<br>Mesen-S,<br>bsnes,<br>mGBA,<br>VBA-M,<br>GBC.emu **(Standalone)**,<br>My OldBoy! **(Standalone**),<br>Pizza Boy GBC **(Standalone)** | No           | Single archive or ROM file |
| gba                   | Nintendo Game Boy Advance                      | mGBA                              | VBA-M,<br>VBA Next,<br>gpSP,<br>GBA.emu **(Standalone)**,<br>My Boy! **(Standalone)**,<br>Pizza Boy GBA **(Standalone)** | No          | Single archive or ROM file |
| gbc                   | Nintendo Game Boy Color                        | Gambatte                          | SameBoy,<br>Gearboy,<br>TGB Dual,<br>DoubleCherryGB,<br>Mesen-S,<br>bsnes,<br>mGBA,<br>VBA-M,<br>GBC.emu **(Standalone)**,<br>My OldBoy! **(Standalone**),<br>Pizza Boy GBC **(Standalone)** | No           | Single archive or ROM file |
| gc                    | Nintendo GameCube                              | Dolphin                           | Dolphin **(Standalone)**,<br>Dolphin MMJR **(Standalone)**,<br>Dolphin MMJR2 **(Standalone)** | No           | Disc image file for single-disc games, .m3u playlist for multi-disc games |
| genesis               | Sega Genesis                                   | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive,<br>MD.emu **(Standalone)** | No           | Single archive or ROM file |
| gmaster               | Hartung Game Master                            | _Placeholder_                     |                                   |              |                                      |
| gx4000                | Amstrad GX4000                                 | Caprice32                         | CrocoDS                           | No           | Single archive or ROM file           |
| intellivision         | Mattel Electronics Intellivision               | FreeIntv                          |                                   | Yes          | Single archive or ROM file           |
| j2me                  | Java 2 Micro Edition (J2ME)                    | SquirrelJME                       |                                   | No           | Single .jar file                     |
| kodi                  | Kodi Home Theatre Software                     | _Placeholder_                     |                                   |              |                                      |
| laserdisc             | LaserDisc Games                                | DirkSimple                        |                                   | No           |                                      |
| lcdgames              | LCD Handheld Games                             | Multi (MESS)                      | MAME4droid 2024 **(Standalone)**,<br>Handheld Electronic (GW) | No           | Single archive or ROM file           |
| lowresnx              | LowRes NX Fantasy Console                      | LowRes NX                         |                                   | No           | Single ROM file                      |
| lutris                | Lutris Open Gaming Platform                    | _Placeholder_                     |                                   |              |                                      |
| lutro                 | Lutro Game Engine                              | Lutro                             |                                   |              |                                      |
| macintosh             | Apple Macintosh                                | _Placeholder_                     |                                   |              |                                      |
| mame                  | Multiple Arcade Machine Emulator               | MAME - Current                    | MAME 2010,<br>MAME 2003-Plus,<br>MAME 2000,<br>MAME4droid 2024 **(Standalone)**,<br>MAME4droid **(Standalone)**,<br>NEO.emu **(Standalone)**,<br>FinalBurn Neo,<br>FB Alpha 2012,<br>Geolith,<br>Flycast,<br>Flycast **(Standalone)** | Depends      |                                      |
| mame-advmame          | AdvanceMAME                                    | _Placeholder_                     |                                   |              |                                      |
| mastersystem          | Sega Master System                             | Genesis Plus GX                   | Genesis Plus GX Wide,<br>SMS Plus GX,<br>Gearsystem,<br>PicoDrive,<br>MD.emu **(Standalone)**,<br>MasterGear **(Standalone)** | No           | Single archive or ROM file |
| megacd                | Sega Mega-CD                                   | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive,<br>MD.emu **(Standalone)** | Yes          |                                      |
| megacdjp              | Sega Mega-CD [Japan]                           | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive,<br>MD.emu **(Standalone)** | Yes          |                                      |
| megadrive             | Sega Mega Drive                                | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive,<br>MD.emu **(Standalone)** | No           | Single archive or ROM file           |
| megadrivejp           | Sega Mega Drive [Japan]                        | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive,<br>MD.emu **(Standalone)** | No           | Single archive or ROM file           |
| megaduck              | Creatronic Mega Duck                           | SameDuck                          |                                    | No           | Single archive or ROM file           |
| mess                  | Multi Emulator Super System                    | Multi (MESS)                      |                                   |              |                                      |
| model2                | Sega Model 2                                   | MAME - Current                    |                                   | Yes          |                                      |
| model3                | Sega Model 3                                   | _Placeholder_                     |                                   |              |                                      |
| moto                  | Thomson MO/TO Series                           | Theodore                          |                                   |              |                                      |
| msx                   | MSX                                            | blueMSX                           | fMSX,<br>fMSX **(Standalone)**,<br>MSX.emu **(Standalone)** | Yes except for fMSX standalone |                                      |
| msx1                  | MSX1                                           | blueMSX                           | fMSX,<br>fMSX **(Standalone)**,<br>MSX.emu **(Standalone)** | Yes except for fMSX standalone |                                      |
| msx2                  | MSX2                                           | blueMSX                           | fMSX,<br>fMSX **(Standalone)**,<br>MSX.emu **(Standalone)** | Yes except for fMSX standalone |                                      |
| msxturbor             | MSX Turbo R                                    | blueMSX                           | fMSX,<br>MSX.emu **(Standalone)** | Yes          |                                      |
| mugen                 | M.U.G.E.N Game Engine                          | _Placeholder_                     |                                   | Yes          |                                      |
| multivision           | Othello Multivision                            | Gearsystem                        | MasterGear **(Standalone)**       | No           | Single archive or ROM file           |
| naomi                 | Sega NAOMI                                     | Flycast                           | Flycast **(Standalone)**          | Yes          | Single archive file + .chd file in subdirectory if GD-ROM game |
| naomi2                | Sega NAOMI 2                                   | Flycast                           | Flycast **(Standalone)**          | Yes          | Single archive file + .chd file in subdirectory if GD-ROM game |
| naomigd               | Sega NAOMI GD-ROM                              | Flycast                           | Flycast **(Standalone)**          | Yes          | Single archive file + .chd file in subdirectory if GD-ROM game |
| n3ds                  | Nintendo 3DS                                   | Citra                             | Citra **(Standalone)**,<br>Citra Canary **(Standalone)**,<br>Citra MMJ **(Standalone)**,<br>Panda3DS **(Standalone)**  | No           | Single ROM file       |
| n64                   | Nintendo 64                                    | Mupen64Plus-Next                  | M64Plus FZ **(Standalone)**,<br>Mupen64Plus AE **(Standalone)**,<br>ParaLLEl N64 | No           | Single archive or ROM file |
| n64dd                 | Nintendo 64DD                                  | Mupen64Plus-Next                  | M64Plus FZ **(Standalone)**,<br>Mupen64Plus AE **(Standalone)**,<br>ParaLLEl N64 | Yes          |                                      |
| nds                   | Nintendo DS                                    | melonDS DS                        | melonDS **(Standalone)**,<br>melonDS Nightly **(Standalone)**,<br>DeSmuME,<br>DeSmuME 2015,<br>DraStic **(Standalone)** | No           | Single archive or ROM file |
| neogeo                | SNK Neo Geo                                    | FinalBurn Neo                     | Geolith,<br>NEO.emu **(Standalone)**,<br>MAME4droid 2024 **(Standalone)**,<br>MAME4droid **(Standalone)** | Yes          | Single archive or ROM file |
| neogeocd              | SNK Neo Geo CD                                 | NeoCD                             | FinalBurn Neo                     | Yes          |                                      |
| neogeocdjp            | SNK Neo Geo CD [Japan]                         | NeoCD                             | FinalBurn Neo                     | Yes          |                                      |
| nes                   | Nintendo Entertainment System                  | Mesen                             | Nestopia UE,<br>FCEUmm,<br>QuickNES,<br>NES.emu **(Standalone)**,<br>iNES **(Standalone)**,<br>Nesoid **(Standalone)** | No           | Single archive or ROM file           |
| ngage                 | Nokia N-Gage                                   | EKA2L1 **(Standalone)**           |                                   | Yes          | See the specific _Symbian and Nokia N-Gage_ section in the User guide |
| ngp                   | SNK Neo Geo Pocket                             | Beetle NeoPop                     | RACE,<br>NGP.emu **(Standalone)** | No           | Single archive or ROM file           |
| ngpc                  | SNK Neo Geo Pocket Color                       | Beetle NeoPop                     | RACE,<br>NGP.emu **(Standalone)** | No           | Single archive or ROM file           |
| odyssey2              | Magnavox Odyssey 2                             | O2EM                              |                                   | Yes          | Single archive or ROM file           |
| openbor               | OpenBOR Game Engine                            | OpenBOR **(Standalone)**          |                                   | No           | See the specific _OpenBOR_ section in the User guide |
| oric                  | Tangerine Computer Systems Oric                | _Placeholder_                     |                                   |              |                                      |
| palm                  | Palm OS                                        | Mu                                |                                   |              |                                      |
| pc                    | IBM PC                                         | DOSBox-Pure                       | DOSBox-Core,<br>DOSBox-SVN        | No           |                                      |
| pc88                  | NEC PC-8800 Series                             | QUASI88                           |                                   | Yes          |                                      |
| pc98                  | NEC PC-9800 Series                             | Neko Project II Kai               | Neko Project II                   |              |                                      |
| pcarcade              | PC Arcade Systems                              | _Placeholder_                     |                                   |              |                                      |                                   |
| pcengine              | NEC PC Engine                                  | Beetle PCE                        | Beetle PCE FAST,<br>Beetle SuperGrafx,<br>PCE.emu **(Standalone)** | No           | Single archive or ROM file           |
| pcenginecd            | NEC PC Engine CD                               | Beetle PCE                        | Beetle PCE FAST,<br>Beetle SuperGrafx,<br>PCE.emu **(Standalone)** | Yes          |                                      |
| pcfx                  | NEC PC-FX                                      | Beetle PC-FX                      |                                   | Yes          |                                      |
| pico8                 | PICO-8 Fantasy Console                         | Fake-08                           | Retro8                            | No           | See the specific _PICO-8_ section in the User guide |
| plus4                 | Commodore Plus/4                               | VICE xplus4                       |                                   | No           | Single archive or image file for tape, cartridge or single-diskette games, .m3u playlist for multi-diskette games |
| pokemini              | Nintendo Pokémon Mini                          | PokeMini                          |                                   | No           |                                      |
| ports                 | Ports                                          | ECWolf (Wolfenstein 3D)           | NXEngine (Cave Story),<br>OpenLara (Tomb Raider),<br>Super Bros War | Yes for ECWolf |                                      |
| ps2                   | Sony PlayStation 2                             | AetherSX2 **(Standalone)**        | Play! **(Standalone)**            | Yes for AetherSX2 |                                      |
| ps3                   | Sony PlayStation 3                             | _Placeholder_                     |                                   |              |                                      |
| ps4                   | Sony PlayStation 4                             | _Placeholder_                     |                                   |              |                                      |
| psp                   | Sony PlayStation Portable                      | PPSSPP                            | PPSSPP **(Standalone)**           | No           | Single disc image file               |
| psvita                | Sony PlayStation Vita                          | Vita3K **(Standalone)**           |                                   | Yes          | See the specific _Sony PlayStation Vita_ section in the User guide |
| psx                   | Sony PlayStation                               | Beetle PSX                        | Beetle PSX HW,<br>PCSX ReARMed,<br>SwanStation,<br>DuckStation **(Standalone)**,<br>ePSXe **(Standalone)**,<br>FPseNG **(Standalone)**,<br>FPse **(Standalone)** | Yes          | .chd file for single-disc games, .m3u playlist for multi-disc games |
| pv1000                | Casio PV-1000                                  | _Placeholder_                     |                                   |              |                                      |
| quake                 | Quake                                          | TyrQuake                          | vitaQuake 2,<br>vitaQuake 2 [Rogue],<br>vitaQuake 2 [Xatrix],<br>vitaQuake 2 [Zaero] | No           |                                      |
| samcoupe              | MGT SAM Coupé                                  | Speccy **(Standalone)**           |                                   | No           | Single archive or ROM file           |
| satellaview           | Nintendo Satellaview                           | Snes9x - Current                  | Snes9x 2010,<br>Snes9x 2005 Plus,<br>Snes9x EX+ **(Standalone)**,<br>bsnes,<br>bsnes-hd,<br>bsnes-mercury Accuracy,<br>Mesen-S |              |                                      |
| saturn                | Sega Saturn                                    | Beetle Saturn                     | YabaSanshiro,<br>Yaba Sanshiro 2 **(Standalone)**,<br>Yabause,<br>Saturn.emu **(Standalone)** | Yes          | .chd file for single-disc games, .m3u playlist for multi-disc games |
| saturnjp              | Sega Saturn [Japan]                            | Beetle Saturn                     | YabaSanshiro,<br>Yaba Sanshiro 2 **(Standalone)**,<br>Yabause,<br>Saturn.emu **(Standalone)** | Yes          | .chd file for single-disc games, .m3u playlist for multi-disc games |
| scummvm               | ScummVM Game Engine                            | ScummVM                           |                                   | No           |                                      |
| scv                   | Epoch Super Cassette Vision                    | _Placeholder_                     |                                   |              |                                      |
| sega32x               | Sega Mega Drive 32X                            | PicoDrive                         |                                   | No           | Single archive or ROM file           |
| sega32xjp             | Sega Super 32X [Japan]                         | PicoDrive                         |                                   | No           | Single archive or ROM file           |
| sega32xna             | Sega Genesis 32X [North America]               | PicoDrive                         |                                   | No           | Single archive or ROM file           |
| segacd                | Sega CD                                        | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive,<br>MD.emu **(Standalone)** | Yes          |                                      |
| sfc                   | Nintendo SFC (Super Famicom)                   | Snes9x - Current                  | Snes9x 2010,<br>Snes9x 2005 Plus,<br>Snes9x EX+ **(Standalone)**,<br>bsnes,<br>bsnes-hd,<br>bsnes-mercury Accuracy,<br>Beetle Supafaust,<br>Mesen-S | No           | Single archive or ROM file |
| sg-1000               | Sega SG-1000                                   | Genesis Plus GX                   | Genesis Plus GX Wide,<br>Gearsystem,<br>blueMSX,<br>MasterGear **(Standalone)** | No           | Single archive or ROM file           |
| sgb                   | Nintendo Super Game Boy                        | Mesen-S                           | SameBoy,<br>mGBA                  |              |  Single archive or ROM file |
| snes                  | Nintendo SNES (Super Nintendo)                 | Snes9x - Current                  | Snes9x 2010,<br>Snes9x 2005 Plus,<br>Snes9x EX+ **(Standalone)**,<br>bsnes,<br>bsnes-hd,<br>bsnes-mercury Accuracy,<br>Beetle Supafaust,<br>Mesen-S | No           | Single archive or ROM file |
| snesna                | Nintendo SNES (Super Nintendo) [North America] | Snes9x - Current                  | Snes9x 2010,<br>Snes9x 2005 Plus,<br>Snes9x EX+ **(Standalone)**,<br>bsnes,<br>bsnes-hd,<br>bsnes-mercury Accuracy,<br>Beetle Supafaust,<br>Mesen-S | No           | Single archive or ROM file |
| solarus               | Solarus Game Engine                            | _Placeholder_                     |                                   |              |                                      |
| spectravideo          | Spectravideo                                   | blueMSX                           |                                   |              |                                      |
| steam                 | Valve Steam                                    | _Placeholder_                     |                                   |              |                                      |
| stv                   | Sega Titan Video Game System                   | MAME - Current                    | MAME4droid 2024 **(Standalone)**,<br>MAME4droid **(Standalone)** | Yes          | Single archive file                  |
| sufami                | Bandai SuFami Turbo                            | Snes9x - Current                  | Snes9x 2010,<br>Snes9x 2005 Plus,<br>Snes9x EX+ **(Standalone)**,<br>bsnes,<br>bsnes-hd,<br>bsnes-mercury Accuracy |              |                                      |
| supergrafx            | NEC SuperGrafx                                 | Beetle SuperGrafx                 | Beetle PCE,<br>PCE.emu **(Standalone)** | No           | Single archive or ROM file           |
| supervision           | Watara Supervision                             | Potator                           |                                   | No           | Single archive or ROM file           |
| supracan              | Funtech Super A'Can                            | _Placeholder_                     |                                   |              |                                      |
| switch                | Nintendo Switch                                | Skyline **(Standalone)**          |                                   | Yes          |                                      |
| symbian               | Symbian                                        | EKA2L1 **(Standalone)**           |                                   | Yes          | See the specific _Symbian and Nokia N-Gage_ section in the User guide |
| tanodragon            | Tano Dragon                                    | _Placeholder_                     |                                   |              |                                      |
| tg16                  | NEC TurboGrafx-16                              | Beetle PCE                        | Beetle PCE FAST,<br>Beetle SuperGrafx,<br>PCE.emu **(Standalone)** | No           | Single archive or ROM file           |
| tg-cd                 | NEC TurboGrafx-CD                              | Beetle PCE                        | Beetle PCE FAST,<br>Beetle SuperGrafx,<br>PCE.emu **(Standalone)** | Yes          |                                      |
| ti99                  | Texas Instruments TI-99                        | _Placeholder_                     |                                   |              |                                      |
| tic80                 | TIC-80 Fantasy Computer                        | TIC-80                            |                                   | No           | Single .tic file                     |
| to8                   | Thomson TO8                                    | Theodore                          |                                   |              |                                      |
| triforce              | Namco-Sega-Nintendo Triforce                   | _Placeholder_                     |                                   |              |                                      |
| trs-80                | Tandy TRS-80                                   | _Placeholder_                     |                                   |              |                                      |
| type-x                | Taito Type X                                   | _Placeholder_                     |                                   |              |                                      |
| uzebox                | Uzebox Open Source Console                     | Uzem                              |                                   |              |                                      |
| vectrex               | GCE Vectrex                                    | vecx                              |                                   | No           | Single archive or ROM file           |
| vic20                 | Commodore VIC-20                               | VICE xvic                         |                                   | No           | Single archive or tape, cartridge or diskette image file |
| videopac              | Philips Videopac G7000                         | O2EM                              |                                   | Yes          | Single archive or ROM file           |
| virtualboy            | Nintendo Virtual Boy                           | Beetle VB                         |                                   | No           |                                      |
| vpinball              | Visual Pinball                                 | _Placeholder_                     |                                   |              |                                      |
| vsmile                | VTech V.Smile                                  | _Placeholder_                     |                                   |              |                                      |
| wasm4                 | WASM-4 Fantasy Console                         | WASM-4                            |                                   | No           | Single .wasm file                    |
| wii                   | Nintendo Wii                                   | Dolphin                           | Dolphin **(Standalone)**,<br>Dolphin MMJR **(Standalone)**,<br>Dolphin MMJR2 **(Standalone)** | No           |                                      |
| wiiu                  | Nintendo Wii U                                 | _Placeholder_                     |                                   |              |                                      |
| windows               | Microsoft Windows                              | _Placeholder_                     |                                   |              |                                      |
| windows3x             | Microsoft Windows 3.x                          | DOSBox-Pure                       |                                   | No           |                                      |
| windows9x             | Microsoft Windows 9x                           | DOSBox-Pure                       |                                   | No           |                                      |
| wonderswan            | Bandai WonderSwan                              | Beetle Cygne                      | Swan.emu **(Standalone)**         | No           | Single archive or ROM file           |
| wonderswancolor       | Bandai WonderSwan Color                        | Beetle Cygne                      | Swan.emu **(Standalone)**         | No           | Single archive or ROM file           |
| x1                    | Sharp X1                                       | X Millennium                      |                                   | No           | Single archive or diskette/tape file |
| x68000                | Sharp X68000                                   | PX68k                             |                                   | Yes          |                                      |
| xbox                  | Microsoft Xbox                                 | _Placeholder_                     |                                   |              |                                      |
| xbox360               | Microsoft Xbox 360                             | _Placeholder_                     |                                   |              |                                      |
| zmachine              | Infocom Z-machine                              | _Placeholder_                     |                                   |              |                                      |
| zx81                  | Sinclair ZX81                                  | EightyOne                         |                                   |              |                                      |
| zxnext                | Sinclair ZX Spectrum Next                      | _Placeholder_                     |                                   |              |                                      |
| zxspectrum            | Sinclair ZX Spectrum                           | Fuse                              | Speccy **(Standalone)**           | No           | Single archive or ROM file           |
