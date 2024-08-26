# ES-DE Frontend - Haiku documentation

Note that support for Haiku is currently experimental as the operating system itself is experimental.

There are currently no pre-built packages available so you'll need to build ES-DE yourself. Detailed instructions are available in the _Building on Haiku_ section of the [INSTALL-DEV.md](INSTALL-DEV.md#building-on-haiku) document.

## Known problems

* Attempting to use the theme downloader crashes the application when using a nightly Haiku build, this is seemingly caused by an OS or libgit2 library bug as ES-DE runs correctly in R1/beta4 (but with other issues on that release)

* The video player behaves erratic and videos may randomly hang or refuse to play

* Key repeat doesn't work in text editing fields (but it works elsewhere in the application)

* There is no 3D acceleration as the operating system does not support that

## Emulator problems

In contrast with all other platforms which ES-DE runs on, on Haiku emulators which are not working correctly are still included in the configuration. This is done with the belief that things will improve in the future as the operating system matures.

**FS-UAE (Standalone)**

This emulator does not seem to accept command-line arguments, meaning games can't be launched from ES-DE.

**MAME (Standalone)**

When existing a game the OS screen resolution is sometimes not reset back to its previous state meaning it has to be manually reset to the correct resolution using the operating system's _Screen_ utility.

**PUAE**

Crashes on game start.

**ScummVM (Standalone)**

Games can only be launched if ES-DE has been started from the command line, i.e. from a _Terminal_ window.

## Supported game systems

The **@** symbol indicates that the emulator is _deprecated_ and will be removed in a future ES-DE release.

| System name           | Full name                                      | Default emulator                  | Alternative emulators             | Needs BIOS   | Recommended game setup               |
| :-------------------- | :--------------------------------------------- | :-------------------------------- | :-------------------------------- | :----------- | :----------------------------------- |
| 3do                   | 3DO Interactive Multiplayer                    | Opera                             |                                   | Yes          |                                      |
| adam                  | Coleco Adam                                    | MAME [Diskette] **(Standalone)**  | MAME [Tape] **(Standalone)**,<br>MAME [Cartridge] **(Standalone)**,<br>MAME [Software list] **(Standalone)** | Yes          |                                      |
| ags                   | Adventure Game Studio Game Engine              | _Placeholder_                     |                                   |              |                                      |
| amiga                 | Commodore Amiga                                | PUAE                              | FS-UAE **(Standalone)**           | Yes          | See the specific _Commodore Amiga and CDTV_ section in the user guide |
| amiga1200             | Commodore Amiga 1200                           | PUAE                              | FS-UAE **(Standalone)**           | Yes          | See the specific _Commodore Amiga and CDTV_ section in the user guide |
| amiga600              | Commodore Amiga 600                            | PUAE                              | FS-UAE **(Standalone)**           | Yes          | See the specific _Commodore Amiga and CDTV_ section in the user guide |
| amigacd32             | Commodore Amiga CD32                           | PUAE                              | FS-UAE **(Standalone)**           | Yes          | See the specific _Commodore Amiga and CDTV_ section in the user guide |
