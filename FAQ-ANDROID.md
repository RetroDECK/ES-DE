# ES-DE Frontend - Frequently Asked Questions for Android

In addition to this document it's a good idea to read the [Android documentation](ANDROID.md) and the [User guide](USERGUIDE.md).

## Is this the same application as the releases for Linux, macOS and Windows?

Yes it's the exact same application, with only some minor differences. This means that it behaves exactly as you're used to from those other platforms, and it means that you can transfer files between your Android device and any other devices where you have ES-DE installed. This for example includes your game ROMs, gamelists, scraped media and custom collections.

## Why is it named ES-DE as in "Desktop Edition" if this is a release for a mobile operating system?

First it's branding, it would be very confusing to have different names for the same application when it's available cross-platform. Second, the _Desktop Edition_ part is now a legacy, nowadays instead think of the D as standing for _Deck, Droid_ or _Desktop_. The _EmulationStation Desktop Edition_ subtitle used on the splash screen is only temporary during a transition period, it will be removed.

## Is it available for free, and is it open source?

The Android release specifically is not free, it's a paid app available for purchase through [Patreon](https://www.patreon.com/es_de). And although approximately 99% of the app is open source there are some portions of the code that is closed source.

## I bought ES-DE on Patreon, how do I get access to future releases?

When a new release is available you will be sent a download link to the email address you used to sign up for Patreon. Note that if you pay once on Patreon and cancel your paid membership you'll get one month of access to all posts and content, and when this month has passed you will no longer have access. This is how the Patreon platform works, and it's the reason why updates are distributed via email. As indicated in the welcome message when you join the ES-DE Patreon it's a good idea to save the download link so you have it available if you need to download the APK again.

## Can I use ES-DE on more than a single Android device or do I need to buy it multiple times?

You only need to buy it once, and then you can use it on all your devices. There are no subscriptions or additional costs, you just buy it once. With that said we do appreciate if you want to support the project by keeping your paid Patreon subscription.

## ES-DE doesn't work on my device, can I get a refund?

Although the overwhelming majority of people have successfully got ES-DE to run on their devices (assuming they are fulfilling the basic requirements of 64-bit Android 10 or later) there are some devices that have been problematic. Unfortunately Android is not really a standardized operating system and hardware manufacturers are sometimes applying custom patches and such which may prevent ES-DE from working correctly. We will refund everyone up to one month from the purchase date if they are unable to get ES-DE to run on their device, just send a DM on Patreon and we will issue a refund as soon as possible.

## Will I lose any settings or data when upgrading to a new release?

No, you will not lose any settings or data when you upgrade. Just download the latest version and sideload it on your device to apply the update.

## Why do I get a "There was a problem parsing the package" error when I attempt to install ES-DE?

There are two possible reasons for this, the first and most common issue is that your device does not fulfill the basic requirements for ES-DE, which is that it has to run a 64-bit version of Android 10 or later.

The second reason is that the APK is corrupt or not complete. When we make releases we include an MD5 hash value with the download link, and it's recommended to check the hash of the downloaded file before you attempt to install it. This will also ensure that you actually have the real release and not some third party scam or fake app or similar. A recommended app for generating the MD5 checksums is [Hash Checker](https://play.google.com/store/apps/details?id=com.smlnskgmail.jaman.hashchecker) from the Google Play store.

## Can I set ES-DE as my home app/launcher?

Kind of, ES-DE can't natively be set as your launcher but you can use a third party app to make this work such as [AnyHome](https://play.google.com/store/apps/details?id=com.draco.anyhome&hl=en_US&gl=US). It's not necessarily recommended though as ES-DE is not a native Android application, it's written in C++ and essentially works as a game engine with a game loop that constantly runs. For this reason it consumes more resources and battery than a native launcher app. An alternative approach would be to use a native launcher with ES-DE, this makes for a nice game console experience. To achieve this the following app is recommended:
https://play.google.com/store/apps/details?id=com.k2.consolelauncher

## Can I launch Android apps and games from inside ES-DE?

Yes, as of ES-DE 3.0.2 there is experimental support for launching native Android apps and games. Read the _Launching native Android apps and games_ section of the [Android documentation](ANDROID.md#launching-native-android-apps-and-games) to see how this is accomplished. As you can read there a separate app is needed to import your games into ES-DE, but this functionality will be built into ES-DE itself in a future release.

## What game systems/platforms and emulators are supported by ES-DE?

See the _Supported game systems_ section at the bottom of the [Android documentation](ANDROID.md#supported-game-systems) where there's a table listing all supported systems/platforms and emulators.

## Can I split my game system directories across multiple storage devices?

Yes but this is not recommended. It's tedious to setup and not how ES-DE is intended to be used. If you still insist on doing it you can read the _Splitting system directories across multiple storage devices_ section in the [Android documentation](ANDROID.md#splitting-system-directories-across-multiple-storage-devices).

## When I launch a game using RetroArch I just see a black screen, what is wrong?

RetroArch on Android is very unforgiving, if you haven't installed the necessary core or BIOS files it's a high chance that you just see a black screen and it will hang there, possibly until you kill it. And due to the security model in Android it's not possible for ES-DE to check if a core is actually installed prior to attempting to launch RetroArch (on Linux, macOS and Windows a popup is shown if the core file is missing and the game is never actually launched in this case). Also make sure that the core you have installed in RetroArch is the one you actually use in ES-DE. You can select between different cores and emulators for most systems using the _Alternative emulators_ interface in the _Other settings_ menu.

Also note that the RetroArch release on the Google Play store is not working correctly on some devices, it can be used on its own but game launching fails from ES-DE. These issues have been resolved by a number of people by instead switching to the release from the [RetroArch](https://retroarch.com) website.

## When I launch a game using a standalone emulator, why does it say the game file could not be opened?

Due to the scoped storage permissions in Android many emulators need to be given access to the specific game system directory where you store your ROMs. For example DuckStation needs access to your ROMs/psx directory and M64Plus FZ needs access to ROMs/n64. Some emulators support the FileProvider API in which case ES-DE will provide the emulator with the necessary access and it therefore does not need to be setup upfront in the emulator. You can see in the [Android documentation](ANDROID.md#emulation-on-android-in-general) which emulators support the FileProvider API.

Another reason for why it may not work is that the ROMs directories are in the wrong letter case. If you are reusing an old setup from another frontend where the directories are named for instance ROMs/PS2 and ROMs/C64 then these will actually get populated inside ES-DE but you won't be able to run any games with some specific emulators. So make sure that your game system directories are in lowercase such as ROMs/ps2 and ROMs/c64 and it should work fine. After changing a directory from uppercase to lowercase you'll also need to go into the emulator and provide access to the renamed directory. If you start with an empty ROMs directory and you let ES-DE generate the game system folders then all game system directories will be correctly generated in lowercase, so this is the recommended approach. **Note that there appears to be a bug in the Android operating system so that the scoped storage directory picker is not correctly updating the URI if the letter case has changed. For this reason, always "go back" to the root of the storage device when selecting the scoped storage directory inside the emulator, then select the ROMs folder and then the game systems folder, such as ps2 or c64.**

There seems to be a third situation as well where some emulators apparently keep some residual configuration even after changing the ROM path, which makes game launching fail. In some cases it's been successful to clear the emulator settings completely and then add access to the ROM directory again. The easiest way to do this is to go into the Android Settings app, choose _Apps_, select the emulator you want to clear the settings for, open _Storage & cache_ and select _Clear storage_. Just make sure that the emulator has not placed savestates and similar data on internal storage, as this might otherwise get lost. Following this open the emulator and give access to the correct ROM/game system directory.

Also be aware that there are some slight variations when it comes to how emulators behave when they can't access the game files. In most cases an explicit error message is displayed that it can't open the file, but some emulators like M64Plus FZ will just display the emulator GUI instead.

## Why do some standalone emulators fail to launch with "ERROR CODE -1" or just display a black screen?

ERROR CODE -1 is a general failure mode which could be caused by multiple things. Some emulators react like this when there's a permission issue and they can't access the game file. See the previous question above for how to deal with such permission problems. And some emulators return this error when the file you attempt to launch has an unsupported file extension. For example MD.emu does not support .bin files, but if you rename these files to the .gen extension then game launching works as expected.

A black screen on game launch is also a possible variation of this failure mode, it depends on how the emulator handles errors whether there will be a black screen or whether it will abort and report the launch failure to ES-DE.

## Sometimes after I return from a game ES-DE is restarting, did it crash?

No Android may stop applications that are not currently focused if it needs to recover the RAM that those applications were using. This is an integral part of the operating system design, there's really no way to prevent this behavior. ES-DE is a complex application that is more akin to a game engine than a regular Android application, and if you are using a demanding theme with lots of game media it can consume quite a lot of memory. If you are using a device with limited RAM, say 4 GB or so, it's almost unavoidable that ES-DE will get stopped if you are running an emulator that also uses a lot of memory. You could switch to a more lightweight theme though, this sometimes prevents these restarts from occuring.

## ES-DE takes a very long time to start, is there a way to improve this?

Unfortunately disk I/O performance on Android leaves a lot to be desired compared to desktop operating systems. Google has repeatedly prioritized other things over performance which leads to disk speed being poor on this operating system. The biggest offender is the choice of FAT filesystems such as exFAT for external storage which offer abysmal performance for some file operations on which ES-DE relies heavily. Generally speaking a small to medium ROM collection can normally be placed on a FAT-formatted device such as an SD card but the ES-DE directory and more importantly the _downloaded_media_ directory should always be placed on internal storage. For large game collections ES-DE could turn borderline unusable if the ES-DE directory is placed on an SD card or USB memory stick. It's also possible to enable the _Only show games from gamelist.xml files_ option in the _Other settings_ menu to skip checking for game files on startup, but this has multiple implications such as what's displayed inside ES-DE not necessarily reflecting reality any longer. And obviously you'll need gamelist.xml entries for all games you want to show up inside ES-DE. So this option is really a last resort and is generally only recommended for testing purposes. In summary huge game collections are discouraged on Android due to limitations in the operating system itself. Setting up a collection of tens of thousands of games is for sure achievable with ES-DE on Linux, macOS or Windows but it's not really feasible on Android.

## On game launch RetroArch runs an old game instead of the one I just selected, how do I prevent this?

There is a video on the official ES-DE YouTube channel on how to configure RetroArch correctly so that it quits completely when you're exiting a game:\
https://www.youtube.com/watch?v=k5WWacfIn6Y

## What type of Android devices are supported

ES-DE runs on a wide range on devices, for example handheld consoles like the Ayn Odin and the Retroid Pocket models, on mobile phones, on tablets and on Android TV devices like the Nvidia Shield Pro. It supports a wide range of screen resolutions and aspect ratios. A 64-bit version of Android 10 or later is required.

## Can I use the Android soft keyboard to enter text using touch and swiping

Yes this is somehow possible, by disabling the _Enable virtual keyboard_ setting in the _UI settings_ menu the soft keyboard built into Android can be used. There is however a bug in the SDL library that causes rendering issues in ES-DE when combining this with controller input. More specifically a green border is shown around the screen and the bottom portion of the screen is not getting rendered at all. If this happens you need to restart ES-DE, and for this reason the ES-DE built-in virtual keyboard is enabled by default. If you are however only using touch input with ES-DE then using the Android soft keyboard should work fine.

## Will touch gesture support get added?

ES-DE is not a good match for gesture navigation, there are a lot of contextual navigation and quite a number of possible actions at any given time. So it would probably get quite confusing to attempt to learn gestures for all these actions. For this reason it was decided to instead implement a touch overlay with virtual buttons. This is also what most emulators use for their input so ES-DE will match the emulators in this regard. However, if you are using a controller with ES-DE then none of this applies and you can go ahead and disable the touch overlay from the _Input device settings_ menu.

## Why does ES-DE need to have permissions to manage my storage?

ES-DE needs the storage permissions (MANAGE_EXTERNAL_STORAGE) to manage storage because it's well.. a storage manager. It handles your library of games and media which can easily be tens or even hundreds of thousands of files, and this may span across multiple storage devices as for instance scraped media could be relocated to an SD card or the ROMs moved to another device than the application data directory. During development substantial work was spent on attempting to work around the Android security model without having storage manager permission. Although this was partially successful it never worked 100% due to additional restrictions introduced in Android 13. The approach was to pipe file operations from the native C++ code via JNI to Java and translate them to SAF/MediaStore file operations. And be aware that ES-DE depends on a large amount of C and C++ libraries that have no awareness of or support for the Storage Access Framework or Media Store subsystems in Android. Even if it would have worked 100% this would have lead to unacceptable performance issues as a lot of translations were needed, for instance Java uses "filtered" UTF8 Unicode while C++ doesn't so everything would need to be translated by the UTF8-CPP library. In addition to this an undocumented feature of the FileProvider API is that you can't provide access to files you don't own, even if you have read/write access to them using scoped storage permissions. As the FileProvider API is the future of emulator launching since it removes the need to setup scoped storage access individually in each emulator, it would break a very important feature to not have storage manager access. There are more issues on top of what has just been described, but these are the most important considerations.