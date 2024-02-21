# ES-DE (EmulationStation Desktop Edition) - Frequently Asked Questions for Android

## Is this the same application as the releases for Linux, macOS and Windows?

Yes it's the exact same application, with only some minor differences. This means that it behaves exactly as you're used to from those other platforms, and it means that you can transfer files between your Android device and any other devices where you have ES-DE installed. This for example includes your game ROMs, gamelists, scraped media and custom collections.

## Why is it named ES-DE as in "Desktop Edition" if this is a release for a mobile operating system?

First it's branding, it would be very confusing to have different names for the same application when it's available cross-platform. Second, the _Desktop Edition_ part is now a legacy, nowadays instead think of the D as standing for _Deck, Droid_ or _Desktop_. The _EmulationStation Desktop Edition_ subtitle used on the splash screen is only temporary during a transition period, it will be removed.

## Is it available for free, and is it open source?

The Android release specifically is not free, it's a paid app available for purchase on the [Amazon Appstore](https://www.amazon.com/dp/B0CVXRHWTT). And although approximately 99% of the app is open source there are some portions of the code that is closed source.

## Why is ES-DE not available on the Google Play store?

That is a question for Google. They are have done everything in their power to obstruct the release of ES-DE including constantly rejecting the releases without supplying meaningful explanations why, refusing to respond to tickets and emails and in general being incredibly difficult to deal with. There's a chance that there will be a Play Store release in the future but there's also a chance that it never happens.

## How do I update ES-DE when there is a new release

You update ES-DE via the Amazon Appstore app. This works as expected, whenever a new release becomes available you can download and install it via this app.

## Can I use ES-DE on more than a single Android device or do I need to buy it multiple times?

You only need to buy it once, and then you can use it on all your devices, as long as you have the Amazon Appstore app installed on all these devices. There are no subscriptions or additional costs, you just buy it once.

## Can I set ES-DE as my launcher?

Kind of, ES-DE can't natively be set as your launcher but you can use a third party app to make this work. It's not necessarily recommended though as ES-DE is not a native Android application, it's written in C++ and essentially works as a game engine with a game loop that constantly runs. For this reason it consumes more resources and battery than a native launcher app. An alternative approach would be to use a native launcher with ES-DE, this makes for a nice game console experience. To achieve this the following app is recommended:
https://play.google.com/store/apps/details?id=com.k2.consolelauncher

## Can I launch Android applications and games from inside ES-DE?

Not at the moment. Although there is an _android_ system in ES-DE it's not in use at the moment, but this functionality is planned for a future release.

## What game systems/platforms and emulators are supported by ES-DE?

See the _Supported game systems_ section at the bottom of the [Android documentation](ANDROID.md#supported-game-systems) where there's a table listing all supported systems/platforms and emulators.

## When I launch a game using RetroArch I just see a black screen, what is wrong?

RetroArch on Android is very unforgiving, if you haven't installed the necessary core or BIOS files it's a high chance that you just see a black screen and it will hang there, possibly until you kill it. And due to the security model in Android it's not possible for ES-DE to check if a core is actually installed prior to attempting to launch RetroArch (on Linux, macOS and Windows a popup is shown if the core file is missing and the game is never actually launched).

## When I launch a game using a standalone emulator, why does it say the game file could not be opened?

Due to the scoped storage permissions in Android many emulators need to be given access to the specific game system directory where you store your ROMs. For example DuckStation would need access to your ROMs/psx directory. Some emulators support the FileProvider API in which case ES-DE will provide the emulator with the necessary access and it therefore does not need to be setup upfront in the emulator. You can see in the [Android documentation](ANDROID.md#emulation-on-android-in-general) which emulators support the FileProvider API.

## What type of Android devices are supported

ES-DE runs on a wide range on devices, for example handheld consoles like the Ayn Odin 2 and the Retroid Pocket 2s, 3 and 4, on mobile phones and on tablets. It supports a wide range of screen resolutions and aspect ratios. Android 11 or later is required.

## Will touch gesture support get added?

ES-DE is not a good match for gesture navigation, there are a lot of contextual navigation and quite a number of possible actions at any given time. So it would probably get quite confusing to attempt to learn gestures for all these actions. For this reason it was decided to instead implement a touch overlay with virtual buttons. This is also what most emulators use for their input so ES-DE will match the emulators in this regard. However, if you are using a controller with ES-DE then none of this applies and you can go ahead and disable the touch overlay from the _Input device settings_ menu.

## Why does ES-DE need to have permissions to manage my storage?

ES-DE needs the storage permissions (MANAGE_EXTERNAL_STORAGE) to manage storage because it's well.. a storage manager. It handles your library of games and media which can easily be tens or even hundreds of thousands of files, and this may span across multiple storage devices as for instance scraped media could be relocated to an SD card or the ROMs moved to another device than the application data directory. During development substantial work was spent on attempting to work around the Android security model without having storage manager permission. Although this was partially successful it never worked 100% due to additional restrictions introduced in Android 13. The approach was to pipe file operations from the native C++ code via JNI to Java and translate them to SAF/MediaStore file operations. And be aware that ES-DE depends on a large amount of C and C++ libraries that have no awareness of or support for the Storage Access Framework or Media Store subsystems in Android. Even if it would have worked 100% this would have lead to unacceptable performance issues as a lot of translations were needed, for instance Java uses "filtered" UTF8 Unicode while C++ doesn't so everything would need to be translated by the UTF8-CPP library. In addition to this an undocumented feature of the FileProvider API is that you can't provide access to files you don't own, even if you have read/write access to them using scoped storage permissions. As the FileProvider API is the future of emulator launching since it removes the need to setup scoped storage access individually in each emulator, it would break a very important feature to not have storage manager access. There are more issues on top of what has just been described, but these are the most important considerations.