EmulationStation Desktop Edition (ES-DE) - Portable installation on Windows
---------------------------------------------------------------------------

ES-DE release:
2.2.0-beta

The latest version can be downloaded from https://es-de.org
Please also consider donating to the project, links for that can be found on the ES-DE website just mentioned.

Instructions:

If upgrading from a previous release, then don't unpack this ZIP archive on top of your old installation, instead follow the upgrade instructions below.

New installation:
1) The ROMs_ALL directory contains all the systems that ES-DE supports, but to decrease application startup time only copy the folders you need to the ROMs directory
2) Place your games into their respective folders in the ROMs directory tree
3) Place your emulators inside the Emulators directory
4) Start ES-DE using EmulationStation.exe and enjoy some retrogaming!

Upgrading from an older release:
1) Rename your old EmulationStation-DE directory, for example to EmulationStation-DE_OLD
2) Move your games from EmulationStation-DE_OLD\ROMs\ to EmulationStation-DE\ROMs\
3) Move your emulators from EmulationStation-DE_OLD\Emulators\ to EmulationStation-DE\Emulators\
4) Move the contents of EmulationStation-DE_OLD\.emulationstation\ to EmulationStation-DE\.emulationstation\
   This last step includes your settings, custom collections, custom systems, scraped/downloaded media, gamelist.xml files, scripts and themes

In case of issues, check .emulationstation\es_log.txt for clues as to what went wrong.
Starting EmulationStation.exe with the --debug flag will provide additional details.

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
Emulators\AppleWin\AppleWin.exe
Emulators\ArcadeFlashWeb\ArcadeFlashWeb.exe
Emulators\ares\ares.exe
Emulators\atari800\atari800.exe
Emulators\BasiliskII\BasiliskII.exe
Emulators\BigPEmu\BigPEmu.exe
Emulators\bsnes\bsnes.exe
Emulators\cemu\Cemu.exe
Emulators\Citra\canary-mingw\citra-qt.exe
Emulators\Citra\nightly-mingw\citra-qt.exe
Emulators\cpcemu\cpcemu.exe
Emulators\CSpect\CSpect.exe
Emulators\Cxbx-Reloaded\cxbx.exe
Emulators\Dolphin-x64\Dolphin.exe
Emulators\dosbox-staging\dosbox.exe
Emulators\DOSBox-X\dosbox-x.exe
Emulators\duckstation\duckstation-nogui-x64-ReleaseLTCG.exe
Emulators\duckstation\duckstation-qt-x64-ReleaseLTCG.exe
Emulators\EasyRPG\Player.exe
Emulators\fbneo\fbneo64.exe
Emulators\fbneo\fbneo.exe
Emulators\flycast\flycast.exe
Emulators\Fuse\fuse.exe
Emulators\Future Pinball\Future Pinball.exe
Emulators\gargoyle\gargoyle.exe
Emulators\Gearboy\Gearboy.exe
Emulators\gopher2600\gopher2600_windows_amd64.exe
Emulators\hatari\hatari.exe
Emulators\Hypseus Singe\hypseus.exe
Emulators\KEmulator\KEmulator.exe
Emulators\m2emulator\EMULATOR.EXE
Emulators\mame\mame.exe
Emulators\mednafen\mednafen.exe
Emulators\melonDS\melonDS.exe
Emulators\Mesen\Mesen.exe
Emulators\mGBA\mGBA.exe
Emulators\mupen64plus\mupen64plus-ui-console.exe
Emulators\openMSX\openmsx.exe
Emulators\Oricutron\oricutron.exe
Emulators\PCSX2\pcsx2.exe
Emulators\PCSX2-Qt\pcsx2-qt.exe
Emulators\PICO-8\pico8.exe
Emulators\Play\Play.exe
Emulators\PPSSPP\PPSSPPWindows64.exe
Emulators\prboom-plus\prboom-plus.exe
Emulators\PrimeHack\Dolphin.exe
Emulators\Project64\Project64.exe
Emulators\punes\punes64.exe
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
Emulators\snes9x\snes9x-x64.exe
Emulators\solarus\solarus-run.exe
Emulators\SSF\SSF.exe
Emulators\Stella\64-bit\Stella.exe
Emulators\Supermodel\Supermodel.exe
Emulators\Triforce\DolphinWX.exe
Emulators\tsugaru\Tsugaru_CUI.exe
Emulators\VBA-M\visualboyadvance-m.exe
Emulators\VICE\x64sc.exe
Emulators\VICE\bin\x64sc.exe
Emulators\VICE\xvic.exe
Emulators\VICE\bin\xvic.exe
Emulators\Visual Pinball\VPinballX.exe
Emulators\Vita3K\Vita3K.exe
Emulators\xemu\xemu.exe
Emulators\xenia\xenia.exe
Emulators\xenia_canary\xenia_canary.exe
Emulators\xroar\xroar.exe
Emulators\yuzu\yuzu-windows-msvc\yuzu.exe
Emulators\ZEsarUX\zesarux.exe
