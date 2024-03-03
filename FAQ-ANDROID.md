# ES-DE (EmulationStation Desktop Edition) - Frequently Asked Questions for Android

In addition to this document it's a good idea to read the [Android documentation](ANDROID.md) and the [User guide](USERGUIDE.md).

## Is this the same application as the releases for Linux, macOS and Windows?

Yes it's the exact same application, with only some minor differences. This means that it behaves exactly as you're used to from those other platforms, and it means that you can transfer files between your Android device and any other devices where you have ES-DE installed. This for example includes your game ROMs, gamelists, scraped media and custom collections.

## Why is it named ES-DE as in "Desktop Edition" if this is a release for a mobile operating system?

First it's branding, it would be very confusing to have different names for the same application when it's available cross-platform. Second, the _Desktop Edition_ part is now a legacy, nowadays instead think of the D as standing for _Deck, Droid_ or _Desktop_. The _EmulationStation Desktop Edition_ subtitle used on the splash screen is only temporary during a transition period, it will be removed.

## Is it available for free, and is it open source?

The Android release specifically is not free, it's a paid app available for purchase through [Patreon](https://www.patreon.com/es_de). And although approximately 99% of the app is open source there are some portions of the code that is closed source.

## Why is ES-DE not available on the Google Play store?

That is a question for Google. They are have done everything in their power to obstruct the release of ES-DE including constantly rejecting the releases without supplying meaningful explanations why, refusing to respond to tickets and emails and in general being incredibly difficult to deal with. So there are no further plans at attempting to publish ES-DE there.

## How do I update ES-DE when there is a new release

When a new release is available you will be sent a download link to the email address you used to sign up for Patreon, and we will post it directly on Patreon as well. You can use that link to download the latest version, sideload it on your device and then update. We will explore other options to make this easier in the future.

## Can I use ES-DE on more than a single Android device or do I need to buy it multiple times?

You only need to buy it once, and then you can use it on all your devices. There are no subscriptions or additional costs, you just buy it once. With that said we do appreciate if you want to support the project by keeping your paid Patreon subscription.

## ES-DE doesn't work on my device, can I get a refund?

Although the overwhelming majority of people have successfully got ES-DE to run correctly on their devices (assuming they are fulfilling the basic requirements of 64-bit Android 11 or later) there are some devices that have been problematic. These issues have only been observed on mobile phones as far as we know, and is likely due to customizations done by the manufacturers. Unfortunately Android is not really a standardized operating system and different vendors are applying custom patches and such which may prevent ES-DE from working. We will refund everyone that is unable to get ES-DE to run on their device, just send a DM on Patreon and we will issue a refund as soon as possible.

## Why do I get a "There was a problem parsing the package" error when I attempt to install ES-DE?

There are two possible reasons for this, the first and most common issue is that your device does not fulfill the basic requirements for ES-DE, which is that it has to run a 64-bit version of Android 11 or later. We are investigating whether Android 10 support could be added in a future release but for the time being this is not supported.

The second reason is that the APK is corrupt or not complete. When we make releases we include an MD5 hash value with the download link, and it's recommended to check the hash of the downloaded file before you attempt to install it. This will also ensure that you actually have the real release and not some third party scam or fake app or similar. A recommended app for generating the MD5 checksums is [Hash Checker](https://play.google.com/store/apps/details?id=com.smlnskgmail.jaman.hashchecker) from the Google Play store.

## Can I set ES-DE as my home app/launcher?

Kind of, ES-DE can't natively be set as your launcher but you can use a third party app to make this work such as [AnyHome](https://play.google.com/store/apps/details?id=com.draco.anyhome&hl=en_US&gl=US). It's not necessarily recommended though as ES-DE is not a native Android application, it's written in C++ and essentially works as a game engine with a game loop that constantly runs. For this reason it consumes more resources and battery than a native launcher app. An alternative approach would be to use a native launcher with ES-DE, this makes for a nice game console experience. To achieve this the following app is recommended:
https://play.google.com/store/apps/details?id=com.k2.consolelauncher

## Can I launch Android applications and games from inside ES-DE?

Not at the moment. Although there is an _android_ system in ES-DE it's not in use at the moment, but this functionality is planned for a future release.

## What game systems/platforms and emulators are supported by ES-DE?

See the _Supported game systems_ section at the bottom of the [Android documentation](ANDROID.md#supported-game-systems) where there's a table listing all supported systems/platforms and emulators.

## When I launch a game using RetroArch I just see a black screen, what is wrong?

RetroArch on Android is very unforgiving, if you haven't installed the necessary core or BIOS files it's a high chance that you just see a black screen and it will hang there, possibly until you kill it. And due to the security model in Android it's not possible for ES-DE to check if a core is actually installed prior to attempting to launch RetroArch (on Linux, macOS and Windows a popup is shown if the core file is missing and the game is never actually launched in this case).

Also note that the RetroArch release on the Google Play store is not working correctly on some devices, it can be used on its own but game launching fails from ES-DE. These issues have been resolved by a number of people by instead switching to the release from the [RetroArch](https://retroarch.com) website.

## When I launch a game using a standalone emulator, why does it say the game file could not be opened?

Due to the scoped storage permissions in Android many emulators need to be given access to the specific game system directory where you store your ROMs. For example DuckStation needs access to your ROMs/psx directory and M64Plus FZ needs access to ROMs/n64. Some emulators support the FileProvider API in which case ES-DE will provide the emulator with the necessary access and it therefore does not need to be setup upfront in the emulator. You can see in the [Android documentation](ANDROID.md#emulation-on-android-in-general) which emulators support the FileProvider API.

Another reason for why it may not work is that the ROMs directories are in the wrong letter case. If you are reusing an old setup from another frontend where the directories are named for instance ROMs/PS2 and ROMs/C64 then these will actually get populated inside ES-DE but you won't be able to run any games with some specific emulators. So make sure that your game system directories are in lowercase such as ROMs/ps2 and ROMs/c64 and it should work fine. After changing a directory from uppercase to lowercase you'll also need to go into the emulator and provide access to the renamed directory. If you start with an empty ROMs directory and you let ES-DE generate the game system folders then all game system directories will be correctly generated in lowercase, so this is the recommended approach. **Note that there appears to be a bug in the Android operating system so that the scoped storage directory picker is not correctly updating the URI if the letter case has changed. For this reason, always "go back" to the root of the storage device when selecting the scoped storage directory inside the emulator, then select the ROMs folder and then the game systems folder, such as ps2 or c64.**

There seems to be a third situation as well where some emulators apparently keep some residual configuration even after changing the ROM path, which makes game launching fail. In some cases it's been successful to clear the emulator settings completely and then add access to the ROM directory again. The easiest way to do this is to go into the Android Settings app, choose _Apps_, select the emulator you want to clear the settings for, open _Storage & cache_ and select _Clear storage_. Just make sure that the emulator has not placed savestates and similar data on internal storage, as this might otherwise get lost. Following this open the emulator and give access to the correct ROM/game system directory.

## Why do some standalone emulators fail to launch with "ERROR CODE -1" or just display a black screen?

ERROR CODE -1 is a general failure mode which could be caused by multiple things. Some emulators react like this when there's a permission issue and they can't access the game file. See the previous question above for how to deal with such permission problems. And some emulators return this error when the file you attempt to launch has an unsupported file extension. For example MD.emu does not support .bin files, but if you rename these to the .gen extension then game launching works as expected.

If you have an Ayn Odin 2 this error will occur for a number of emulators such as Redream and My Boy! due to what seems to be a problem with their Android OS image. See the [Issues with the Ayn Odin 2](ANDROID.md#issues-with-the-ayn-odin-2) section in the Android documentation for more details about the latter.

The black screen on game launch is just a variation of this failure mode, it depends on how the emulator handles errors whether there will be a black screen or whether it will abort and report the launch failure to ES-DE.

## Sometimes after I return from a game ES-DE is restarting, did it crash?

No Android may stop applications that are not currently focused if it needs to recover the RAM that those applications were using. This is an integral part of the operating system design, there's really no way to prevent this behavior. ES-DE is a complex application that is more akin to a game engine than a regular Android application, and if you are using a demanding theme with lots of game media it can consume quite a lot of memory. If you are using a device with limited RAM, say 4 GB or so, it's almost unavoidable that ES-DE will get stopped if you are running an emulator that also uses a lot of memory. You could switch to a more lightweight theme though, this sometimes prevents these restarts from occuring.

## What type of Android devices are supported

ES-DE runs on a wide range on devices, for example handheld consoles like the Ayn Odin 2 and the Retroid Pocket 2s, 3 and 4, on mobile phones and on tablets. It supports a wide range of screen resolutions and aspect ratios. Android 11 or later is required.

## Will touch gesture support get added?

ES-DE is not a good match for gesture navigation, there are a lot of contextual navigation and quite a number of possible actions at any given time. So it would probably get quite confusing to attempt to learn gestures for all these actions. For this reason it was decided to instead implement a touch overlay with virtual buttons. This is also what most emulators use for their input so ES-DE will match the emulators in this regard. However, if you are using a controller with ES-DE then none of this applies and you can go ahead and disable the touch overlay from the _Input device settings_ menu.

## Why does ES-DE need to have permissions to manage my storage?

ES-DE needs the storage permissions (MANAGE_EXTERNAL_STORAGE) to manage storage because it's well.. a storage manager. It handles your library of games and media which can easily be tens or even hundreds of thousands of files, and this may span across multiple storage devices as for instance scraped media could be relocated to an SD card or the ROMs moved to another device than the application data directory. During development substantial work was spent on attempting to work around the Android security model without having storage manager permission. Although this was partially successful it never worked 100% due to additional restrictions introduced in Android 13. The approach was to pipe file operations from the native C++ code via JNI to Java and translate them to SAF/MediaStore file operations. And be aware that ES-DE depends on a large amount of C and C++ libraries that have no awareness of or support for the Storage Access Framework or Media Store subsystems in Android. Even if it would have worked 100% this would have lead to unacceptable performance issues as a lot of translations were needed, for instance Java uses "filtered" UTF8 Unicode while C++ doesn't so everything would need to be translated by the UTF8-CPP library. In addition to this an undocumented feature of the FileProvider API is that you can't provide access to files you don't own, even if you have read/write access to them using scoped storage permissions. As the FileProvider API is the future of emulator launching since it removes the need to setup scoped storage access individually in each emulator, it would break a very important feature to not have storage manager access. There are more issues on top of what has just been described, but these are the most important considerations.