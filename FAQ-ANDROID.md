# ES-DE Frontend - Frequently Asked Questions for Android

In addition to this document it's a good idea to read the [Android documentation](ANDROID.md) and the [User guide](USERGUIDE.md).

## Is this the same application as the releases for Linux, macOS and Windows?

Yes it's the exact same application, with only some minor differences. This means that it behaves exactly as you're used to from those other platforms, and it means that you can transfer files between your Android device and any other devices where you have ES-DE installed. This for example includes your game ROMs, gamelists, scraped media and custom collections.

## Why is it named ES-DE as in "Desktop Edition" if this is a release for a mobile operating system?

First it's branding, it would be very confusing to have different names for the same application when it's available cross-platform. Second, the _Desktop Edition_ part is now basically legacy. Actually the entire _EmulationStation Desktop Edition_ subtitle used on the splash screen is only temporary during a transition period and it will be removed at some point. The official name of the project is already ES-DE Frontend or ES-DE for short.

## Is it available for free, and is it open source?

The Android release specifically is not free, it's a paid app available for purchase through [Patreon](https://www.patreon.com/es_de), the [Samsung Galaxy Store](https://galaxystore.samsung.com/detail/org.es_de.frontend.galaxy) and [Huawei AppGallery](https://appgallery.huawei.com/#/app/C111315115). And although the majority of the code is open source there is some Android-specific code that is copyrighted and closed source.

## I bought ES-DE on Patreon, how do I get access to future releases?

When a new release is available you will be sent a download link to the email address you used to sign up for Patreon. Note that if you pay once on Patreon and cancel your paid membership you'll get one month of access to all posts and content, and when this month has passed you will no longer have access. This is how the Patreon platform works, and it's the reason why updates are distributed via email. As indicated in the welcome message when you join the ES-DE Patreon it's a good idea to save the download link so you have it available if you need to download the APK again. It's also possible to resend the latest update email to yourself using our self-service tool which can be found at https://resend.es-de.org

## Can I use ES-DE on more than a single Android device or do I need to buy it multiple times?

You only need to buy the Patreon release once, and then you can use it on all your devices. There are no subscriptions or additional costs, you just buy it once. With that said we do appreciate if you want to support the project by keeping your paid Patreon subscription. The Samsung Galaxy Store and Huawei AppGallery releases may not be available on all your devices, but that is not an ES-DE restriction but rather governed by the availability of these app stores on your different Android devices.

## ES-DE doesn't work on my device, can I get a refund?

Although the overwhelming majority of people have successfully got ES-DE to run on their devices (assuming they are fulfilling the basic requirements of 64-bit Android 10 or later) there are some devices that have been problematic. Unfortunately Android is not really a standardized operating system and hardware manufacturers are sometimes applying custom patches and such which may prevent ES-DE from working correctly. We will refund anyone that bought ES-DE on Patreon within one month from the purchase date if they are unable to get ES-DE to run on their device. Just send a DM on Patreon and we will issue a refund as soon as possible. We are unfortunately not able to directly refund purchases on the Samsung Galaxy Store and Huawei AppGallery, but you may still be able to get a refund by contacting Samsung or Huawei. Anyway, make sure to read the next question below as your device may be compatible after all.

## ES-DE hangs at the onboarding configurator, is the app not compatible with my device?

There are some Android developer options that break ES-DE (and probably many other apps too) so make sure to never change such settings unless you know exactly what you are doing. For instance the option _Don't keep activities_ will make the configurator hang so that you'll never be able to get past the onboarding step.

## I received an update email to my Gmail account but the APK download link doesn't seem to work?

There seems to be an issue with Gmail (both web version and app) that a few people have run into. Clicking on some external download links will result in extremely slow downloads or the download failing completely. The solution is to copy the download link from the email and paste it into the address bar of a web browser, i.e. outside of the Gmail web interface or outside the Gmail app. That should resolve the issue and the APK should download correctly.

## Will I lose any settings or data when upgrading to a new release?

No, you will not lose any settings or data when you upgrade, everything will stay intact.

## Why do I get a "There was a problem parsing the package" error when I attempt to install ES-DE?

There are two possible reasons for this, the first and most common issue is that your device does not fulfill the basic requirements for ES-DE, which is that it has to run a 64-bit version of Android 10 or later.

The second reason is that the APK is corrupt or not complete. When we make releases we include an MD5 hash value with the download link, and it's recommended to check the hash of the downloaded file before you attempt to install it. This will also ensure that you actually have the real release and not some third party scam or fake app or similar. A recommended app for generating the MD5 checksums is [Hash Checker](https://play.google.com/store/apps/details?id=com.smlnskgmail.jaman.hashchecker) from the Google Play store.

## Can I set ES-DE as my home app/launcher?

Yes, read the _Running ES-DE as the Android home app_ section of the [Android documentation](ANDROID.md#running-es-de-as-the-android-home-app) for more information about this functionality.

## Can I launch Android apps and games from inside ES-DE?

Yes, as of ES-DE 3.0.2 there is experimental support for launching native Android apps and games. Read the _Launching native Android apps and games_ section of the [Android documentation](ANDROID.md#launching-native-android-apps-and-games) to see how this is accomplished. As you can read there a separate app is needed to import your games into ES-DE, but this functionality will be built into ES-DE itself in a future release.

## Every time I reboot my device ES-DE is starting the onboarding process, why is this happening?

If you have set ES-DE as your home app then for some devices the onboarding configurator is displayed after booting your device. This happens because of an issue in the Android operating system where apps are started before the SD card has been mounted. When ES-DE starts it will obviously try to access the ES-DE and ROMs directories that it needs to function, but if either of these have been placed on an SD card that is not available, then the application assumes that the storage has been permanently removed and runs the onboarding process again. This is normal and intended behavior. On app startup ES-DE will however check if the SD card has been mounted, and it will wait up to 4.5 seconds for the storage to become available before it gives up and displays the configurator. For the overwhelming amount of cases this time is enough to handle reboots without issues, but some SD cards of larger sizes apparently need more time than this to get mounted, which triggers the failure mode. Note that you don't need to run through the entire onboarding process if this happens, it's enough to just press B or the back button, just make sure to wait a sufficient amount of time for the SD card to first get mounted. Unfortunately this issues is impossible to resolve on the application layer, it's an operating system defect and it needs to be fixed by Google. Setting a higher retry time than 4.5 seconds will make Android report ES-DE as non-responding, so that's unfortunately not a viable solution either.

## Can I use ES-DE with Samsung DeX?

While ES-DE would in theory work fine with DeX this is unfortunately not supported as Samsung has a policy to not allow apps that can be set as home apps/launchers to run via DeX. This is a Samsung limitation that they would need to resolve.

## What game systems/platforms and emulators are supported by ES-DE?

See the _Supported game systems_ section at the bottom of the [Android documentation](ANDROID.md#supported-game-systems) where there's a table listing all supported systems/platforms and emulators.

## Can I split my game system directories across multiple storage devices?

Yes but this is not recommended. It's tedious to setup and not how ES-DE is intended to be used. If you still insist on doing it you can read the _Splitting system directories across multiple storage devices_ section in the [Android documentation](ANDROID.md#splitting-system-directories-across-multiple-storage-devices).

## When I launch a game using RetroArch I just see a black screen, what is wrong?

RetroArch on Android is very unforgiving, if you haven't installed the necessary core or BIOS files it's a high chance that you just see a black screen and it will hang there, possibly until you kill it. And due to the security model in Android it's not possible for ES-DE to check if a core is actually installed prior to attempting to launch RetroArch (on Linux, macOS and Windows a popup is shown if the core file is missing and the game is never actually launched in this case). Also make sure that the core you have installed in RetroArch is the one you actually use in ES-DE. You can select between different cores and emulators for most systems using the _Alternative emulators_ interface in the _Other settings_ menu.

Another reason for the black screen is if you have multiple users configured on your device and attempt to run RetroArch from a non-primary user while having your ROMs on internal storage. At the time of writing RetroArch does not support external game launching for any other user than the primary user as it can't parse paths such as /storage/emulated/10/.

Also note that the RetroArch release on the Google Play store is not working correctly on most devices. It can be used on its own but game launching fails from ES-DE. These issues are resolved by using a current release from the [RetroArch](https://retroarch.com) website.

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

Unfortunately disk I/O performance on Android is not on par with desktop operating systems. Google has prioritized other things over performance which leads to disk speed being poor overall on this operating system. The main issue is the choice of FAT filesystems such as exFAT for external storage which offer very poor performance for some file operations on which ES-DE relies heavily. The SAF/MediaStore layer also adds a lot of overhead. Generally speaking a small to medium ROM collection can be placed on a FAT-formatted device such as an SD card, but it's recommended to place the ES-DE directory and more importantly the _downloaded_media_ directory on internal storage. For large game collections ES-DE could turn borderline unusable if the _downloaded_media_ directory is placed on an SD card or on a USB memory stick. This seems to be quite device-dependent though and on some devices performance is still acceptable, so you'll need to test it.

One possible improvement to startup times is to enable the _Only show games from gamelist.xml files_ option in the _Other settings_ menu to skip checking for game files on startup, but this has multiple implications such as what's displayed inside ES-DE not necessarily reflecting reality any longer. And obviously you'll need gamelist.xml entries for all games you want to show up inside ES-DE. So this option is really a last resort and is generally only recommended for testing purposes.

Another option that could speed up startup times under some circumstances is disabling the _Enable theme variant triggers_ setting in the _UI settings_ menu. But whether this has a tangible effect depends on the theme used and to what extent there is scraped media available for your game systems.

Finally, if you keep directories containing texture packs and similar inside your game system folders then these can slow down the startup considerably. To exclude scanning of any such directory you can place a file named `noload.txt` inside the folder and it will get completely skipped on startup.

In summary huge game collections are discouraged on Android due to limitations in the operating system itself. Setting up a collection of tens of thousands of games is for sure achievable with ES-DE on Linux, macOS or Windows but it's not really feasible on Android.

## On game launch the emulator runs an old game instead of the one I just selected, how do I prevent this?

You need to exit the game every time you stop playing, by doing this everything will work correctly. Pressing the home button or manually navigating back to ES-DE without exiting the game is equivalent to pressing "Alt+tab" on a desktop operating system, i.e. the game will still run. The difference from desktop operating systems is that Android pauses the game if you switch away from its window so it may seem like it has closed down, although it actually hasn't. While the procedure to fully exit a game differs between emulators there's a video on the official ES-DE YouTube channel on how to configure RetroArch correctly so that it quits completely when you're exiting a game:\
https://www.youtube.com/watch?v=k5WWacfIn6Y

## What type of Android devices are supported

ES-DE runs on a wide range on devices, for example handheld consoles like the Ayn Odin and the Retroid Pocket models, on mobile phones, on tablets and on Android TV devices like the Nvidia Shield Pro. It supports a wide range of screen resolutions and aspect ratios. A 64-bit version of Android 10 or later is required.

## Can I use the Android soft keyboard to enter text using touch and swiping

Yes this is somehow possible, by disabling the _Enable virtual keyboard_ setting in the _UI settings_ menu the soft keyboard built into Android can be used. There is however a bug in the SDL library that causes rendering issues in ES-DE when combining this with controller input. More specifically a green border is shown around the screen and the bottom portion of the screen is not getting rendered at all. If this happens you need to restart ES-DE, and for this reason the ES-DE built-in virtual keyboard is enabled by default. If you are however only using touch input with ES-DE then using the Android soft keyboard should work fine.

## Will touch gesture support get added?

ES-DE is not a good match for gesture navigation, there are a lot of contextual navigation and quite a number of possible actions at any given time. So it would probably get quite confusing to attempt to learn gestures for all these actions. For this reason it was decided to instead implement a touch overlay with virtual buttons. This is also what most emulators use for their input so ES-DE will match the emulators in this regard. However, if you are using a controller with ES-DE then none of this applies and you can go ahead and disable the touch overlay from the _Input device settings_ menu.

## Why does ES-DE need to have permissions to manage my storage?

ES-DE needs the storage permissions (MANAGE_EXTERNAL_STORAGE) to manage storage because it's well.. a storage manager. It handles your library of games and media which can easily be tens or even hundreds of thousands of files, and this may span across multiple storage devices as for instance scraped media could be relocated to an SD card or the ROMs moved to another device than the application data directory. During development substantial work was spent on attempting to work around the Android security model without having storage manager permission. Although this was partially successful it never worked 100% due to additional restrictions introduced in Android 13. The approach was to pipe file operations from the native C++ code via JNI to Java and translate them to SAF/MediaStore file operations. And be aware that ES-DE depends on a large amount of C and C++ libraries that have no awareness of or support for the Storage Access Framework or Media Store subsystems in Android. Even if it would have worked 100% this would have lead to unacceptable performance issues as a lot of translations were needed, for instance Java uses "filtered" UTF8 Unicode while C++ doesn't so everything would need to be translated by the UTF8-CPP library. In addition to this an undocumented feature of the FileProvider API is that you can't provide access to files you don't own, even if you have read/write access to them using scoped storage permissions. As the FileProvider API is very useful for emulator launching since it removes the need to setup scoped storage access individually in each emulator, it would break a very important feature to not have storage manager access. There are more issues on top of what has just been described, but these are the most important considerations.
