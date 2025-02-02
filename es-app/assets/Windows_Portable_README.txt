ES-DE Frontend - Portable installation on Windows
-------------------------------------------------

ES-DE release:
3.1.1

The latest version can be downloaded from https://es-de.org

Instructions:

If upgrading from a previous release, then don't unpack this ZIP archive on top of your old installation, instead follow the upgrade instructions below.

New installation:
1) The ROMs_ALL directory contains all the systems that ES-DE supports, but to decrease application startup time only copy the folders you need to the ROMs directory
2) Place your games into their respective folders in the ROMs directory tree
3) Place your emulators inside the Emulators directory
4) Start ES-DE using ES-DE.exe and enjoy some retrogaming!

Upgrading from an older release:
1) Rename your old ES-DE directory, for example to ES-DE_OLD
2) Move your games from ES-DE_OLD\ROMs\ to ES-DE\ROMs\
3) Move your emulators from ES-DE_OLD\Emulators\ to ES-DE\Emulators\
4) Move the contents of ES-DE_OLD\ES-DE\ to ES-DE\ES-DE\
   This last step includes your settings, custom collections, custom systems, scraped/downloaded media, gamelist.xml files, scripts and themes
5) Update your themes using the theme downloader to get support for all the latest systems and features

In case of issues, check ES-DE\es_log.txt for clues as to what went wrong.
Enabling the "Debug mode" setting in the "Other settings" menu or starting ES-DE.exe with the --debug flag will provide additional details.

Refer to the FAQ and user guide for more detailed instructions and documentation:
https://gitlab.com/es-de/emulationstation-de/-/blob/master/FAQ.md
https://gitlab.com/es-de/emulationstation-de/-/blob/master/USERGUIDE.md

This portable release contains a specific es_find_rules.xml file that will only look for emulators inside the portable directory tree.
If you would like to use the installer release configuration file instead which will also look for emulators elsewhere on your system then go to
resources\systems\windows\ and delete the es_find_rules.xml file and rename es_find_rules_installer.xml to es_find_rules.xml

See resources\systems\windows\es_find_rules.xml for more details about the emulators listed below.

Preconfigured emulator locations:

Emulators\RetroArch-Win64\retroarch.exe
Emulators\RetroArch\retroarch.exe
Emulators\3dSen\3dSen.exe
Emulators\AceDL\AceDL.exe
Emulators\AdvanceMAME\advmame.exe
Emulators\Altirra\Altirra64.exe
Emulators\Altirra\Altirra.exe
Emulators\AppleWin\AppleWin.exe
Emulators\ArcadeFlashWeb\ArcadeFlashWeb.exe
Emulators\ares\ares.exe
Emulators\atari800\atari800.exe
Emulators\BasiliskII\BasiliskII.exe
Emulators\BeebEm\BeebEm.exe
Emulators\BigPEmu\BigPEmu.exe
Emulators\bsnes\bsnes.exe
Emulators\cemu\Cemu.exe
Emulators\Citra\canary-mingw\citra-qt.exe
Emulators\Citra\nightly-mingw\citra-qt.exe
Emulators\ColEm\ColEm.exe
Emulators\cpcemu\cpcemu.exe
Emulators\CSpect\CSpect.exe
Emulators\Cxbx-Reloaded\cxbx.exe
Emulators\demul\demul.exe
Emulators\Dolphin-x64\Dolphin.exe
Emulators\dosbox-staging\dosbox.exe
Emulators\DOSBox-X\dosbox-x.exe
Emulators\dreamm\dreamm.exe
Emulators\duckstation\duckstation-qt-x64-ReleaseLTCG.exe
Emulators\duckstation\duckstation-qt-x64-ReleaseLTCG-SSE2.exe
Emulators\EasyRPG\Player.exe
Emulators\EKA2L1\eka2l1_qt.exe
Emulators\fbneo\fbneo64.exe
Emulators\fbneo\fbneo.exe
Emulators\flycast\flycast.exe
Emulators\flycast-dojo\flycast.exe
Emulators\FS-UAE\Windows\x86-64\fs-uae.exe
Emulators\FS-UAE-Launcher\Windows\x86-64\fs-uae-launcher.exe
Emulators\Fuse\fuse.exe
Emulators\Future Pinball\Future Pinball.exe
Emulators\gargoyle\gargoyle.exe
Emulators\Gearboy\Gearboy.exe
Emulators\gopher2600\gopher2600_windows_amd64.exe
Emulators\hatari\hatari.exe
Emulators\Hypseus Singe\hypseus.exe
Emulators\izapple2\izapple2sdl_windows_amd64.exe
Emulators\jgenesis\jgenesis-cli.exe
Emulators\KEmulator\KEmulator.exe
Emulators\kronos\kronos.exe
Emulators\lime3ds\lime3ds.exe
Emulators\m2emulator\EMULATOR.EXE
Emulators\mame\mame.exe
Emulators\mandarine\mandarine-qt.exe
Emulators\mednafen\mednafen.exe
Emulators\melonDS\melonDS.exe
Emulators\Mesen\Mesen.exe
Emulators\mGBA\mGBA.exe
Emulators\mupen64plus\mupen64plus-ui-console.exe
Emulators\noods\noods.exe
Emulators\openMSX\openmsx.exe
Emulators\Oricutron\oricutron.exe
Emulators\Panda3DS\Alber.exe
Emulators\PCSX2\pcsx2.exe
Emulators\PCSX2-Qt\pcsx2-qt.exe
Emulators\PICO-8\pico8.exe
Emulators\Play\Play.exe
Emulators\PPSSPP\PPSSPPWindows64.exe
Emulators\prboom-plus\prboom-plus.exe
Emulators\PrimeHack\Dolphin.exe
Emulators\Project64\Project64.exe
Emulators\punes\punes64.exe
Emulators\quasi88\QUASI88.exe
Emulators\redream\redream.exe
Emulators\RMG\RMG.exe
Emulators\RPCS3\rpcs3.exe
Emulators\ruffle\ruffle.exe
Emulators\ryujinx\Ryujinx.exe
Emulators\ryujinx\Ryujinx.Ava.exe
Emulators\sameboy\sameboy.exe
Emulators\scummvm\scummvm.exe
Emulators\sdl2trs\sdl2trs64.exe
Emulators\SheepShaver\SheepShaver.exe
Emulators\SimCoupe\SimCoupe.exe
Emulators\simple64\simple64-gui.exe
Emulators\SkyEmu\SkyEmu.exe
Emulators\snes9x\snes9x-x64.exe
Emulators\solarus\solarus-run.exe
Emulators\SSF\SSF.exe
Emulators\Stella\64-bit\Stella.exe
Emulators\Supermodel\Supermodel.exe
Emulators\tic80\tic80.exe
Emulators\Triforce\DolphinWX.exe
Emulators\tsugaru\Tsugaru_CUI.exe
Emulators\VBA-M\visualboyadvance-m.exe
Emulators\VICE\x64sc.exe
Emulators\VICE\bin\x64sc.exe
Emulators\VICE\xplus4.exe
Emulators\VICE\bin\xplus4.exe
Emulators\VICE\xvic.exe
Emulators\VICE\bin\xvic.exe
Emulators\VPinballX\VPinballX_GL64.exe
Emulators\VPinballX\VPinballX64.exe
Emulators\Vita3K\Vita3K.exe
Emulators\WinArcadia\WinArcadia.exe
Emulators\xemu\xemu.exe
Emulators\xenia\xenia.exe
Emulators\xenia_canary\xenia_canary.exe
Emulators\XM6 Pro-68k\XM6.exe
Emulators\xroar\xroar.exe
Emulators\yabasanshiro\yabasanshiro.exe
Emulators\ZEsarUX\zesarux.exe
