# EmulationStation Desktop Edition (ES-DE) v2.0 - Themes

If creating theme sets specifically for ES-DE, please add `-es-de` to the repository/directory name, as in `slate-es-de`. Because ES-DE theme engine functionality has deviated greatly from the RetroPie EmulationStation fork on which it was originally based, any newer themes will not work on such older forks. At least the -es-de extension is an indicator that it's an ES-DE specific theme set. The actual theme name as defined using the `themeName` tag in capabilities.xml does of course not need to include the `-es-de` extension as that's the actual theme name that will be displayed when selecting theme sets from the _UI Settings_ menu. For example slate-es-de will be listed simply as _Slate_ in this menu.

Before your start, make sure to download the _Theme engine examples_ theme set that contains a number of example variants for things like vertical and horizontal carousels, wheel carousels, system view textlists, grids etc:

https://gitlab.com/es-de/themes/theme-engine-examples-es-de

To test whether your theme set includes support for all ES-DE systems, download one of the following archives which contain ROMs directory structures fully populated with dummy files:

[ROMs_ALL_Unix.zip](tools/system-dirs-dummy-files/ROMs_ALL_Unix.zip)\
[ROMs_ALL_macOS.zip](tools/system-dirs-dummy-files/ROMs_ALL_macOS.zip)\
[ROMs_ALL_Windows.zip](tools/system-dirs-dummy-files/ROMs_ALL_Windows.zip)

If you unzip and temporarily replace your ROMs directory with one of these, every system will be enabled on startup.

It's recommended to use a proper code editor for theme development, such as [VSCode](https://code.visualstudio.com) with the [Red Hat XML extension](https://github.com/redhat-developer/vscode-xml).

A general comment regarding SVG graphic files is that fonts are not supported by the LunaSVG library so these need to be converted to paths in order for them to get rendered inside ES-DE. In Inkscape the relevant command is named _Object to Path_ but there should be equivalent functionality in other vector graphics editors.

Another general remark is that Linux almost always uses case-sensitive file systems (that's sometimes true for macOS as well). Therefore it's a good idea to always name files with lowercase characters only. Also make sure to regularly test on Linux if that's not your primary operating system.

Table of contents:

[[_TOC_]]

## Introduction

ES-DE allows the grouping of themes for multiple game systems into a **theme set**. A theme is a collection of **elements**, each with their own **properties** that define the way they look and behave. These elements include things like text lists, carousels, images and animations.

Internally ES-DE uses the concept of **components** to actually implement the necessary building blocks to parse and render the elements, and although this is normally beyond the scope of what a theme author needs to consider, it's still good to be aware of the term as it's sometimes used in the documentation.

Every game system has its own subdirectory within the theme set directory structure, and these are defined in the systems configuration file `es_systems.xml` either via the optional `<theme>` tag, or otherwise via the mandatory `<name>` tag. When ES-DE populates a system on startup it will look for a file named `theme.xml` in each such directory.

By placing a theme.xml file directly in the root of the theme set directory, that file will be processed as a default if there is no system-specific theme.xml file available.

In the example below, we have a theme set named `mythemeset-es-de` which includes the `snes` and `nes` systems. Assuming you have some games installed for these systems, the files `mythemeset-es-de/nes/theme.xml` and `mythemeset-es-de/snes/theme.xml` will be processed on startup. If there are no games available for a system, its theme.xml file will be skipped.

The directory structure of our example theme set could look something like the following:

```
...
   themes/
      mythemeset-es-de/
         core/
            font.ttf
            bold_font.ttf
            frame.png

         nes/
            theme.xml
            background.jpg
            logo.svg
            logo_video.svg

         snes/
            theme.xml
            background.jpg
            logo.svg
            logo_video.svg

         fonts.xml
         theme.xml
```

The theme set approach makes it easy for users to install different themes and choose between them from the _UI Settings_ menu.

There are two places that ES-DE can load theme sets from:
* `[HOME]/.emulationstation/themes/[THEME_SET]/`
* `[INSTALLATION PATH]/themes/[THEME_SET]/`

An example installation path would be: \
`/usr/share/emulationstation/themes/slate-es-de/`

If a theme set with the same name exists in both locations, the one in the home directory will be loaded and the other one will be skipped.

## Differences to legacy RetroPie themes

If you are not familiar with theming for RetroPie or similar forks of EmulationStation you can skip this section as it only describes the key differences between the updated ES-DE themes and these _legacy_ theme sets. The term _legacy_ is used throughout this document to refer to this older style of themes which ES-DE still fully supports for backward compatibility reasons. The old theme format is described in [THEMES-LEGACY.md](THEMES-LEGACY.md) although this document is basically a historical artifact by now.

With ES-DE v2.0 a new theme engine was introduced that fundamentally changed some aspects of how theming works. The previous design used specific view styles (basic, detailed, video and grid) and this was dropped completely and replaced with _variants_ that can accomplish the same thing while being much more powerful and flexible.

In the past EmulationStation basically had hardcoded view styles with certain elements always being present and only a limited ability to manipulate these via positioning, resizing, coloring etc. As well so-called _extras_ were provided to expand theming support somehow but even this was quite limited.

With the new theme engine the view presets were removed and the only views now available, _system_ and _gamelist_, were rewritten to be much more flexible. Essentially the element selection and placement is now unlimited; any number of elements of any type can be used, although with a few notable exceptions as explained throughout this document.

In addition to _variants_, support for _color schemes_ and _aspect ratios_ was introduced. The former makes it possible to provide different color profiles via variable declarations, and the latter makes it possible to define different theme configurations for different display aspect ratios. That could for example be a choice between a 16:9 and a 4:3 layout, and perhaps also a vertical screen orientation layout. All these options are selectable via the _UI Settings_ menu.

New theming abilities like GIF and Lottie animations were also added to the new theme engine.

The NanoSVG rendering library has been replaced with [LunaSVG](https://github.com/sammycage/lunasvg) which greatly improves SVG file support as NanoSVG had issues with rendering quite some files. There might be some slight regressions with LunaSVG, but most of these are probably due to issues in NanoSVG that caused some non-conformant files to render seemingly correct. Make sure to compare any SVG files that don't seem to render correctly in ES-DE with what they look like if opened in for example Firefox or Chrome/Chromium.

As for more specific changes, the following are the most important ones compared to legacy themes:

* View styles are now limited to only _system_ and _gamelist_ (there is a special _all_ view style as well but that is only used for navigation sounds as explained later in this document)
* The hardcoded metadata attributes like _md_image_ and _md_developer_ are gone, but a new `<metadata>` property is available for populating views with metadata information
* The concept of _extras_ is gone as all element can now be used however the theme author wishes
* The concept of _features_ is gone
* The `<formatVersion>` tag is gone as tracking theme versions doesn't make much sense after all
* The `video` element properties `showSnapshotNoVideo` and `showSnapshotDelay` have been removed
* The ambiguous `alignment` property has been replaced with the `horizontalAlignment` and `verticalAlignment` properties (the same is true for `logoAlignment` for the `carousel` element)
* The `forceUppercase` property has been replaced with the more versatile `letterCase` property
* Many property names for the carousel have been renamed, with _logo_ being replaced by _item_ as this element can now be used in both the gamelist and system views. As well, setting the alignment will not automatically add any margins as is the case for legacy themes. These can still be set manually using the `horizontalOffset` and `verticalOffset` properties if needed. The way that alignment works in general for both carousel items and the overall carousel has also changed
* The rating elements were previously not sized and overlaid consistently, this has now been fixed and rating images should now be centered on the image canvas in order for this element to render correctly rather than being left-adjusted as has previously been done by some theme authors (likely as a workaround for the previous buggy implementation). Images of any aspect ratios are now also supported where previously only square images could be used
* The carousel text element hacks `systemInfo` and `logoText` have been removed and replaced with proper carousel properties
* The carousel property `maxItemCount` (formerly named maxLogoCount) is now in float format for more granular control of logo placement compared to integer format for legacy themes. However some legacy theme authors thought this property supported floats (as the theme documentation incorrectly stated this) and have therefore set it to fractional values such as 3.5. This was actually rounded up when loading the theme configuration, and this logic is retained for legacy themes for backward compatibility. But for current themes the float value is correctly interpreted which means a manual rounding of the value is required in order to retain an identical layout when porting theme sets to the new theme engine. As well carousels of the wheel type now have the amount of entries controlled by the two new properties `itemsBeforeCenter` and `itemsAfterCenter`. This provides more exact control, including the ability to setup asymmetric wheels.
* The full names of unthemed systems (or systems where the defined staticImage file is missing) will now be displayed in the system carousel instead of the short names shown for legacy themes. So for instance, instead of "cps" the full name "Capcom Play System" (as defined in es_systems.xml) will be displayed.
* The carousel now has a zIndex value of 50 instead of 40. This means it's aligned with the textlist element which already had a zIndex value of 50.
* The textlist property `selectorOffsetY` has been renamed to `selectorVerticalOffset` and a `selectorHorizontalOffset` property has been added as well.
* The helpsystem `textColorDimmed` and `iconColorDimmed` properties (which apply when opening a menu) were always defined under the system view configuration which meant these properties could not be separately set for the gamelist views. Now these properties work as expected with the possibility to configure separate values for the system and gamelist views
* When right-aligning the helpsystem using an X origin value of 1, the element is now aligned correctly to the defined position instead of being offset by the entrySpacing width (in RetroPie ES the offset was instead the hardcoded element entry padding)
* Correct theme structure is enforced more strictly than before, and deviations will generate error log messages and make the theme loading fail
* Many additional elements and properties have been added, refer to the [Reference](THEMES.md#reference) section for more information

Attempting to use any of the legacy logic in the new theme structure will make the theme loading fail, for example adding the _extra="true"_ attribute to any element.

Except the points mentioned above, theme configuration looks pretty similar to the legacy theme structure, so anyone having experience with these older themes should hopefully feel quite at home with the new theme engine. Probably the most important thing to keep in mind is that as there are no longer any view presets available, some more effort is needed from the theme developer to define values for some elements. This is especially true for zIndex values as elements could now be hidden by other elements if care is not taken to explicitly set the zIndex for each of them. This additional work is however a small price to pay for the much more powerful and flexible theming functionality provided by the new theme engine.

Note that the legacy theme engine had quite inaccurate text sizing and font rendering and while this has been greatly improved in the new engine, for legacy themes most old bugs are retained for maximum backward compatibility. This means that you may need to revise font sizes and text placements when porting a legacy theme to the new engine. Here are some examples:

* Line spacing for the textlist element was not consistently applied across different screen resolutions
* Carousel text entries did not multiply the font size by the itemScale (logoScale) property value
* The defined line spacing was not always applied for automatically sized text elements
* Font sizes were rounded to integers, leading to imprecise text sizing across different resolutions (the rounding was also done incorrectly)

## Simple example

Here is a very simple theme that changes the color of the game name text:

```xml
<theme>
    <view name="gamelist">
        <text name="gameName">
            <color>00FF00</color>
        </text>
        <image name="frame1">
            <pos>0.5 0.5</pos>
            <origin>0.5 0.5</origin>
            <size>0.8 0.8</size>
            <path>./core/frame.png</path>
            <zIndex>10</zIndex>
        </image>
    </view>
</theme>
```

## How it works

All configuration must be contained within a `<theme>` tag pair. That is true for each separate .xml file used to build the completely theme set.

The `<view>` tag pair refers to the available views within ES-DE, which is either _system_ or _gamelist_. There is a special _all_ view available as well, but that is only used for defining the navigation sounds as these are always applied globally to both view types.

Views are defined like this:

```xml
<view name="viewNameHere">
    ... define elements here ...
</view>
```

An element is a particular visual component such as an image, an animation or a piece of text. It has a mandatory _name_ attribute which is used by ES-DE to track each element entry. By using this name attribute it's possible to split up the definition of an element to different locations. For example you may want to define the color properties separately from where the size and position are configured (see the example below). The name attribute can be set to any string value.

This is the element structure:

```xml
<ElementTypeHere name="elementNameHere">
    ... define properties here ...
</ElementTypeHere>
```

Finally _properties_ control how a particular element looks and behaves, for example its position, size, image path, animation controls etc. The property type determines what kinds of values you can use. You can read about each type below in the
[Reference](THEMES.md#reference) section. Properties are defined like this:

```xml
<propertyNameHere>valueHere</propertyNameHere>
```
Let's now put it all together. The following is a simple example of a text element which has its definition split across two separate XML files.

`themes.xml`:
```xml
<theme>
    <view name="gamelist">
        <text name="systemName">
            <pos>0.27 0.32</pos>
            <origin>0.5 0.5</origin>
            <size>0.12 0.41</size>
            <zIndex>40</zIndex>
        </text>
    </view>
</theme>
```

`colors.xml`:
```xml
<theme>
    <view name="gamelist">
        <text name="systemName">
            <color>707070</color>
        </text>
    </view>
</theme>
```

As long as the name attribute is identical, the element configuration will be combined automatically. But that is only true for elements of the same type, so for instance an image element could be defined that also uses _systemName_ for its name attribute without colliding with the text element:
```xml
<theme>
    <view name="gamelist">
        <text name="systemName">
            <pos>0.27 0.32</pos>
            <origin>0.5 0.5</origin>
            <size>0.12 0.41</size>
            <zIndex>40</zIndex>
        </text>
        <!-- Does not cause a collision, but is probably a bad idea for readability reasons -->
        <image name="systemName">
            <pos>0.49 0.8</pos>
            <maxSize>0.4 0.28</maxSize>
            <zIndex>35</zIndex>
        </text>
    </view>
</theme>
```

Whether this is a good idea is another question, it would probably be better to set the name attribute for the image to _systemLogo_ or similar for this example.

In addition to this, if the name is used for the same element type but for different views, then there will also not be any collision:

```xml
<theme>
    <view name="system">
        <text name="systemName">
        <pos>0.04 0.73</pos>
        <origin>0.5 0.5</origin>
        <size>0.12 0.22</size>
        <zIndex>40</zIndex>
    </view>
    <!-- This will not cause a collision as these two text elements are defined for different views -->
    <view name="gamelist">
        <text name="systemName">
        <pos>0.27 0.32</pos>
        <origin>0.5 0.5</origin>
        <size>0.12 0.41</size>
        <zIndex>40</zIndex>
    </view>
</theme>
```

## Debugging during theme development

If you are writing a theme it's recommended to launch ES-DE with the `--debug` flag from a terminal window. You can also pass the `--resolution` flag to avoid having the application window fill the entire screen. By doing so, you can read error messages directly in the terminal window without having to open the es_log.txt file. You can also reload the current gamelist or system view with `Ctrl+r` if the `--debug` flag has been set. There is also support for highlighting the size and position of each image and animation element by using the `Ctrl+i` key combination and likewise to highlight each text element by using the `Ctrl+t` keys. Again, both of these require that ES-DE has been launched with the `--debug` command line option, for example:
```
emulationstation --debug --resolution 1280 720
```

Enforcement of a correct theme configuration is quite strict, and most errors will abort the theme loading, leading to an unthemed system. In each such situation the log output will be very clear of what happened, for instance:
```
Jan 28 17:17:30 Error:  ThemeData::parseElement(): "/home/myusername/.emulationstation/themes/mythemeset-es-de/theme.xml": Property "origin" for element "image" has no value defined (system "collections", theme "custom-collections")
```

Note that an unthemed system means precisely that, the specific system where the error occured will be unthemed but not necessarily the entire theme set. The latter can still happen if the error is global such as a missing variable used by all XML files or an error in a file included by all XML files. The approach is to only untheme relevant sections of the theme set to be able to pinpoint precisely where the problem lies.

Sanitization for valid data format and structure is done in this manner, but verification that property values are actually correct (or reasonable) is handled by the individual component that takes care of creating and rendering the specific theme element. What happens in many instances is that a warning log entry is created and the invalid property is reset to its default value. So for these situations, the system will not become unthemed. Here's an example where a badges element accidentally had its horizontalAlignment property set to _leftr_ instead of _left_:
```
Jan 28 17:25:27 Warn:   BadgeComponent: Invalid theme configuration, property "horizontalAlignment" for element "gamelistBadges" defined as "leftr"
```

Note however that warnings are not printed for all invalid properties as that would lead to an excessive amount of logging code. This is especially true for numeric values which are commonly just clamped to the allowable range without notifying the theme author. So make sure to check the [Reference](THEMES.md#reference) section of this document for valid values for each property.

For more serious issues where it does not make sense to assign a default value or auto-adjust the configuration, an error log entry is generated and the element will in most instances not get rendered at all. Here's such an example where the imageType property for a video element was accidentally set to _covr_ instead of _cover_:

```
Jan 28 17:29:11 Error:  VideoComponent: Invalid theme configuration, property "imageType" for element "gamelistVideo" defined as "covr"
```

Error handling for missing files is handled a bit differently depending on whether the paths have been defined explicitly or via a variable. For explicitly defined paths a warning will be logged for element properties and an error will be triggered for include files. Here's an example of the latter case:

```
Jan 28 17:32:29 Error:  ThemeData::parseIncludes(): "/home/myusername/.emulationstation/themes/mythemeset-es-de/theme.xml" -> "./colors_dark.xml" not found (resolved to "/home/myusername/.emulationstation/themes/mythemeset-es-de/colors_dark.xml")
```

However, if a variable has been used to define the include file, only a debug message will be generated if the file is not found:
```
Jan 28 17:34:03 Debug:  ThemeData::parseIncludes(): "/home/myusername/.emulationstation/themes/mythemeset-es-de/theme.xml": Couldn't find file "./${system.theme}/colors.xml" which resolves to "/home/myusername/.emulationstation/themes/mythemeset-es-de/amiga/colors.xml"
```

It works essentially the same way for element path properties as for include files. This distinction between explicit values and variables makes it possible to create a theme configuration where both include files and files for fonts, images, videos etc. will be used if found, and if not found a fallback configuration can still be applied so the system will be themed.

By default all debug messages regarding missing files will be logged for regular systems and automatic collections and suppressed for custom collections. This behavior can be changed by modifying the _DebugSkipMissingThemeFiles_ and _DebugSkipMissingThemeFilesCustomCollections_ settings in es_settings.xml. You can read more about those settings [here](INSTALL.md#settings-not-configurable-via-the-gui).

## Variants

A core concept of ES-DE is the use of theme set _variants_ to provide different theme profiles. These are not fixed presets and a theme author can instead name and define whatever variants he wants for his theme (or possibly use no variants at all as they are optional).

The variants could be purely cosmetic, such as providing different designs for a theme set, or they could provide distinctive functionality by for instance using different primary elements like a carousel or a text list.

Before a variant can be used it needs to be declared, which is done in the `capabilities.xml` file that must be stored in the root of the theme set directory tree. How to setup this file is described in detail later in this document.

The use of variants is straightforward, a section of the configuration that should be included for a certain variant is enclosed inside the `<variant>` tag pair. This has to be placed inside the `<theme>` tag pair, and it can only be used at this level of the hierarchy and not inside a `<view>` tag pair for example.

The mandatory _name_ attribute is used to specificy which variants to use, and multiple variants can be specified at the same time by separating them by commas or by whitespace characters (tabs, spaces or line breaks). It's also possible to use the special _all_ variant that will apply the configuration to all defined variants (although this is only a convenient shortcut and you can explicitly define every variant individually if you prefer that). Note that _all_ is a reserved name and attempting to use it in the capabilities.xml file will trigger a warning on application startup.

It could sometimes be a good idea to separate the variant configuration into separate files that are then included from the main theme file as this could improve the structure and readability of the theme set configuration.

It's also possible to apply only portions of the theme configuration to the variants and keep a common set of elements that are shared between all variants. This is accomplished by simply adding the shared configuration without specifying a variant, as is shown in the first example below for the `infoText01` text element. Just be aware that the variant-specific configuration will always be loaded after the general configuration even if it's located above the general configuration in the XML file. As this is potentially confusing and error-prone it's instead generally recommended to use the special _all_ variant to define common configuration used by all variants in the theme set rather than mixing variants configuration with non-variants configuration.

Here are some example uses of the `<variant>` functionality:

```xml
<theme>
    <!-- Implementing the variants via separate include files could be a good idea -->
    <variant name="gamelistTextlist">
        <include>./gamelist_textlist.xml</include>
    </variant>
    <variant name="gamelistCarousel">
        <include>./gamelist_carousel.xml</include>
    </variant>

    <!-- The special "all" variant is a convenient shortcut for some situations -->
    <variant name="all">
        <include>./${system.theme}/systeminfo.xml</include>
    </variant>

    <!-- This will be parsed before the variant-specific configuration -->
    <view name="gamelist">
        <text name="infoText01">
            <fontPath>./core/font.ttf</fontPath>
            <fontSize>0.035</fontSize>
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>
```

```xml
<!-- In other instances it may make more sense to apply the variant configuration inline -->
<theme>
    <variant name="withVideos">
        <view name="gamelist">
            <video name="gameVideo">
                <imageType>cover</imageType>
                <delay>1.7</delay>
                <scrollFadeIn>true</scrollFadeIn>
                <zIndex>42</zIndex>
            </video>
        </view>
    </variant>
    <variant name="withoutVideos">
        <view name="gamelist">
            <image name="gameImage">
                <imageType>titlescreen</imageType>
                <scrollFadeIn>true</scrollFadeIn>
                <zIndex>42</zIndex>
            </image>
        </view>
    </variant>
</theme>
```

```xml
<!-- The following is NOT supported as <variant> tags can't be located inside <view> tag pairs -->
<theme>
    <view name="gamelist">
        <variant name="lightModeNoVideo">
            <image name="gameImage">
                <imageType>titlescreen</imageType>
                <scrollFadeIn>true</scrollFadeIn>
                <zIndex>42</zIndex>
            </image>
        </variant>
    </view>
</theme>
```

## Variant triggers (overrides)

Variant triggers is an optional feature which can be used to replicate the automatic view style switching functionality of the legacy theme engine. This can be used to automatically override the selected variant based on two triggers, either when there are no game videos found for a system, or if there are no game media files of some specified types found for a system. These two trigger types are named `noVideos` and `noMedia` respectively.

For the `noMedia` trigger there's an optional `mediaType` tag that can be used to specify precisely which media files should be checked for to determine whether to switch to the override variant. Valid values are `miximage`, `marquee`, `screenshot`, `titlescreen`, `cover`, `backcover`, `3dbox`, `physicalmedia`, `fanart` and `video`. Multiple values can be defined, in which case they are separated by a comma, or by a whitespace character (tab, space or line break). If no value is defined, it will be set to `miximage`.

The `useVariant` tag specifies which variant to use to override the selected variant.

You'll probably rarely need to use the `noVideos` trigger as `video` can be defined also when using the `noMedia` trigger. The reason for including both trigger types is that it makes it possible to apply a specific variant only when videos are missing and another variant when no media files at all are present.

The following example (from the `capabilities.xml` file) defines a `noGameMedia` variant which is used as the override for the `withVideos` variant if no miximages, screenshots, covers and videos are found for any game in a system. For this example the `noGameMedia` variant has been set as non-selectable from the _UI Settings_ menu by defining the `selectable` property as `false`.

As can be seen here, the overall variant trigger configuration needs to be enclosed within an `override` tag pair. And you can only define a single `override` tag pair per trigger type.

```xml
    <variant name="withVideos">
        <label>Textlist with videos</label>
        <selectable>true</selectable>
        <override>
            <trigger>noMedia</trigger>
            <mediaType>miximage, screenshot, cover, video</mediaType>
            <useVariant>noGameMedia</useVariant>
        </override>
    </variant>

    <variant name="noGameMedia">
        <label>No game media</label>
        <selectable>false</selectable>
    </variant>
```

Note that variant triggers will only apply to the gamelist view and not the system view. Also be aware that it will add a potentially noticeable application slowdown as game media files need to be scanned for at various points when using the application, as well as during startup. The impact of the performance penalty depends on multiple factors such as the game collection size, how many games have been scraped, as well as disk I/O and filesystem performance. So only use variant triggers if really necessary for your theme design. As well, specifying many values for the `mediaType` tag will lead to more files potentially being scanned which could introduce further lag and latency.

As a final note, variant triggers can also be globally disabled by the user via the _Enable theme variant triggers_ option in the _UI Settings menu_. Not everyone may want the variant auto-switching to take place, and if all systems contain scraped media then disabling the functionality will eliminate the performance penalty described above.

## Color schemes

Color schemes are essentially a collection of variables that can be selected between from the _UI Settings_ menu. This makes it possible to define different values that will be applied to the overall theme configuration based on this menu selection. Only variables can be used for the color schemes, but since variables can be used for almost everything this makes the functionality very flexible. In most cases you'll probably want to apply different color values to `<color>` properties and similar, but it's also possible to apply different images, animations, fonts etc. per color scheme.

To understand the basics on how to use variables, make sure to read the _Theme variables_ section elsewhere in this document.

Before a color scheme can be used it needs to be declared, which is done in the `capabilities.xml` file that must be stored in the root of the theme set directory tree. How to setup this file is described in detail later in this document.

The `<colorScheme>` tag pair can be placed directly inside the `<theme>` tags, inside the `<variants>` tags or inside the `<aspectRatio>` tags.

The mandatory name attribute is used to specificy which color scheme to use, and multiple values can be specified at the same time by separating them by commas or by whitespace characters (tabs, spaces or line breaks).

Note that the use of color schemes for a theme set is entirely optional.

Here's an example configuration:

```xml
<theme>
    <colorScheme name="dark">
        <variables>
            <backgroundColor>404040</backgroundColor>
            <defaultTextColor>F0F0F0</defaultTextColor>
        </variables>
    </colorScheme>

    <colorScheme name="light">
        <variables>
            <backgroundColor>707070</backgroundColor>
            <defaultTextColor>262626</defaultTextColor>
        </variables>
    </colorScheme>

    <variant name="withVideos, withoutVideos">
        <colorScheme name="dark, light">
            <panelColor>74747488</panelColor>
        </colorScheme>
        <view name="system">
            <image name="background">
                <pos>0 0</pos>
                <size>1 1</size>
                <path>./core/images/background.png</path>
                <tile>true</tile>
                <color>${backgroundColor}</color>
            </image>
            <text name="gameCounter">
                <pos>0.5 0.6437</pos>
                <size>1 0.056</size>
                <color>${defaultTextColor}</color>
            </text>
        </view>
    </variant>
</theme>
```

## Aspect ratios

The aspect ratio support works almost identically to the variants and color schemes with the main difference that the available aspect ratios are hardcoded into ES-DE. The theme set can still decide which of the aspect ratios to support (or none at all in which case the theme aspect ratio is left undefined) but it can't create entirely new aspect ratio entries.

In the same manner as for the variants and color schemes, the aspect ratios that the theme set provides need to be declared in the `capabilities.xml` file that must be stored in the root of the theme set directory tree. How to setup this file is described in detailed later in this document.

The `<aspectRatio>` tag pair can be placed directly inside the `<theme>` tags or inside the `<variants>` tags.

Once the aspect ratios have been defined, they are applied to the theme configuration like the following examples:

```xml
<!-- Implementing the aspect ratios by separate include files could be a good idea -->
<theme>
    <aspectRatio name="4:3, 5:4">
        <include>./../layout_narrow.xml</include>
    </aspectRatio>
    <aspectRatio name="16:9, 16:10">
        <include>./../layout_wide.xml</include>
    </aspectRatio>
    <aspectRatio name="21:9">
        <include>./../layout_ultrawide.xml</include>
    </aspectRatio>

    <view name="gamelist">
        <text name="infoText01">
            <fontPath>./core/font.ttf</fontPath>
            <fontSize>0.035</fontSize>
        </text>
    </view>
</theme>
```

```xml
<!-- In other instances it may make more sense to apply the aspect ratio configuration inline -->
<theme>
    <aspectRatio name="4:3, 5:4">
        <view name="gamelist">
            <image name="imageLogo">
                <pos>0.3 0.56</pos>
            </image>
        </view>
    </aspectRatio>

    <aspectRatio name="16:9, 16:10, 21:9">
        <view name="gamelist">
            <image name="imageLogo">
                <pos>0.42 0.31</pos>
            </image>
        </view>
    </aspectRatio>
</theme>
```

```xml
<!-- Placing aspectRatio tags inside the variants tags is also supported -->
<theme>
    <variant name="withVideos, withoutVideos">
        <aspectRatio name="4:3, 5:4">
            <view name="gamelist">
                <image name="imageLogo">
                    <pos>0.3 0.56</pos>
                </image>
            </view>
        </aspectRatio>
    </variant>
</theme>
```

```xml
<!-- The following is NOT supported as <aspectRatio> tags can't be located inside <view> tag pairs -->
<theme>
    <view name="gamelist">
        <aspectRatio name="4:3, 5:4">
            <image name="imageLogo">
                <pos>0.3 0.56</pos>
            </image>
        </aspectRatio>
    </view>
</theme>
```

## Transitions (animation profiles)

Using the `capabilities.xml` file it's possible to define granular transition animation profiles. Prior to ES-DE 2.0 there was only a user-selectable option for _Instant_, _Slide_ or _Fade_ animations that was applied globally. It's now possible to select each of these animation types individually for the following transitions:

* System to system
* System to gamelist
* Gamelist to gamelist
* Gamelist to system
* Startup to system
* Startup to gamelist

This is a brief overview of the supported animations:

* Instant - as the name implies, transitions are immediate
* Slide - pans the camera to move between views which may look broken with some elements like textlists and grids when used in the system view
* Fade - fades to black when transitioning between views


Here's an example configuration:
```xml
<transitions name="instantAndSlide">
    <label>instant and slide</label>
    <selectable>true</selectable>
    <systemToSystem>instant</systemToSystem>
    <systemToGamelist>slide</systemToGamelist>
    <gamelistToGamelist>instant</gamelistToGamelist>
    <gamelistToSystem>slide</gamelistToSystem>
    <startupToSystem>fade</startupToSystem>
    <startupToGamelist>fade</startupToGamelist>
</transitions>

<transitions name="instant">
    <label>instant</label>
    <selectable>true</selectable>
    <systemToSystem>instant</systemToSystem>
    <systemToGamelist>instant</systemToGamelist>
    <gamelistToGamelist>instant</gamelistToGamelist>
    <gamelistToSystem>instant</gamelistToSystem>
</transitions>
```

The `name` attribute is mandatory and it must be set to a unique value for each profile. Any string can be used except the three reserved values `builtin-instant`, `builtin-slide` and `builtin-fade`.

The `selectable` property which is set to `true` by default defines whether the transitions profile can be selected from the _Theme transitions_ entry on the _UI Settings_ menu. The `label` defines the label to show there. If no label value is set then a default _Theme profile_ label will be applied.

At least one of the six transition types must be defined or the `transitions` entry is not considered valid. Any non-defined types will be set to `instant` with the exception of `startupToSystem` which will be set to the same value as `systemToSystem` and `startupToGamelist` which will be set to the same value as `gamelistToGamelist`.

The profiles will be listed in the _UI Settings_ menu in the order that they have been defined, and the first profile (regardless of whether it's set as user-selectable or not), will be used if the _Automatic_ entry has been selected, unless a per-variant configuration is defined in the theme configuration.

In addition to defining custom transition profiles it's possible to suppress the built-in profiles. For example slide transitions will look very broken with some theme designs so in such cases it could make sense to disable this animation type altogether. Suppressing a profile simply means its entry will not show up under _Theme transitions_ in the _UI Settings_ menu, making it impossible to select and use it.

Here's an example where all the built-in transition profiles have been disabled:

```xml
<suppressTransitionProfiles>
    <entry>builtin-instant</entry>
    <entry>builtin-slide</entry>
    <entry>builtin-fade</entry>
</suppressTransitionProfiles>
```

Regardless of whether any custom profiles have been created or whether the built-in profiles have been disabled there will always be an `Automatic` entry added to the _Theme transition animations_ menu. If no theme profiles have been defined and all built-in profiles have been suppressed, then the `Automatic` entry will be the only available option. In this case `instant` animations will by applied to all transition types.

Finally it's possible to apply theme-defined transition profiles on a per-variant basis. This requires that the user has selected the `Automatic` profile from the _Theme transitions_ menu as selecting any other profile will override whatever is defined in the theme configuration. Note that the built-in transition profiles can't be used in this manner, only profiles defined in `capabilities.xml`.

```xml
<variant name="withVideos">
    <transitions>instantAndSlide</transitions>
</variant>

<variant name="withoutVideos">
    <transitions>instant</transitions>
</variant>
```

## capabilities.xml

Variants, variant triggers, color schemes, aspect ratios and transition animation profiles need to be declared before they can be used inside the actual theme set configuration files and that is done in the `capabilities.xml` file. This file needs to exist in the root of the theme directory, for example:
```
~/.emulationstation/themes/mythemeset-es-de/capabilities.xml
```

This file type was introduced with the new ES-DE theme engine in v2.0 and is an indicator that the theme set is of the new generation instead of being of the legacy type (i.e. a theme set backward compatible with RetroPie EmulationStation). In other words, if the capabilities.xml file is absent, the theme will get loaded as a legacy set.

The structure of the file is simple, as can be seen in this example:

```xml
<!-- Theme capabilities for mythemeset-es-de -->
<themeCapabilities>
    <themeName>My theme set</themeName>

    <aspectRatio>16:9</aspectRatio>
    <aspectRatio>4:3</aspectRatio>
    <aspectRatio>4:3_vertical</aspectRatio>

    <colorScheme name="dark">
        <label>Dark mode</label>
    </colorScheme>

    <colorScheme name="light">
        <label>Light mode</label>
    </colorScheme>

    <transitions name="instantAndSlide">
        <systemToSystem>instant</systemToSystem>
        <systemToGamelist>slide</systemToGamelist>
        <gamelistToGamelist>instant</gamelistToGamelist>
        <gamelistToSystem>slide</gamelistToSystem>
    </transitions>

    <variant name="withVideos">
        <label>Textlist with videos</label>
        <selectable>true</selectable>
        <override>
            <trigger>noMedia</trigger>
            <mediaType>miximage, screenshot, cover, video</mediaType>
            <useVariant>noGameMedia</useVariant>
        </override>
    </variant>

    <variant name="withoutVideos">
        <label>Textlist without videos</label>
        <selectable>true</selectable>
        <override>
            <trigger>noMedia</trigger>
            <mediaType>miximage, screenshot, cover</mediaType>
            <useVariant>noGameMedia</useVariant>
        </override>
    </variant>

    <variant name="noGameMedia">
        <label>No game media</label>
        <selectable>false</selectable>
    </variant>
</themeCapabilities>
```
The file format is hopefully mostly self-explanatory; this example provides three aspect ratios, two color schemes, one transition animation profile and three variants, one of which is a variant trigger override. The `<label>` tag for the variants and transitions is the text that will show up in the _UI Settings_ menu, assuming `<selectable>` has been set to true. The same is true for color schemes, although these will always show up in the GUI and can't be disabled.

The optional `<themeName>` tag defines the name that will show up in the _Theme set_ option in the _UI Settings_ menu. If no such tag is present, then the physical directory name will be displayed instead, for example _MYTHEMESET-ES-DE_. Note that theme names will always be converted to uppercase characters when displayed in the menu. Legacy theme sets are also clearly marked with a _[LEGACY]_ suffix.

The variant, color scheme and transitions names as well as their labels can be set to arbitrary values, but the name has to be unique. If two entries are declared with the same name, a warning will be generated on startup and the duplicate entry will not get loaded. Variants, color schemes and transition animations will be listed in the _UI Settings_ menu in the order that they are defined in capabilities.xml.

Unlike the types just mentioned, aspectRatio entries can not be set to arbitrary values, instead they have to use a value from the _horizontal name_ or _vertical name_ columns in the following table:

| Horizontal name  | Vertical name  | Common resolutions                             |
| :--------------- | :------------- | :--------------------------------------------- |
| 16:9             | 16:9_vertical  | 1280x720, 1920x1080, 2560x1440, 3840x2160      |
| 16:10            | 16:10_vertical | 1280x800, 1440x900, 1920x1200                  |
| 3:2              | 3:2_vertical   | 2160x1440                                      |
| 4:3              | 4:3_vertical   | 320x240, 640x480, 800x600, 1024x768, 1600x1200 |
| 5:4              | 5:4_vertical   | 1280x1024                                      |
| 21:9             | 21:9_vertical  | 2560x1080, 3840x1600, 5120x2160                |
| 32:9             | 32:9_vertical  | 3840x1080, 5120x1440                           |

The 21:9 and 32:9 aspect ratios are approximate as monitors of slightly different ratios are collectively marketed using these numbers.

It's normally not necessary to define all or even most of these for a theme set, instead only a few are likely to be needed. The element placement will always adapt to the screen resolution as relative positions are utilized, so in most cases similar aspect ratios like 4:3 and 5:4 could be used interchangeably. The same is true for instance for 16:9 and 16:10. But if precise element placement is required, a separate configuration can still be made for each aspect ratio.

The declared aspect ratios will always get displayed in the _UI settings_ menu in the order listed in the table above, so they can be declared in any order in the capabilities.xml file. If an unsupported aspect ratio value is entered, a warning will be generated on startup and the entry will not get loaded.

The use of variants, variant triggers, color schemes, aspect ratios and transition animation profiles is optional, i.e. a theme set does not need to provide any of them. There must however be a capabilities.xml file present in the root of the theme set directory. So if you don't wish to provide this functionality, simply create an empty file or perhaps add a short XML comment to clarify that the theme set does not provide this functionality. In this case the theme will still load and work correctly but the menu options for selecting variants, color schemes and aspect ratios will be grayed out.

Note that changes to the capabilities.xml file are not reloaded when using the Ctrl+r key combination, instead ES-DE needs to be restarted to reload any changes to this file.

## The \<include\> tag

You can include theme files within theme files, for example:

`~/.emulationstation/themes/mythemeset-es-de/fonts.xml`:
```xml
<theme>
    <view name="gamelist">
        <text name="infoText01">
            <fontPath>./core/font.ttf</fontPath>
            <fontSize>0.035</fontSize>
            <color>00FF00</color>
        </text>
    </view>
</theme>
```

`~/.emulationstation/themes/mythemeset-es-de/snes/theme.xml`:
```xml
<theme>
    <include>./../fonts.xml</include>
    <view name="gamelist">
        <text name="infoText01">
            <pos>0.3 0.56</pos>
            <color>FF0000</color>
        </text>
    </view>
</theme>
```

The above is equivalent to the following:
```xml
<theme>
    <view name="gamelist">
        <text name="infoText01">
            <fontPath>./core/font.ttf</fontPath>
            <fontSize>0.035</fontSize>
            <pos>0.3 0.56</pos>
            <!-- This may or may not be what is intended, i.e. overriding the <color> tag in fonts.xml -->
            <color>FF0000</color>
        </text>
    </view>
</theme>
```

As covered earlier in this document, as long as the name attributes are identical for the same element type, the properties are combined automatically. The potential issue with the current example is that the color tag is defined in both the fonts.xml and snes/theme.xml files. As parsing is done sequentially, the property value that is defined last will overwrite the earlier value. This may be used intentionally to override a general property value, so the configuration in the example above example is not necessarily a mistake.

The paths defined for the `<include>` entry and `<fontPath>` and similar properties are set as relative to the theme file by adding "./" as a prefix. That is usually how paths would be defined as you commonly want to access files only within the theme set directory structure. This prefix works for all path properties. Windows-style backslashes are also supported as directory separators but their use is not recommended.

Explicitly defining a path will lead to an error (and the system getting unthemed) if the file is missing, but if instead using a variable to populate the `<include>` tag then a missing file will only generate a debug log entry. This makes it possible to use system variables to build flexible theme configurations where it's not guaranteed that every file exists. Such an example would be to implement default/fallback configuration for custom systems that may get added by a user.

Note that include loops are not checked for, it's the responsibility of the theme developer to make sure no such loops exist. If you accidentally introduce a loop the application will hang indefinitely on startup.

You can add `<include>` tags directly inside the `<theme>` tags or inside the `<variant>` and `<aspectRatio>` tags, but not inside the `<view>` tags:

```xml
<!-- Adding <include> directly inside <theme> is supported -->
<theme>
    <include>./../colors.xml</include>
    <view name="gamelist">
        <text name="infoText01">
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>
```

```xml
<!-- Adding <include> inside <variant> is supported -->
<theme>
    <variant name="lightMode">
        <include>./../colors.xml</include>
    </variant>
    <view name="gamelist">
        <text name="infoText01">
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>
```

```xml
<!-- Adding <include> inside <aspectRatio> is supported -->
<theme>
    <aspectRatio name="4:3">
        <include>./../colors.xml</include>
    </aspectRatio>
    <view name="gamelist">
        <text name="infoText01">
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>
```

```xml
<!-- Adding <include> inside <view> is NOT supported -->
<theme>
    <view name="gamelist">
        <include>./../colors.xml</include>
        <text name="infoText01">
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>
```

```xml
<!-- Adding <include> outside <theme> is NOT supported -->
<include>./../colors.xml</include>
<theme>
    <view name="gamelist">
        <text name="infoText01">
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>
```

## Theming the system and gamelist views simultaneously

Sometimes you may want to apply the same elements and properties to both the system and gamelist views, for instance defining a common background image. For these situations both views can simply be defined in the `name` attribute. The values can be separated by a comma, or by a whitespace character (tab, space or line break).

```xml
<view name="system, gamelist">
    <image name="background">
        <tile>true</tile>
        <size>1 1</size>
        <pos>0 0</pos>
        <origin>0 0</origin>
        <path>./core/images/background.png</path>
        <zIndex>0</zIndex>
    </image>
```

The above is equivalent to:

```xml
<view name="system">
    <image name="background">
        <tile>true</tile>
        <size>1 1</size>
        <pos>0 0</pos>
        <origin>0 0</origin>
        <path>./core/images/background.png</path>
        <zIndex>0</zIndex>
    </image>
</view>
<view name="gamelist">
    <image name="background">
        <tile>true</tile>
        <size>1 1</size>
        <pos>0 0</pos>
        <origin>0 0</origin>
        <path>./core/images/background.png</path>
        <zIndex>0</zIndex>
    </image>
</view>
```

## Theming multiple elements simultaneously

You can theme multiple elements of the same type simultaneously, which can lead to a more compact and easier to understand theme configuration. To accomplish this you simply define multiple entries inside a single `name` attribute, separated by commas or whitespace characters (tabs, spaces or line breaks).

Here's an example of defining a common color to multiple text elements:

```xml
<theme>
    <view name="gamelist">
        <!-- Weird spaces/newline on purpose -->
        <text name="labelRating, labelReleasedate labelDeveloper labelPublisher,
                labelGenre,    labelPlayers,        labelLastplayed, labelPlaycount">
            <color>48474D</color>
        </text>
    </view>
</theme>
```

The above is equivalent to:

```xml
<theme>
    <view name="gamelist">
        <text name="labelRating">
            <color>48474D</color>
        </text>
        <text name="labelReleasedate">
            <color>48474D</color>
        </text>
        <text name="labelDeveloper">
            <color>48474D</color>
        </text>
        <text name="labelPublisher">
            <color>48474D</color>
        </text>
        <text name="labelGenre">
            <color>48474D</color>
        </text>
        <text name="labelPlayers">
            <color>48474D</color>
        </text>
        <text name="labelLastplayed">
            <color>48474D</color>
        </text>
        <text name="labelPlaycount">
            <color>48474D</color>
        </text>
    </view>
</theme>
```

Just remember, _this only works if the elements have the same type._


## Navigation sounds

Navigation sounds are configured globally per theme set, so it needs to be defined using the special `all` view.
It's recommended to put these elements in a separate file and include it from the main theme file (e.g. `<include>./navigationsounds.xml</include>`). Starting ES-DE with the --debug flag will provide feedback on whether any navigation sound elements were read from the theme set. If no navigation sounds are provided by the theme, ES-DE will use the bundled navigation sounds as a fallback. This is done per sound file, so the theme could provide for example one or two custom sounds while using the bundled ES-DE sounds for the rest.

Example debug output:
```
Jul 12 11:28:58 Debug:  NavigationSounds::loadThemeNavigationSounds(): Theme set includes navigation sound support, loading custom sounds
Jul 12 11:28:58 Debug:  Sound::getFromTheme(): Looking for tag <sound name="systembrowse">
Jul 12 11:28:58 Debug:  Sound::getFromTheme(): Tag found, ready to load theme sound file
Jul 12 11:28:58 Debug:  Sound::getFromTheme(): Looking for tag <sound name="quicksysselect">
Jul 12 11:28:58 Debug:  Sound::getFromTheme(): Tag not found, using fallback sound file
```

Example `navigationsounds.xml` file:

```xml
<theme>
    <view name="all">
        <sound name="systembrowse">
            <path>./core/sounds/systembrowse.wav</path>
        </sound>
        <sound name="quicksysselect">
            <path>./core/sounds/quicksysselect.wav</path>
        </sound>
        <sound name="select">
            <path>./core/sounds/select.wav</path>
        </sound>
        <sound name="back">
            <path>./core/sounds/back.wav</path>
        </sound>
        <sound name="scroll">
            <path>./core/sounds/scroll.wav</path>
        </sound>
        <sound name="favorite">
            <path>./core/sounds/favorite.wav</path>
        </sound>
        <sound name="launch">
            <path>./core/sounds/launch.wav</path>
        </sound>
    </view>
</theme>
```

## Element rendering order using zIndex

You can change the order in which elements are rendered by setting their `zIndex` values. All elements have a default value so you only need to define it for the ones you wish to explicitly change. Elements will be rendered in order from smallest to largest values. A complete description of each element including all supported properties can be found in the [Reference](THEMES.md#reference) section.

These are the default zIndex values per element type:

| Element          | zIndex value  |
| :--------------- | :-----------: |
| image            | 30            |
| video            | 30            |
| animation        | 35            |
| badges           | 35            |
| text             | 40            |
| datetime         | 40            |
| gamelistinfo     | 45            |
| rating           | 45            |
| carousel         | 50            |
| grid             | 50            |
| textlist         | 50            |

The `helpsystem` element does not really have a zIndex value and is always rendered on top of all other elements.

## Theme variables

Theme variables can be used to simplify theme construction and there are two types available:

* System variables
* Theme defined variables

### System variables

System variables are system specific and are derived from values defined in es_systems.xml (except for collections which are derived from hardcoded application-internal values).
* `system.name`
* `system.name.autoCollections`
* `system.name.customCollections`
* `system.name.noCollections`
* `system.fullName`
* `system.fullName.autoCollections`
* `system.fullName.customCollections`
* `system.fullName.noCollections`
* `system.theme`
* `system.theme.autoCollections`
* `system.theme.customCollections`
* `system.theme.noCollections`

`system.name` expands to the short name of the system as defined by the `name` tag in es_systems.xml\
`system.fullName` expands to the full system name as defined by the `fullname` tag in es_systems.xml\
`system.theme` expands to the theme directory as defined by the `theme` tag in es_systems.xml

If using variables to load theme assets like images and videos, then use the `.name` versions of these variables as short system names should be stable and not change over time. The `.fullName` values could change in future ES-DE releases or they could be user-customized which would break your theme set.

The `.autoCollections`, `.customCollections` and `.noCollections` versions of the variables make it possible to differentiate between regular systems, automatic collections (_all games_, _favorites_ and _last played_) and custom collections. This can for example be used to apply different formatting to the names of the collections as opposed to regular systems.

The below example capitalizes the names of the auto collections while leaving custom collections and regular systems at their default formatting (as they are defined by the user and es_systems.xml respectively). The reason this works is that the .autoCollections, .customCollections and .noCollections variables are mutually exclusive, i.e. a system is either a real system or an automatic collection or a custom collection and never more than one of these.

```xml
<theme>
    <view name="system">
        <text name="systemName, autoCollectionName, customCollectionName">
            <pos>0.05 0.83</pos>
            <size>0.9 0.06</size>
            <fontSize>0.06</fontSize>
            <fontPath>./core/font.ttf</fontPath>
        </text>
        <text name="systemName">
            <text>${system.fullName.noCollections}</text>
            <letterCase>none</letterCase>
        </text>
        <text name="autoCollectionName">
            <text>${system.fullName.autoCollections}</text>
            <letterCase>capitalize</letterCase>
        </text>
        <text name="customCollectionName">
            <text>${system.fullName.customCollections}</text>
            <letterCase>none</letterCase>
        </text>
</view>
</theme>
```

### Theme defined variables
Variables can also be defined in the theme.
```xml
<theme>
    <variables>
        <themeColor>8B0000</themeColor>
    </variables>
</theme>
```

### Usage in themes
Variables can be used to specify the value of a theme property:
```xml
<color>${themeColor}</color>
```

It can also be used to specify only a portion of the value of a theme property:

```xml
<color>${themeColor}C0</color>
<path>./core/images/${system.theme}.svg</path>
````

Nesting of variables is supported, so the following could be done:
```xml
<theme>
    <variables>
        <colorRed>8b0000</colorRed>
        <themeColor>${colorRed}</themeColor>
    </variables>
</theme>
```

Variables can also be declared inside the `<variant>` and `<aspectRatio>` tags, but make sure to read the comments below for the implications and possibly unforeseen behavior when doing this:
```xml
<theme>
    <variant name="lightMode, lightModeNoVideo">
        <variables>
            <colorRed>8b0000</colorRed>
            <themeColor>${colorRed}</themeColor>
        </variables>
    </variant>
</theme>
```

Variables live in the global namespace, i.e. they are reachable by all configuration entries regardless of whether variants are used or not. This means that if a variable is defined directly under the `<theme>` tag and then redefined inside a `<variant>` or `<aspectRatio>` tag then the global variable will be modified rather than a copy specific to the variant. As well, since all general (non-variant) configuration is parsed prior to the variant configuration, any overriding of the variable will be done "too late" to apply to the general configuaration. Take this example:

```xml
<theme>
    <!-- Set the value of variable themeColor to 8b0000 -->
    <variables>
        <colorRed>8b0000</colorRed>
        <themeColor>${colorRed}</themeColor>
    </variables>

    <!-- Override the value of variable themeColor by defining it as 6533ff -->
    <variant name="lightMode, lightModeNoVideo">
        <variables>
            <themeColor>6533ff</themeColor>
        </variables>
    </variant>

    <!-- color will be set to 8b0000 as it's parsed before the variants configuration -->
    <view name="gamelist">
        <text name="infoText01">
            <pos>0.3 0.56</pos>
            <color>${themeColor}</color>
        </text>
    </view>

    <!-- color will be set to 6533ff -->
    <variant name="lightMode, lightModeNoVideo">
        <view name="gamelist">
            <text name="gameName">
                <pos>0.8 0.12</pos>
                <color>${themeColor}</color>
            </text>
        </view>
    </variant>
</theme>
```

Due to the potential confusion caused by the above configuration it's recommended to never use the same variable names under the `<variant>` or `<aspectRatio>` tags as have previously been declared directly under the `<theme>` tag.

## Configuration parsing order

It's important to understand how the theme configuration files are parsed in order to avoid potentially confusing issues that may appear to be bugs. The following order is always used:

1) Transitions
2) Variables
3) Color schemes
4) Included files
5) "General" (non-variant) configuration
6) Variants
7) Aspect ratios

When including a file using the `<include>` tag (i.e. step 4 above) then all steps listed above are executed for that included file prior to continuing to the next line after the `<include>` tag.

For any given step, the configuration is parsed in the exact order that it's defined in the XML file. Be mindful of the logic described above as for instance defining variant-specific configuration above general configuration in the same XML file will still have that parsed afterwards.

## Property data types

* NORMALIZED_PAIR - two decimal values delimited by a space, for example `0.25 0.5`
* PATH - path to a resource. If the first character is a tilde (`~`) then it will be expanded to the user's home directory (`$HOME` for Unix and macOS and `%HOMEPATH%` for Windows) unless overridden using the --home command line option.  If the first character is a dot (`.`) then the resource will be searched for relative to the location of the theme file, for example `./myfont.ttf` or `./../core/fonts/myfont.ttf`
* BOOLEAN - `true`/`1` or `false`/`0`
* COLOR - a hexadecimal RGB or RGBA color value consisting of 6 or 8 digits. If a 6 digit value is used then the alpha channel will be set to `FF` (completely opaque)
* UNSIGNED_INTEGER - an unsigned integer value
* FLOAT - a decimal value
* STRING - a string of text

## Element types and their properties

There are three groups of elements available for use which are named _primary_, _secondary_ and _special_. They are all covered in detail below.

Common to almost all elements are `pos` and `size` properties of the NORMALIZED_PAIR type. They are normalized in terms of their parent's size. Most of the time this is just the size of the screen. In this case, `<pos>0 0</pos>` would correspond to the top left corner, and `<pos>1 1</pos>` the bottom right corner (a positive Y value points further down). You can also use numbers outside the 0 to 1 range if you want to place an element partially off-screen.

The order in which you define properties for a given element does not matter and you only need to define a property if you want to override its default value. If a property is defined multiple times then the latest entry will override any previous occurances.

### Primary elements

Elements from this group can only occur once per view (for a certain variant) and they handle basic functionality like controller input and navigation.

#### carousel

A carousel for navigating and selecting games or systems.

Supported views:
* `system`
* `gamelist`

Instances per view:
* `single`

Properties:
* `pos` - type: NORMALIZED_PAIR
    - Default is `0 0.38378`
* `size` - type: NORMALIZED_PAIR
    - Minimum value per axis is `0.05` and maximum value per axis is `2`
    - Default is `1 0.2324`
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the carousel exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `type` - type: STRING
    - Sets the carousel type and scroll direction.
    - Valid values are `horizontal`, `vertical`, `horizontalWheel` or `verticalWheel`
    - Default is `horizontal`
* `staticImage` - type: PATH
    - Path to a static image file. Most common extensions are supported (including .svg, .jpg, .png, and unanimated .gif).
    - This property can only be used in the `system` view.
* `imageType` - type: STRING
    - This displays a game image of a certain media type, and can only be used in the `gamelist` view. Optionally two types can be defined, in which case the entries should be delimited by commas or by whitespace characters (tabs, spaces or line breaks). The media will be searched for in the order that the entries have been defined, and any superfluous entries will be ignored. Note that defining two entries can lead to quite a performance penalty so in general it's recommended to define a single value and instead use `defaultImage` as a fallback in case no image is found.
    - Valid values:
    - `marquee` - This will look for a marquee (wheel) image.
    - `cover` - This will look for a box front cover image.
    - `backcover` - This will look for a box back cover image.
    - `3dbox` - This will look for a 3D box image.
    - `physicalmedia` - This will look for a physical media image.
    - `screenshot` - This will look for a screenshot image.
    - `titlescreen` - This will look for a title screen image.
    - `miximage` - This will look for a miximage.
    - `fanart` - This will look for a fan art image.
    - `none` - No image will be used, instead the game name will be displayed as text. Has no effect if `defaultImage` has been defined.
    - Default is `marquee`
* `defaultImage` - type: PATH
    - Path to the default image file which will be displayed if the image defined via the `staticImage` or `imageType` property is not found. Most common extensions are supported (including .svg, .jpg, .png, and unanimated .gif).
* `defaultFolderImage` - type: PATH
    - Path to the default image file which will be displayed if the image defined via the `staticImage` or `imageType` property is not found and the item is a folder. Most common extensions are supported (including .svg, .jpg, .png, and unanimated .gif).
    - Default is the same value as `defaultImage`
    - This property can only be used in the `gamelist` view.
* `maxItemCount` - type: FLOAT
    - Sets the number of carousel items to display.
    - Minimum value is `0.5` and maximum value is `30`
    - Default is `3`
    - This property can only be used when `type` is `horizontal` or `vertical`
* `itemsBeforeCenter` - type: UNSIGNED_INTEGER
    - Sets the number of items before the center position (the currently selected item). By setting this property and `itemsAfterCenter` to different values an asymmetric wheel can be configured. Combine with `itemRotation` to control how many entries to display in the carousel.
    - Minimum value is `0` and maximum value is `20`
    - Default is `8`
    - This property can only be used when `type` is `horizontalWheel` or `verticalWheel`
* `itemsAfterCenter` - type: UNSIGNED_INTEGER
    - Sets the number of items after the center position (the currently selected item). By setting this property and `itemsBeforeCenter` to different values an asymmetric wheel can be configured. Combine with `itemRotation` to control how many entries to display in the carousel.
    - Minimum value is `0` and maximum value is `20`
    - Default is `8`
    - This property can only be used when `type` is `horizontalWheel` or `verticalWheel`
* `itemStacking` - type: STRING
    - Controls how to stack overlapping items. When set to `centered` the selected item will be raised and items further from the selected item (to the left/right or above/below depending on the carousel orientation) will be progressively rendered lower than the items closer to the center. If set to `ascending` then items will be rendered progressively higher from left to right or from top to bottom depending on the carousel orientation. If set to `descending` the opposite takes place with items being progressively rendered lower from left to right or top to bottom depending on the carousel orientation. Finally `ascendingRaised` and `descendingRaised` work identically to `ascending` and `descending` with the only difference that the currently selected item will be raised above the other items.
    - Valid values are `centered`, `ascending`, `ascendingRaised`, `descending` or `descendingRaised`
    - Default is `centered`
    - This property can only be used when `type` is `horizontal` or `vertical`
* `selectedItemMargins` - type: NORMALIZED_PAIR
    - By default items are evenly spaced across the carousel area, but this property makes it possible to define margins (extra space or less space) around the currently selected item. The first value in the pair defines the margin to the left of the item if it's a horizontal carousel or above the item if it's a vertical carousel, and the second value of the pair sets the right or bottom margin for the selected item depending on the carousel orientation.
    - Minimum value per margin is `-1` and maximum value per margin is `1`
    - Default is `0 0`
    - This property can only be used when `type` is `horizontal` or `vertical`
* `itemSize` - type: NORMALIZED_PAIR
    - Size of the item prior to multiplication by the `itemScale` value, i.e. the size of all unselected items. Both axes need to be defined.
    - Minimum value per axis is `0.05` and maximum value per axis is `1`
    - Default is `0.25 0.155`
* `itemScale` - type: FLOAT.
    - Selected item is scaled by the value defined by this property.
    - Minimum value is `0.2` and maximum value is `3`
    - Default is `1.2`
* `itemRotation` - type: FLOAT
    - Angle in degrees that items should be rotated. This value should be positive if the `itemRotationOrigin` X axis has a negative value, and it should be negative if the `itemRotationOrigin` X axis has a positive value, otherwise the wheel will rotate in the wrong direction.
    - Default is `7.5`
    - This property can only be used when `type` is `horizontalWheel` or `verticalWheel`
* `itemRotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the items will be rotated. The X axis of this property is the distance from the left side of the item to the center of the wheel in multiples of the size defined by the `itemSize` X axis. So if for instance the itemSize X axis is set to 0.2 and itemRotationOrigin is set to -2, then the center of the wheel will be at a -0.4 distance from the left side of the item. In other words, if specifying a negative number the item will be located on the right side of the carousel, i.e. the wheel will be to the left and if specifying a positive number the wheel will be to the right. Note again that this is calculated from the left side of the item, so to get an identically sized wheel as the -2 wheel just mentioned you need to define 3 as the value rather than 2 if you want the wheel to the right side of the item. This is not an error but due to the way that coordinates are calculated. The Y axis should normally be left at `0.5` or you may get some weird results. It is however possible to use this axis value creatively if you know what you are doing.
    - Default is `-3 0.5`
    - This property can only be used when `type` is `horizontalWheel` or `verticalWheel`
* `itemAxisHorizontal` - type: BOOLEAN
    - Wheel carousel items are normally rotated towards the center of the wheel as defined by `itemRotation` and `itemRotationOrigin`. But if enabling this property the items will not get rotated along their own axis, meaning they will retain their original horizontal orientation regardless of their position along the wheel. Make sure that `itemVerticalAlignment` is set to `center` when using this property or you'll get strange alignment issues.
    - Default is `false`
    - This property can only be used when `type` is `horizontalWheel` or `verticalWheel`
* `itemAxisRotation` - type: FLOAT
    - Angle in degrees that items should be rotated around their own axis. Note that this does not work well with reflections as these are rotated too which does not look right.
    - Default is `0`
    - This property can only be used when `type` is `horizontal` or `vertical`
* `imageFit` - type: STRING
    - Controls how to fit the image within the aspect ratio defined by `itemSize`. To scale and preserve the original aspect ratio, set the value to `contain`, to stretch/squash the image to fill the entire area set it to `fill` and to crop the image to fill the entire area set it to `cover`
    - Valid values are `contain`, `fill` or `cover`
    - Default is `contain`
* `imageInterpolation` - type: STRING
    - Interpolation method to use when scaling images. Nearest neighbor (`nearest`) preserves sharp pixels and linear filtering (`linear`) makes the image smoother. The effect of this property is primarily visible for raster graphic images, but it has a limited effect also when using scalable vector graphics (SVG) images as these are rasterized at a set resolution and then scaled using the GPU.
    - Valid values are `nearest` or `linear`
    - Default is `linear`
* `imageColor` - type: COLOR
    - Applies a color shift to the images defined by `staticImage`, `imageType` or `defaultImage` by multiplying each pixel's color by this color value. For example, an all-white image with `FF0000` applied would become completely red. You can also control the transparency of the images by setting the value to for example `FFFFFFAA`. This keeps all pixels at their normal color and only affects the alpha channel. This property is applied after `imageSaturation` so by setting that property to `0` it's possible to colorize rather than color shift.
    - Default is `FFFFFFFF` (no color shift applied)
* `imageColorEnd` - type: COLOR
    - Works in the exact same way as `imageColor` but can be set as the end color to apply a color shift gradient.
    - Default is the same value as `imageColor`
* `imageGradientType` - type: STRING
    - The direction to apply the color gradient if both `imageColor` and `imageColorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `imageSelectedColor` - type: COLOR
    - Applies a color shift to the currently selected item's image as defined by `staticImage`, `imageType` or `defaultImage` by multiplying each pixel's color by this color value. For example, an all-white image with `FF0000` applied would become completely red. You can also control the transparency of the images by setting the value to for example `FFFFFFAA`. This keeps all pixels at their normal color and only affects the alpha channel. This property is applied after `imageSaturation` so by setting that property to `0` it's possible to colorize rather than color shift.
    - Default is the same value as `imageColor`
* `imageSelectedColorEnd` - type: COLOR
    - Works in the exact same way as `imageSelectedColor` but can be set as the end color to apply a color shift gradient.
    - Default is the same value as `imageSelectedColor`
* `imageSelectedGradientType` - type: STRING
    - The direction to apply the color gradient if both `imageSelectedColor` and `imageSelectedColorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `imageBrightness` - type: FLOAT
    - Controls the relative level of brightness. This is intended primarily for fine adjustments, for example if a color shift has been applied which may have lowered the overall brightness of the image.
    - Minimum value is `-2` and maximum value is `2`
    - Default is `0` (no brightness adjustments applied)
* `imageSaturation` - type: FLOAT
    - Controls the level of color saturation.
    - Minimum value is `0` (grayscale) and maximum value is `1` (original file saturation).
    - Default is `1`
* `itemTransitions` - type: STRING
    - How to render item transitions when navigating the carousel. By default a slide, scale and opacity fade animation will be played when moving between items (the latter two assuming `itemScale` and `unfocusedItemOpacity` have not been set to `1`) but if this property is set to `instant` then transitions will be immediate.
    - Valid values are `animate` or `instant`
    - Default is `animate`
* `itemDiagonalOffset` - type: FLOAT
    - Offsets all items to the left/right or above/below the selected item (depending on the carousel orientation) to achieve a diagonal layout. The defined value is the per-item offset (screen height percentage if `type` is `horizontal` or screen width percentage if `type` is `vertical`)
    - Minimum value is `-0.5` and maximum value is `0.5`
    - Default is `0`
    - This property can only be used when `type` is `horizontal` or `vertical`
* `itemHorizontalAlignment` - type: STRING
    - Sets `staticImage` / `imageType` and `text` alignment relative to the carousel on the X axis.
    - Valid values are `left`, `center` or `right`
    - Default is `center`
    - This property can only be used when `type` is `vertical` or `verticalWheel`
* `itemVerticalAlignment` - type: STRING
    - Sets `staticImage` / `imageType` and `text` alignment relative to the carousel on the Y axis. Make sure to set this to `center` if you've enabled `itemAxisHorizontal`
    - Valid values are `top`, `center` or `bottom`
    - Default is `center`
    - This property can only be used when `type` is `horizontal`, `horizontalWheel` or `verticalWheel`
* `wheelHorizontalAlignment` - type: STRING
    - Sets the horizontal alignment of the actual carousel inside the overall element area. Note that the positioning is calculated before `itemAxisHorizontal` is applied.
    - Valid values are `left`, `center` or `right`
    - Default is `center`
    - This property can only be used when `type` is `verticalWheel`
* `wheelVerticalAlignment` - type: STRING
    - Sets the vertical alignment of the actual carousel inside the overall element area.
    - Valid values are `top`, `center` or `bottom`
    - Default is `center`
    - This property can only be used when `type` is `horizontalWheel`
* `horizontalOffset` - type: FLOAT
    - Offsets the carousel horizontally inside its designated area, as defined by the `size` property. The value of this property is relative to the width of the carousel (with `1` being equivalent to its entire width). This property can for example be used to add a margin if using `itemHorizontalAlignment` or to offset the selected item of horizontal carousels to a non-centered position.
    - Minimum value is `-1.0` and maximum value is `1`
    - Default is `0`
* `verticalOffset` - type: FLOAT
    - Offsets the carousel vertically inside its designated area, as defined by the `size` property. The value of this property is relative to the height of the carousel (with `1` being equivalent to its entire height). This can be used to add a margin if using `itemVerticalAlignment` but is even more useful if `reflections` has been set as it allows the control of how much of the reflections to display by relocating the carousel inside its clipping area. It can also be used to offset the selected item of vertical carousels to a non-centered position.
    - Minimum value is `-1.0` and maximum value is `1`
    - Default is `0`
* `reflections` - type: BOOLEAN
    - Enables reflections beneath the carousel items. It's probably a good idea to combine this with the `verticalOffset` property to define how much of the reflections should be visible.
    - Default is `false`
    - This property can only be used when `type` is `horizontal`
* `reflectionsOpacity` - type: FLOAT
    - Defines the base opacity for the reflections.
    - Minimum value is `0.1` and maximum value is `1`
    - Default is `0.5`
    - This property can only be used when `type` is `horizontal`
* `reflectionsFalloff` - type: FLOAT
    - Defines the reflections opacity falloff, starting from the item's base opacity and ending at complete transparency. The value is set relative to the item height, so `1` will fade the bottom of the item to full transparency, `2` will fade to full transparency at half the item height and `0.5` will place the full transparency point at twice the item height.
    - Minimum value is `0` and maximum value is `10`
    - Default is `1`
    - This property can only be used when `type` is `horizontal`
* `unfocusedItemOpacity` - type: FLOAT
    - Sets the opacity for all items except the currently selected entry.
    - Minimum value is `0.1` and maximum value is `1`
    - Default is `0.5`
* `unfocusedItemSaturation` - type: FLOAT
    - Sets the saturation for all items except the currently selected entry. This property takes precedence over `imageSaturation` if that has also been defined.
    - Minimum value is `0` (grayscale) and maximum value is `1` (original file saturation).
    - Default is `1`
* `unfocusedItemDimming` - type: FLOAT
    - Sets the dimming for all items except the currently selected entry.
    - Minimum value is `0` (pure black) and maximum value is `1` (no adjustment)
    - Default is `1`
* `fastScrolling` - type: BOOLEAN
    - Normally the carousel scrolls at a constant and somehow slow pace, but via this property it's possible to introduce faster scrolling with an additional higher scrolling tier similar to the gamelist textlist (although slightly slower than that). This requires that the carousel has three or more entries, otherwise the highest scrolling tier will never be triggered. Be aware of possible performance implications when enabling this property, for gamelist views it's probably mostly useful for text-based carousels as streaming carousel images at the higher scrolling speed is likely to lead to stuttering on slower machines. Similarly, using this property in the system view together with gameselector configuration may lead to quite a lot of lag on weaker machines.
    - Default is `false`
* `color` - type: COLOR
    - Color of the carousel background panel. Setting a value of `00000000` makes the background panel transparent.
    - Default is `FFFFFFD8`
* `colorEnd` - type: COLOR
    - Setting this to something other than what is defined for `color` creates a color gradient on the background panel.
    - Default is the same value as `color`
* `gradientType` - type: STRING
    - The direction to apply the color gradient if both `color` and `colorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `text` - type: STRING
    - A string literal to display if there is no `staticImage` or `defaultImage` property defined and if no image is found.
    - Default is the full system name.
    - This property can only be used in the `system` view as for the gamelist view the game name is always used as fallback.
* `textColor` - type: COLOR
    - Default is `000000FF`
* `textBackgroundColor` - type: COLOR
    - Default is `FFFFFF00`
* `textSelectedColor` - type: COLOR
    - Sets the text color for the currently selected item.
    - Default is the same value as `textColor`
* `textSelectedBackgroundColor` - type: COLOR
    - Sets the text background color for the currently selected item.
    - Default is the same value as `textBackgroundColor`
* `fontPath` - type: PATH
    - Path to a TrueType font (.ttf) used as fallback if there is no `staticImage` / `imageType` image defined or found, and if `defaultImage` has not been defined.
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height for horizontally oriented screens or screen width for vertically oriented screens (e.g. for a value of `0.1`, the text's height would be 10% of the screen height). This calculation is based on the reference 'S' character so other glyphs may not fill this area, or they may exceed this area. This property value is effectively multiplied by the `itemScale` value for the currently selected item (but if this property is omitted then the default value will not get multiplied by `itemScale`).
    - Minimum value is `0.001` and maximum value is `1.5`. Note that when running at a really low resolution, the minimum value may get clamped to a larger relative size.
    - Default is `0.085`
* `letterCase` - type: STRING
    - Sets the letter case for all entries.
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is `none` (original letter case is retained).
* `letterCaseAutoCollections` - type: STRING
    - Sets the letter case specifically for automatic collection entries (_all games_, _favorites_ and _last played_) which have their names spelled in lowercase by default.
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is the same value as `letterCase`
    - This property can only be used in the `system` view and it will take precedence over `letterCase` if that has also been defined.
* `letterCaseCustomCollections` - type: STRING
    - Sets the letter case specifically for custom collections entries. Be cautious about using this property as it will override whatever lettercase the user has defined for their custom collection names. This property takes precedence over `letterCase` if that has also been defined.
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is the same value as `letterCase`
* `lineSpacing` - type: FLOAT
    - Controls the space between lines (as a multiple of the font height). Due to the imprecise nature of typefaces where certain glyphs (characters) may exceed the requested font size, it's recommended to keep this value at around `1.1` or higher. This way overlapping glyphs or characters being cut off at the top or bottom will be prevented.
    - Minimum value is `0.5` and maximum value is `3`
    - Default is `1.5`
* `systemNameSuffix` - type: BOOLEAN
    - Whether to add the system name in square brackets after the game name when inside a collection system (automatic as well as custom collections). This assumes a fallback to text is made.
    - Default is `true`
    - This property can only be used in the `gamelist` view.
* `letterCaseSystemNameSuffix` - type: STRING
    - Sets the letter case for the system name suffix.
    - Valid values are `uppercase`, `lowercase` or `capitalize`
    - Default is `uppercase`
    - This property can only be used in the `gamelist` view and only when `systemNameSuffix` is `true`
* `fadeAbovePrimary` - type: BOOLEAN
    - When using fade transitions, all elements in the `system` view with a zIndex value higher than the carousel are by default still rendered during transitions. If this property is enabled then all such elements will instead be faded out. Note that elements below the carousel will be dimmed to black and elements above the carousel will be faded to transparent.
    - Default is `false`
    - This property can only be used in the `system` view.
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `50`

#### grid

An X*Y grid for navigating and selecting games or systems using the left/right and up/down buttons. The layout including the amount of columns and rows is automatically calculated based on the relevant property values.

Supported views:
* `system`
* `gamelist`

Instances per view:
* `single`

Properties:
* `pos` - type: NORMALIZED_PAIR
    - Default is `0 0.1`
* `size` - type: NORMALIZED_PAIR
    - Minimum value per axis is `0.05` and maximum value per axis is `1`
    - Default is `1 0.8`
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the grid exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `staticImage` - type: PATH
    - Path to a static image file. Most common extensions are supported (including .svg, .jpg, .png, and unanimated .gif).
    - This property can only be used in the `system` view.
* `imageType` - type: STRING
    - This displays a game image of a certain media type, and can only be used in the `gamelist` view. Optionally two types can be defined, in which case the entries should be delimited by commas or by whitespace characters (tabs, spaces or line breaks). The media will be searched for in the order that the entries have been defined, and any superfluous entries will be ignored. Note that defining two entries can lead to quite a performance penalty so in general it's recommended to define a single value and instead use `defaultImage` as a fallback in case no image is found.
    - Valid values:
    - `marquee` - This will look for a marquee (wheel) image.
    - `cover` - This will look for a box front cover image.
    - `backcover` - This will look for a box back cover image.
    - `3dbox` - This will look for a 3D box image.
    - `physicalmedia` - This will look for a physical media image.
    - `screenshot` - This will look for a screenshot image.
    - `titlescreen` - This will look for a title screen image.
    - `miximage` - This will look for a miximage.
    - `fanart` - This will look for a fan art image.
    - `none` - No image will be used, instead the game name will be displayed as text. Has no effect if `defaultImage` has been defined.
    - Default is `marquee`
* `defaultImage` - type: PATH
    - Path to the default image file which will be displayed if the image defined via the `staticImage` or `imageType` property is not found. Most common extensions are supported (including .svg, .jpg, .png, and unanimated .gif).
* `defaultFolderImage` - type: PATH
    - Path to the default image file which will be displayed if the image defined via the `staticImage` or `imageType` property is not found and the item is a folder. Most common extensions are supported (including .svg, .jpg, .png, and unanimated .gif).
    - Default is the same value as `defaultImage`
    - This property can only be used in the `gamelist` view.
* `itemSize` - type: NORMALIZED_PAIR
    - Size of the overall item prior to multiplication by the `itemScale` value, i.e. the size of all unselected items. If one of the axis is defined as `-1` then it will be set to the same pixel value as the other axis, resulting in a perfectly square item. If not using this approach then both axes need to be defined.
    - Minimum value per axis is `0.05` and maximum value per axis is `1`
    - Default is `0.15 0.25`
* `itemScale` - type: FLOAT.
    - Selected overall item is scaled by the value defined by this property.
    - Minimum value is `0.5` and maximum value is `2`
    - Default is `1.05`
* `itemSpacing` - type: NORMALIZED_PAIR
    - The horizontal and vertical space between items. This value is added to the unscaled item size, i.e. `itemSize` before it's been multiplied by `itemScale`. This means that if an axis is set to `0` then unscaled items will be perfectly adjacent to each other on that axis but if `itemScale` has been set to higher than `1` then the currently selected item will overlap adjacent items. If this property is omitted then spacing will be automatically calculated so that no overlaps occur during scaling. However you'd normally want to define and adjust this property for an optimal layout. If one of the axis is defined as `-1` then it will be set to the same pixel value as the other axis. Note that all spacing calculations are based on the value defined by `itemSize` which may or may not be the same as the actual image sizes, depending on their aspect ratios and if the `imageFit` property is used.
    - Minimum value per axis is `0` and maximum value per axis is `0.1`
* `fractionalRows` - type: BOOLEAN
    - Whether to allow rendering of fractional rows of items. If set to false then the effective area of the overall element size will be snapped to the item height multiplied by `itemScale`. Note that if setting `itemScale` too high relative to the `itemSpacing` Y axis value then fractional rows may still be rendered even if the `fractionalRows` property is set to false.
    - Default is `false`
* `itemTransitions` - type: STRING
    - How to render item transitions when navigating the grid. By default a scaling and opacity fade animation will be played when moving between items (assuming `itemScale` and `unfocusedItemOpacity` have not been set to `1`) but if this property is set to `instant` then transitions will be immediate.
    - Valid values are `animate` or `instant`
    - Default is `animate`
* `rowTransitions` - type: STRING
    - How to render row transitions when navigating the grid. By default a sliding animation will be rendered when moving between rows but if this property is set to `instant` then transitions will be immediate. If setting this to `instant` it's recommended to do the same for `itemTransitions` or otherwise the animations will look a bit ugly.
    - Valid values are `animate` or `instant`
    - Default is `animate`
* `unfocusedItemOpacity` - type: FLOAT
    - Sets the opacity for all items except the currently selected entry.
    - Minimum value is `0.1` and maximum value is `1`
    - Default is `1`
* `unfocusedItemSaturation` - type: FLOAT
    - Sets the saturation for all items except the currently selected entry. This property takes precedence over `imageSaturation` if that has also been defined.
    - Minimum value is `0` (grayscale) and maximum value is `1` (original file saturation).
    - Default is `1`
* `unfocusedItemDimming` - type: FLOAT
    - Sets the dimming for all items except the currently selected entry.
    - Minimum value is `0` (pure black) and maximum value is `1` (no adjustment)
    - Default is `1`
* `imageFit` - type: STRING
    - Controls how to fit the image within the aspect ratio defined by `itemSize`. To scale and preserve the original aspect ratio, set the value to `contain`, to stretch/squash the image to fill the entire area set it to `fill` and to crop the image to fill the entire area set it to `cover`
    - Valid values are `contain`, `fill` or `cover`
    - Default is `contain`
* `imageRelativeScale` - type: FLOAT.
    - This property makes it possible to size the image defined by `staticImage`, `imageType` or `defaultImage` relative to the overall item size. This is mostly useful when combined with the `backgroundImage` and `selectorImage` properties.
    - Minimum value is `0.2` and maximum value is `1`
    - Default is `1`
* `imageColor` - type: COLOR
    - Applies a color shift to the images defined by `staticImage`, `imageType` or `defaultImage` by multiplying each pixel's color by this color value. For example, an all-white image with `FF0000` applied would become completely red. You can also control the transparency of the images by setting the value to for example `FFFFFFAA`. This keeps all pixels at their normal color and only affects the alpha channel. This property is applied after `imageSaturation` so by setting that property to `0` it's possible to colorize rather than color shift.
    - Default is `FFFFFFFF` (no color shift applied)
* `imageColorEnd` - type: COLOR
    - Works in the exact same way as `imageColor` but can be set as the end color to apply a color shift gradient.
    - Default is the same value as `imageColor`
* `imageGradientType` - type: STRING
    - The direction to apply the color gradient if both `imageColor` and `imageColorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `imageSelectedColor` - type: COLOR
    - Applies a color shift to the currently selected item's image as defined by `staticImage`, `imageType` or `defaultImage` by multiplying each pixel's color by this color value. For example, an all-white image with `FF0000` applied would become completely red. You can also control the transparency of the images by setting the value to for example `FFFFFFAA`. This keeps all pixels at their normal color and only affects the alpha channel. This property is applied after `imageSaturation` so by setting that property to `0` it's possible to colorize rather than color shift.
    - Default is the same value as `imageColor`
* `imageSelectedColorEnd` - type: COLOR
    - Works in the exact same way as `imageSelectedColor` but can be set as the end color to apply a color shift gradient.
    - Default is the same value as `imageSelectedColor`
* `imageSelectedGradientType` - type: STRING
    - The direction to apply the color gradient if both `imageSelectedColor` and `imageSelectedColorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `imageBrightness` - type: FLOAT
    - Controls the relative level of brightness. This is intended primarily for fine adjustments, for example if a color shift has been applied which may have lowered the overall brightness of the image.
    - Minimum value is `-2` and maximum value is `2`
    - Default is `0` (no brightness adjustments applied)
* `imageSaturation` - type: FLOAT
    - Controls the level of color saturation.
    - Minimum value is `0` (grayscale) and maximum value is `1` (original file saturation).
    - Default is `1`
* `backgroundImage` - type: PATH
    - Path to an optional background image file which will be displayed behind the image defined by `staticImage`, `imageType` or `defaultImage`. The aspect ratio for this image will not be preserved, it will be stretched or squashed to the aspect ratio set by `itemSize`. Most common extensions are supported (including .svg, .jpg, .png, and unanimated .gif).
* `backgroundRelativeScale` - type: FLOAT.
    - This property makes it possible to size the background relative to the overall item size. This is mostly useful when combined with the `selectorImage` property.
    - Minimum value is `0.2` and maximum value is `1`
    - Default is `1`
* `backgroundColor` - type: COLOR
    - Applies a color shift or draws a colored rectangle. If an image has been defined using the `backgroundImage` property then each pixel of that image is multiplied by this color value. For example, an all-white image with `FF0000` applied would become completely red. You can also control the transparency of the image by setting the value to for example `FFFFFFAA`. This keeps all pixels at their normal color and only affects the alpha channel. If no background image has been defined, then a colored rectangle will be drawn instead.
* `backgroundColorEnd` - type: COLOR
    - Works in the exact same way as `backgroundColor` but can be set as the end color to apply a color gradient.
    - Default is the same value as `backgroundColor`
* `backgroundGradientType` - type: STRING
    - The direction to apply the color gradient if both `backgroundColor` and `backgroundColorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `selectorImage` - type: PATH
    - Path to an optional selector image file which will be displayed for the currently selected item. The aspect ratio for this image will not be preserved, it will be stretched or squashed to the aspect ratio set by `itemSize`. Most common extensions are supported (including .svg, .jpg, .png, and unanimated .gif).
* `selectorRelativeScale` - type: FLOAT.
    - This property makes it possible to size the selector relative to the overall item size. This is mostly useful when combined with the `backgroundImage` property.
    - Minimum value is `0.2` and maximum value is `1`
    - Default is `1`
* `selectorLayer` - type: STRING
    - Defines at what layer position to place the selector. It can either be placed at the bottom, in the middle between the background and image/text or on top.
    - Valid values are `bottom`, `middle` or `top`
    - Default is `top`
* `selectorColor` - type: COLOR
    - Applies a color shift or draws a colored rectangle. If an image has been defined using the `selectorImage` property then each pixel of that image is multiplied by this color value. For example, an all-white image with `FF0000` applied would become completely red. You can also control the transparency of the image by setting the value to for example `FFFFFFAA`. This keeps all pixels at their normal color and only affects the alpha channel. If no selector image has been defined, then a colored rectangle will be drawn instead.
* `selectorColorEnd` - type: COLOR
    - Works in the exact same way as `selectorColor` but can be set as the end color to apply a color gradient.
    - Default is the same value as `selectorColor`
* `selectorGradientType` - type: STRING
    - The direction to apply the color gradient if both `selectorColor` and `selectorColorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `text` - type: STRING
    - A string literal to display if there is no `staticImage` or `defaultImage` property defined or if no image is found.
    - Default is the full system name.
    - This property can only be used in the `system` view as for the gamelist view the game name is always used as fallback.
* `textRelativeScale` - type: FLOAT.
    - This property makes it possible to size the text relative to the overall item size.
    - Minimum value is `0.2` and maximum value is `1`
    - Default is `1`
* `textColor` - type: COLOR
    - Default is `000000FF`
* `textBackgroundColor` - type: COLOR
    - Default is `FFFFFF00`
* `textSelectedColor` - type: COLOR
    - Sets the text color for the currently selected item.
    - Default is the same value as `textColor`
* `textSelectedBackgroundColor` - type: COLOR
    - Sets the text background color for the currently selected item.
    - Default is the same value as `textBackgroundColor`
* `fontPath` - type: PATH
    - Path to a TrueType font (.ttf) used as fallback if there is no `staticImage` / `imageType` image defined or found, and if `defaultImage` has not been defined.
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height for horizontally oriented screens or screen width for vertically oriented screens (e.g. for a value of `0.1`, the text's height would be 10% of the screen height). This calculation is based on the reference 'S' character so other glyphs may not fill this area, or they may exceed this area.
    - Minimum value is `0.001` and maximum value is `1.5`. Note that when running at a really low resolution, the minimum value may get clamped to a larger relative size.
    - Default is `0.045`
* `letterCase` - type: STRING
    - Sets the letter case for all entries.
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is `none` (original letter case is retained).
* `letterCaseAutoCollections` - type: STRING
    - Sets the letter case specifically for automatic collection entries (_all games_, _favorites_ and _last played_) which have their names spelled in lowercase by default.
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is the same value as `letterCase`
    - This property can only be used in the `system` view and it will take precedence over `letterCase` if that has also been defined.
* `letterCaseCustomCollections` - type: STRING
    - Sets the letter case specifically for custom collections entries. Be cautious about using this property as it will override whatever lettercase the user has defined for their custom collection names. This property takes precedence over `letterCase` if that has also been defined.
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is the same value as `letterCase`
* `lineSpacing` - type: FLOAT
    - Controls the space between lines (as a multiple of the font height). Due to the imprecise nature of typefaces where certain glyphs (characters) may exceed the requested font size, it's recommended to keep this value at around `1.1` or higher. This way overlapping glyphs or characters being cut off at the top or bottom will be prevented.
    - Minimum value is `0.5` and maximum value is `3`
    - Default is `1.5`
* `systemNameSuffix` - type: BOOLEAN
    - Whether to add the system name in square brackets after the game name when inside a collection system (automatic as well as custom collections). This assumes a fallback to text is made.
    - Default is `true`
    - This property can only be used in the `gamelist` view.
* `letterCaseSystemNameSuffix` - type: STRING
    - Sets the letter case for the system name suffix.
    - Valid values are `uppercase`, `lowercase` or `capitalize`
    - Default is `uppercase`
    - This property can only be used in the `gamelist` view and only when `systemNameSuffix` is `true`
* `fadeAbovePrimary` - type: BOOLEAN
    - When using fade transitions, all elements in the `system` view with a zIndex value higher than the grid are by default still rendered during transitions. If this property is enabled then all such elements will instead be faded out. Note that elements below the grid will be dimmed to black and elements above the grid will be faded to transparent.
    - Default is `false`
    - This property can only be used in the `system` view.
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `50`

#### textlist

A text list for navigating and selecting games or systems.

Supported views:
* `system`
* `gamelist`

Instances per view:
* `single`

Properties:
* `pos` - type: NORMALIZED_PAIR
    - Default is `0 0.1`
* `size` - type: NORMALIZED_PAIR
    - Minimum value per axis is `0.05` and maximum value per axis is `1`
    - Default is `1 0.8`
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the textlist exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `selectorHeight` - type: FLOAT
    - Height of the selector bar. This is expanded downwards so you'll probably want to adjust its position using `selectorVerticalOffset` if making use of this property.
    - Minimum value is `0` and maximum value is `1`
    - Default is 1.5 times the value defined by `fontSize`
* `selectorHorizontalOffset` - type: FLOAT
    - Allows moving of the selector bar left or right from its calculated position. Useful for fine tuning the selector bar position relative to the text.
    - Minimum value is `-1` and maximum value is `1`
    - Default is `0`
* `selectorVerticalOffset` - type: FLOAT
    - Allows moving of the selector bar up or down from its calculated position. Useful for fine tuning the selector bar position relative to the text.
    - Minimum value is `-1` and maximum value is `1`
    - Default is `0`
* `selectorColor` - type: COLOR
    - Color of the selector bar.
    - Default is `333333FF`
* `selectorColorEnd` - type: COLOR
    - Setting this to something other than what is defined for `selectorColor` creates a color gradient.
    - Default is `333333FF`
* `selectorGradientType` - type: STRING
    - The direction to apply the color gradient if both `selectorColor` and `selectorColorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `selectorImagePath` - type: PATH
    - Path to image to render in place of the selector bar.
* `selectorImageTile` - type: BOOLEAN
    - If true, the selector image will be tiled instead of stretched to fit its size.
    - Default is `false`
* `primaryColor` - type: COLOR
    - Color of the primary entry type. For the `gamelist` view this means file entries and for the `system` view it means system entries.
    - Default is `0000FFFF`
* `secondaryColor` - type: COLOR
    - Color of the secondary entry type. For the `gamelist` view this means folder entries and for the `system` view this property is not used.
    - Default is `00FF00FF`
* `selectedColor` - type: COLOR
    - Color of the highlighted entry for the primary entry type.
    - Default is the same value as `primaryColor`
* `selectedSecondaryColor` - type: COLOR
    - Color of the highlighted entry for the secondary entry type.
    - Default is the same value as `selectedColor`
* `selectedBackgroundColor` - type: COLOR
    - Background color of the highlighted entry for the primary entry type.
    - Default is `00000000`
* `selectedSecondaryBackgroundColor` - type: COLOR
    - Background color of the highlighted entry for the secondary entry type.
    - Default is the same value as `selectedBackgroundColor`
* `fontPath` - type: PATH
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height for horizontally oriented screens or screen width for vertically oriented screens (e.g. for a value of `0.1`, the text's height would be 10% of the screen height). This calculation is based on the reference 'S' character so other glyphs may not fill this area, or they may exceed this area.
    - Default is `0.045`
* `horizontalAlignment` - type: STRING
    - Controls alignment on the X axis.
    - Valid values are `left`, `center` or `right`
    - Default is `left`
* `horizontalMargin` - type: FLOAT
    - Horizontal offset for text from the alignment point. If `horizontalAlignment` is "left", offsets the text to the right. If `horizontalAlignment` is "right", offsets text to the left. No effect if `horizontalAlignment` is "center". Given as a percentage of the element's parent's width (same unit as `size`'s X value).
    - Default is `0`
* `letterCase` - type: STRING
    - Sets the letter case for all entries.
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is `none` (original letter case is retained).
* `letterCaseAutoCollections` - type: STRING
    - Sets the letter case specifically for automatic collection entries (_all games_, _favorites_ and _last played_) which have their names spelled in lowercase by default.
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is the same value as `letterCase`
    - This property can only be used in the `system` view and it will take precedence over `letterCase` if that has also been defined.
* `letterCaseCustomCollections` - type: STRING
    - Sets the letter case specifically for custom collections entries. Be cautious about using this property as it will override whatever lettercase the user has defined for their custom collection names. This property takes precedence over `letterCase` if that has also been defined.
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is the same value as `letterCase`
* `lineSpacing` - type: FLOAT
    - Controls the space between lines. This works a bit different for the textlist element compared to all other elements. In all other instances the line spacing is calculated in relation to the rasterized reference 'S' character. This will however not work for the textlist as there are no guarantees which sizes the rasterized characters may end up as. The nature of font rendering is simply not that static with glyphs being able to have any shape and size and linting/grid alignment being applied during font rasterization. Using the rasterized glyph size would be too imprecise and the spacing would be inconsistent across different display resolutions, possibly leading to a different number of textlist rows. Therefore this specific lineSpacing property is based on the defined font size regardless of what's actually being rasterized. This may seem confusing as some fonts greatly exceed the requested size, but if you simply adjust the spacing until the textlist looks correct it will look almost identical regardless of what display resolution is used.
    - Minimum value is `0.5` and maximum value is `3`
    - Default is `1.5`
* `indicators` - type: STRING
    - Controls the style of the indicators which get displayed when an entry is a favorite, a folder or a folderlink. If set to `none` it's strongly recommended to enable the corresponding badges as it would otherwise be very confusing for the user as there would be no way to discern this important information about each entry. The `symbols` value uses Font Awesome graphics to prefix the game name and `ascii` uses plain ASCII characters instead, as provided by the selected font. The latter sometimes looks better on "lo-fi" themes using pixelated fonts and similar. When using ASCII characters, favorites are marked as `*` folders as `#` and folderlinks as `>`
    - Valid values are `none`, `ascii` and `symbols`
    - Default is `symbols`
* `collectionIndicators` - type: STRING
    - Controls the style of the indicators which get displayed when editing a custom collection. This property can't be disabled as it's crucial for getting a visual overview when editing collections. When set to `ascii`, the indicator is displayed as a `!`
    - Valid values are `ascii` and `symbols`
    - Default is `symbols`
* `systemNameSuffix` - type: BOOLEAN
    - Whether to add the system name in square brackets after the game name when inside a collection system (automatic as well as custom collections).
    - Default is `true`
    - This property can only be used in the `gamelist` view.
* `letterCaseSystemNameSuffix` - type: STRING
    - Sets the letter case for the system name suffix.
    - This property can only be used in the `gamelist` view and only when `systemNameSuffix` is `true`
    - Valid values are `uppercase`, `lowercase` or `capitalize`
    - Default is `uppercase`
* `fadeAbovePrimary` - type: BOOLEAN
    - When using fade transitions, all elements in the `system` view with a zIndex value higher than the textlist are by default still rendered during transitions. If this property is enabled then all such elements will instead be faded out. Note that elements below the textlist will be dimmed to black and elements above the textlist will be faded to transparent.
    - Default is `false`
    - This property can only be used in the `system` view.
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `50`

### Secondary elements

Elements from this group can occur an unlimited number of times and they take care of displaying the bulk of the theme configuration such as text, images, videos, animations etc.

#### image

Displays a raster image or a scalable vector graphics (SVG) image.

Supported views:
* `system `
* `gamelist`

Instances per view:
* `unlimited`

Properties:
* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - If only one axis is specified (and the other is zero), then the other axis will be automatically calculated in accordance with the image's aspect ratio. Setting both axes to 0 is an error and the size will be clamped to `0.001 0.001` in this case. This property takes precedence over `maxSize` and `cropSize` if either or both of those have also been defined.
    - Minimum value per axis is `0.001` and maximum value per axis is `3`. If specifying a value outside the allowed range then no attempt will be made to preserve the aspect ratio.
* `maxSize` - type: NORMALIZED_PAIR
    - The image will be resized as large as possible so that it fits within this size while maintaining its aspect ratio. Use this instead of `size` when you don't know what kind of image you're using so it doesn't get grossly oversized on one axis. This property takes precedence over `cropSize` if that has also been defined.
    - Minimum value per axis is `0.001` and maximum value per axis is `3`
* `cropSize` - type: NORMALIZED_PAIR
    - The image will be resized and cropped to the exact size defined by this property while maintaining its aspect ratio. The crop is always applied centered.
    - Minimum value per axis is `0.001` and maximum value per axis is `3`
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the image should be rotated. Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the image will be rotated.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0.5 0.5`
* `flipHorizontal` - type: BOOLEAN
    - Flips the image texture horizontally.
    - Default is `false`
* `flipVertical` - type: BOOLEAN
    - Flips the image texture vertically.
    - Default is `false`
* `path` - type: PATH
    - Explicit path to an image file. Most common extensions are supported (including .jpg, .png, and unanimated .gif). If `imageType` is also defined then this will take precedence as these two properties are not intended to be used together. If you need a fallback image in case of missing game media, use the `default` property instead.
* `default` - type: PATH
    - Path to a default image file. The default image will be displayed when the selected game does not have an image of the type defined by the `imageType` property (i.e. this `default` property does nothing unless a valid `imageType` property has been set). It's also applied to any custom collection that does not contain any games when browsing the grouped custom collections system.
* `imageType` - type: STRING
    - This displays a game image of a certain media type. Multiple types can be defined, in which case the entries should be delimited by commas or by whitespace characters (tabs, spaces or line breaks). The media will be searched for in the order that the entries have been defined. If no image is found, then the space will be left blank unless the `default` property has been set. To use this property from the `system` view, you will first need to add a `gameselector` element. Defining duplicate values is considered an error and will result in the property getting ignored.
    - Valid values:
    - `image` - This will look for a `miximage`, and if that is not found `screenshot` is tried next, then `titlescreen` and finally `cover`. This is just a convenient shortcut and it's equivalent to explicitly defining `miximage, screenshot, titlescreen, cover`
    - `miximage` - This will look for a miximage.
    - `marquee` - This will look for a marquee (wheel) image.
    - `screenshot` - This will look for a screenshot image.
    - `titlescreen` - This will look for a title screen image.
    - `cover` - This will look for a box front cover image.
    - `backcover` - This will look for a box back cover image.
    - `3dbox` - This will look for a 3D box image.
    - `physicalmedia` - This will look for a physical media image.
    - `fanart` - This will look for a fan art image.
* `metadataElement` - type: BOOLEAN
    - By default game metadata and media are faded out during gamelist fast-scrolling and text metadata fields, ratings and badges are hidden when enabling the _Hide metadata fields_ setting for a game entry. Using this property it's possible to explicitly define additional image elements that should be treated as if they were game media files. This is for example useful for hiding and fading out image elements that are used as indicator icons for the various metadata types like genre, publisher, players etc. It's however not possible to do the opposite, i.e. to disable this functionality for the default game media types as that would break basic application behavior.
    - Default is `false`
* `gameselector` - type: STRING
    - If more than one gameselector element has been defined, this property makes it possible to state which one to use. If multiple gameselector elements have been defined and this property is missing then the first entry will be chosen and a warning message will be logged. If only a single gameselector has been defined, this property is ignored. The value of this property must match the `name` attribute value of the gameselector element. This property is only needed for the `system` view and only if the `imageType` property is utilized.
* `gameselectorEntry` - type: UNSIGNED_INTEGER
    - This optional property which is only available in the `system` view makes it possible to select which `gameselector` entry to use to populate the `imageType` property. This assumes that a `gameCount` property for the gameselector element has been defined with a value higher than `1`. By defining multiple `image` elements with different values for the `gameselectorEntry` property it's possible to display multiple game entries at the same time, for example listing a couple of games that were last played, or a selection of random games. If the requested entry does not exist (for instance if `gameCount` has been set to 5 and `gameselectorEntry` has been set to `4` but the system only contains 3 games), then the overall element will not get rendered. Note that the first entry is defined as `0`, the second entry as `1` etc.
    - Minimum value is `0` and maximum value is the value of the `gameselector` element property `gameCount` minus 1. If a value outside this range is defined, then it will be automatically clamped to a valid value.
    - Default is `0`
* `tile` - type: BOOLEAN
    - If true, the image will be tiled instead of stretched to fit its size. Useful for backgrounds. Do not combine with the `maxSize` or `cropSize` properties, instead always use `size` when tiling.
    - Default is `false`
* `tileSize` - type: NORMALIZED_PAIR
    - Size of the individual images making up the tile as opposed to the overall size for the element which is defined by the `size` property. If only one axis is specified (and the other is zero), then the other axis will be automatically calculated in accordance with the image's aspect ratio. Setting both axes to 0 is an error and tiling will be disabled in this case. If this property is omitted, then the size will be set to the actual image dimensions. For SVG images this means whatever canvas size has been defined inside the file.
    - Minimum value per axis is `0` and maximum value per axis is `1`.
* `tileHorizontalAlignment` - type: STRING
    - If the images making up the tiled texture do not match precisely with the edges of the overall element, then this property can be used to define the alignment on the horizontal axis.
    - Valid values are `left` or `right`
    - Default is `left`
* `tileVerticalAlignment` - type: STRING
    - If the images making up the tiled texture do not match precisely with the edges of the overall element, then this property can be used to define the alignment on the vertical axis.
    - Valid values are `top` or `bottom`
    - Default is `bottom`
* `interpolation` - type: STRING
    - Interpolation method to use when scaling. Nearest neighbor (`nearest`) preserves sharp pixels and linear filtering (`linear`) makes the image smoother. This property has limited effect on scalable vector graphics (SVG) images unless rotation is applied.
    - Valid values are `nearest` or `linear`
    - Default is `nearest`
* `color` - type: COLOR
    - Applies a color shift to the image by multiplying each pixel's color by this color value. For example, an all-white image with `FF0000` applied would become completely red. You can also control the transparency of the image by setting the value to for example `FFFFFFAA`. This keeps all pixels at their normal color and only affects the alpha channel. This property is applied after `saturation` so by setting that property to `0` it's possible to colorize rather than color shift.
    - Default is `FFFFFFFF` (no color shift applied)
* `colorEnd` - type: COLOR
    - Works in the exact same way as `color` but can be set as the end color to apply a color shift gradient.
    - Default is the same value as `color`
* `gradientType` - type: STRING
    - The direction to apply the color shift gradient if both `color` and `colorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `scrollFadeIn` - type: BOOLEAN
    - If enabled, a short fade-in animation will be applied when scrolling through games in the gamelist view. This usually looks best if used for the main game image.
    - Default is `false`
* `brightness` - type: FLOAT
    - Controls the relative level of brightness. This is intended primarily for fine adjustments, for example if a color shift has been applied which may have lowered the overall brightness of the image.
    - Minimum value is `-2` and maximum value is `2`
    - Default is `0` (no brightness adjustments applied)
* `opacity` - type: FLOAT
    - Controls the level of transparency. If set to `0` the element will be disabled.
    - Minimum value is `0` and maximum value is `1`
    - Default is `1`
* `saturation` - type: FLOAT
    - Controls the level of color saturation.
    - Minimum value is `0` (grayscale) and maximum value is `1` (original file saturation).
    - Default is `1`
* `visible` - type: BOOLEAN
    - If set to false, the element will be disabled. This is equivalent to setting `opacity` to `0`
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `30`

#### video

Plays a video and provides support for displaying a static image for a defined time period before starting the video player. Although an unlimited number of videos could in theory be defined per view it's recommended to keep it at a single instance as playing videos takes a lot of CPU resources. But if still going for multiple videos, make sure to use the `audio` property to disable audio on all but one video as ES-DE currently has no audio mixing capabilities so the sound would not play correctly. To use videos in the `system` view, you either need to set a static video using the `path` property, or you need to create a `gameselector` element so game videos can be used.

Supported views:
* `system `
* `gamelist`

Instances per view:
* `unlimited` (but recommended to keep at a single instance)

Properties:
* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - If only one axis is specified (and the other is zero), then the other axis will be automatically calculated in accordance with the static image's aspect ratio and the video's aspect ratio. Setting both axes to 0 is an error and the size will be clamped to `0.01 0.01` in this case. This property takes precedence over `maxSize` and `cropSize` if either or both of those have also been defined.
    - Minimum value per axis is `0.01` and maximum value per axis is `2`. If specifying a value outside the allowed range then no attempt will be made to preserve the aspect ratio.
* `maxSize` - type: NORMALIZED_PAIR
    - The static image and video will be resized as large as possible so that they fit within this size while maintaining their aspect ratios. Use this instead of `size` when you don't know what kind of video you're using so it doesn't get grossly oversized on one axis. This property takes precedence over `cropSize` if that has also been defined.
    - Minimum value per axis is `0.01` and maximum value per axis is `2`
* `cropSize` - type: NORMALIZED_PAIR
    - The static image and video will be resized and cropped to the exact size defined by this property while maintaining their aspect ratios. The crop is always applied centered. Can't be combined with the `scanlines` property.
    - Minimum value per axis is `0.01` and maximum value per axis is `2`
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `path` - type: PATH
    - Path to a video file. Setting a value for this property will make the video static, i.e. any `imageType`, `gameselector` and `default` properties will be ignored.
* `default` - type: PATH
    - Path to a default video file. The default video will be played when the selected game does not have a video. This property is also applied to any custom collection that does not contain any games when browsing the grouped custom collections system. Takes precedence over `defaultImage`.
* `defaultImage` - type: PATH
    - Path to a default image file. If the `imageType` property has a value set, then the default image will be displayed if the selected game does not have an image for any of the defined types. If `imageType` is not defined, then the default image will be shown if there is no video file found and if `default` has not been set. This property is also applied to any custom collection that does not contain any games when browsing the grouped custom collections system.
* `imageType` - type: STRING
    - This displays a game image of a certain media type, either before the video starts to play if `delay` is set to a non-zero value, or if there is no video file found and `default` has not been defined. Multiple types can be defined, in which case the entries should be delimited by commas or by whitespace characters (tabs, spaces or line breaks). The media will be searched for in the order that the entries have been defined. If no image is found, then the space will be left blank unless either the `default` or `defaultImage` properties have been set. To use this property from the `system` view, you will first need to add a `gameselector` element. If `delay` is set to zero, then this property has no effect. Defining duplicate values is considered an error and will result in the property getting ignored.
    - Valid values:
    - `image` - This will look for a `miximage`, and if that is not found `screenshot` is tried next, then `titlescreen` and finally `cover`. This is just a convenient shortcut and it's equivalent to explicitly defining `miximage, screenshot, titlescreen, cover`
    - `miximage` - This will look for a miximage.
    - `marquee` - This will look for a marquee (wheel) image.
    - `screenshot` - This will look for a screenshot image.
    - `titlescreen` - This will look for a title screen image.
    - `cover` - This will look for a box front cover image.
    - `backcover` - This will look for a box back cover image.
    - `3dbox` - This will look for a 3D box image.
    - `physicalmedia` - This will look for a physical media image.
    - `fanart` - This will look for a fan art image.
* `metadataElement` - type: BOOLEAN
    - By default game metadata and media are faded out during gamelist fast-scrolling and text metadata fields, ratings and badges are hidden when enabling the _Hide metadata fields_ setting for a game entry. Using this property it's possible to explicitly define static video elements that should be treated as if they were game media files. This property is ignored if `path` is not set.
    - Default is `false`
* `gameselector` - type: STRING
    - If more than one gameselector element has been defined, this property makes it possible to state which one to use. If multiple gameselector elements have been defined and this property is missing then the first entry will be chosen and a warning message will be logged. If only a single gameselector has been defined, this property is ignored. The value of this property must match the `name` attribute value of the gameselector element.
* `gameselectorEntry` - type: UNSIGNED_INTEGER
    - This optional property which is only available in the `system` view makes it possible to select which `gameselector` entry to use to populate the `imageType` property and to use for playing the video stream. This assumes that a `gameCount` property for the gameselector element has been defined with a value higher than `1`. By defining multiple `video` elements with different values for the `gameselectorEntry` property it's possible to display multiple game entries at the same time, for example listing a couple of games that were last played, or a selection of random games. If the requested entry does not exist (for instance if `gameCount` has been set to 5 and `gameselectorEntry` has been set to `4` but the system only contains 3 games), then the overall element will not get rendered. Note that the first entry is defined as `0`, the second entry as `1` etc.
    - Minimum value is `0` and maximum value is the value of the `gameselector` element property `gameCount` minus 1. If a value outside this range is defined, then it will be automatically clamped to a valid value.
    - Default is `0`
* `audio` - type: BOOLEAN
    - Whether to enable or disable audio playback for the video. For static videos in the gamelist view it's strongly recommended to set this to `false` if there is also a separate video element playing game videos.
    - Default is `true`
* `interpolation` - type: STRING
    - Interpolation method to use when scaling raster images. Nearest neighbor (`nearest`) preserves sharp pixels and linear filtering (`linear`) makes the image smoother. Note that this property only affects the static image, not the video scaling. This property also has no effect on scalable vector graphics (SVG) images.
    - Valid values are `nearest` or `linear`
    - Default is `nearest`
* `color` - type: COLOR
    - Applies a color shift to both the static image and video by multiplying each pixel's color by this color value. For example, an all-white image or video with `FF0000` applied would become completely red. It's however not recommended to use this property to control opacity as this will not look right for actual videos, instead use the `opacity` property if you want to render this element as semi-transparent. The `color` property is applied after `saturation` so by setting that property to `0` it's possible to colorize rather than color shift.
    - Default is `FFFFFFFF` (no color shift applied)
* `colorEnd` - type: COLOR
    - Works in the exact same way as `color` but can be set as the end color to apply a color shift gradient.
    - Default is the same value as `color`
* `gradientType` - type: STRING
    - The direction to apply the color shift gradient if both `color` and `colorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `pillarboxes` - type: BOOLEAN
    - Whether to render black pillarboxes (and to a lesses extent letterboxes) for videos with aspect ratios where this is applicable. This is for instance useful for arcade game videos in vertical orientation.
    - Default is `true`
* `pillarboxThreshold` - type: NORMALIZED_PAIR
    - Normally it doesn't look very good to add really narrow pillarboxes or letterboxes, so by default they are skipped if the actual video size is not reaching a threshold value as compared to the overall defined video area size. By modifying this property it's possible to control that threshold, as for some theme designs it will look better with the consistency of always rendering the pillarboxes/letterboxes even if they are narrow. To clarify, the default X axis value of 0.85 means that if the video width is 85% or less as compared to the X axis defined by the `size` property, then pillarboxes will be rendered. So setting the `pillarboxThreshold` value to `1 1` will always apply pillarboxes/letterboxes regardless of the video file dimension.
    - Minimum value per axis is `0.2` and maximum value per axis is `1`
    - Default is `0.85 0.90`
* `scanlines` - type: BOOLEAN
    - Whether to use a shader to render scanlines. Can't be combined with the `cropSize` property.
    - Default is `false`
* `delay` - type: FLOAT
    - Delay in seconds before video will start playing. During the delay period the game image defined via the `imageType` property will be displayed. If that property is not set, then the `delay` property will be ignored.
    - Minimum value is `0` and maximum value is `15`
    - Default is `1.5`
* `fadeInTime` - type: FLOAT
    - Time in seconds to fade in the video from pure black. This is completely unrelated to the `scrollFadeIn` property. Note that if this is set to zero it may seem as if the property doesn't work correctly as many ScreenScraper videos have a fade-in baked into the actual video stream. Setting this property to lower than 0.3 seconds or so is generally a bad idea for videos that don't have a fade-in baked in as transitions from the static image will then look like a bad jump cut.
    - Minimum value is `0` and maximum value is `8`
    - Default is `1`
* `scrollFadeIn` - type: BOOLEAN
    - If enabled, a short fade-in animation will be applied when scrolling through games in the gamelist view. This animation is only applied to images and not to actual videos, so if no image metadata has been defined then this property has no effect. For this to work correctly the `delay` property also needs to be set.
    - Default is `false`
* `brightness` - type: FLOAT
    - Controls the relative level of brightness. This affects both the static image and the video stream. This is intended primarily for fine adjustments, for example if a color shift has been applied which may have lowered the overall brightness of the image/video.
    - Minimum value is `-2` and maximum value is `2`
    - Default is `0` (no brightness adjustments applied)
* `opacity` - type: FLOAT
    - Controls the level of transparency. This affects both the static image and the video stream. If set to `0` the element will be disabled.
    - Minimum value is `0` and maximum value is `1`
    - Default is `1`
* `saturation` - type: FLOAT
    - Controls the level of color saturation. This affects both the static image and the video stream.
    - Minimum value is `0` (grayscale) and maximum value is `1` (original file saturation).
    - Default is `1`
* `visible` - type: BOOLEAN
    - If set to false, the element will be disabled. This is equivalent to setting `opacity` to `0`
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `30`

#### animation

GIF and Lottie (vector graphics) animations. The animation type is automatically selected based on the file extension with `.gif` for GIF animations and `.json` for Lottie animations. Note that Lottie animations take a lot of memory and CPU resources if scaled up to large sizes so it's adviced to not add too many of these to the same view and to not make them too large. GIF animations on the other hand are not as demanding except if they're really long and/or of high resolution.

Also be aware that the [rlottie](https://github.com/Samsung/rlottie) library used by ES-DE is not compatible with all Lottie animations out there so you may need to convert them to a format that rlottie can read, or use some other animations altogether.

Supported views:
* `system `
* `gamelist`

Instances per view:
* `unlimited`

Properties:
* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - If only one axis is specified (and the other is zero), then the other will be automatically calculated in accordance with the animation's aspect ratio. Note that this is sometimes not entirely accurate as some animations contain invalid size information. Setting both axes to 0 is an error and the size will be clamped to `0.01 0.01` in this case. This property takes precedence over `maxSize` if both properties are defined.
    - Minimum value per axis is `0.01` and maximum value per axis is `1`. If specifying a value outside the allowed range then no attempt will be made to preserve the aspect ratio.
* `maxSize` - type: NORMALIZED_PAIR
    - The animation will be resized as large as possible so that it fits within this size while maintaining its aspect ratio. Note that this is sometimes not entirely accurate as some animations contain invalid size information.
    - Minimum value per axis is `0.01` and maximum value per axis is `1`
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the animation should be rotated. Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the animation will be rotated.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0.5 0.5`
* `metadataElement` - type: BOOLEAN
    - By default game metadata and media are faded out during gamelist fast-scrolling and text metadata fields, ratings and badges are hidden when enabling the _Hide metadata fields_ setting for a game entry. Using this property it's possible to explicitly define animation elements that should be treated as if they were game media files. This is for example useful for hiding and fading out animations that are used as indicators for the various metadata types like genre, publisher, players etc.
    - Default is `false`
* `path` - type: PATH
    - Path to the animation file. Only .gif and .json extensions are supported.
* `speed` - type: FLOAT.
    - The relative speed at which to play the animation.
    - Minimum value is `0.2` and maximum value is `3`
    - Default is `1`
* `direction` - type: STRING
    - The direction that the animation should be played.
    - Valid values are `normal` (forwards), `reverse` (backwards), `alternate` (bouncing forwards/backwards) and `alternateReverse` (bouncing backwards/forwards, i.e. starting with playing backwards).
    - Default is `normal`
* `iterationCount` - type: UNSIGNED_INTEGER
    - Number of times to play the animation until next time it's reset. Animation resets are triggered by various events like navigating between systems and gamelists, reloading a gamelist etc.
    - Minimum value is `0` and maximum value is `10`
    - Default is `0` (infinite amount of times)
* `interpolation` - type: STRING
    - Interpolation method to use when scaling. Nearest neighbor (`nearest`) preserves sharp pixels and linear filtering (`linear`) makes the image smoother.
    - Valid values are `nearest` or `linear`
    - Default is `nearest`
    - This property can only be used for GIF animations.
* `color` - type: COLOR
    - Applies a color shift to the animation by multiplying each pixel's color by this color value. For example, an all-white animation with `FF0000` applied would become completely red. You can also control the transparency of the animation by setting the value to for example `FFFFFFAA`. This keeps all pixels at their normal color and only affects the alpha channel. This property is applied after `saturation` so by setting that property to `0` it's possible to colorize rather than color shift.
    - Default is `FFFFFFFF` (no color shift applied)
* `colorEnd` - type: COLOR
    - Works in the exact same way as `color` but can be set as the end color to apply a color shift gradient.
    - Default is the same value as `color`
* `gradientType` - type: STRING
    - The direction to apply the color shift gradient if both `color` and `colorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `brightness` - type: FLOAT
    - Controls the relative level of brightness. This is intended primarily for fine adjustments.
    - Minimum value is `-2` and maximum value is `2`
    - Default is `0` (no brightness adjustments applied)
* `opacity` - type: FLOAT
    - Controls the level of transparency. If set to `0` the element will be disabled.
    - Minimum value is `0` and maximum value is `1`
    - Default is `1`
* `saturation` - type: FLOAT
    - Controls the level of color saturation.
    - Minimum value is `0` (grayscale) and maximum value is `1` (original file saturation).
    - Default is `1`
* `visible` - type: BOOLEAN
    - If set to false, the element will be disabled. This is equivalent to setting `opacity` to `0`
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `35`

#### badges

Displays graphical symbols representing a number of metadata fields for the currently selected game. It's strongly recommended to use the same image dimensions for all badges as varying aspect ratios will lead to alignment issues. For the controller images it's recommended to keep to the square canvas size used by the default bundled graphics as otherwise sizing and placement will be inconsistent (unless all controller graphic files are customized of course).

Supported views:
* `gamelist`

Instances per view:
* `unlimited`

Properties:
* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - Possible combinations:
    - `w h` - Dimensions of the badges container. The badges will be scaled to fit within these dimensions.
    - Minimum value per axis is `0.03` and maximum value per axis is `1`
    - Default is `0.15 0.20`
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the badges should be rotated. Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the image will be rotated.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0.5 0.5`.
* `horizontalAlignment` - type: STRING.
    - Valid values are `left`, `center` or `right`
* `direction` - type: STRING
    - Controls the primary layout direction (line axis) for the badges. Lines will fill up in the specified direction.
    - Valid values are `row` or `column`
    - Default is `row`
* `lines` - type: UNSIGNED_INTEGER
    - The number of lines available.
    - Default is `3`
* `itemsPerLine` - type: UNSIGNED_INTEGER
    - Number of badges that fit on a line. When more badges are available a new line will be started.
    - Default is `4`
* `itemMargin` - type: NORMALIZED_PAIR
    - The horizontal and vertical margins between badges. If one of the axis is set to `-1` then the margin of the other axis (in pixels) will be used, which makes it possible to get identical spacing between all items regardless of the screen aspect ratio.
    - Minimum value per axis is `0` and maximum value per axis is `0.2`
    - Default is `0.01 0.01`.
* `slots` - type: STRING
    - The badge types that should be displayed. Specified as a list of strings delimited by commas or by whitespace characters (tabs, spaces or line breaks). The order in which they are defined will be followed when placing badges on screen. Available badges are:
    - `collection` - Will be shown when editing a custom collection and the current entry is part of that collection.
    - `folder` - Will be shown when the current entry is a folder. If a folder link has been setup, then a configurable link icon will overlay this badge.
    - `favorite` - Will be shown when the game is marked as favorite.
    - `completed` - Will be shown when the game is marked as completed.
    - `kidgame` - Will be shown when the game is marked as a kids game.
    - `broken` - Will be shown when the game is marked as broken.
    - `controller` - Will be shown and overlaid by the corresponding controller icon if a controller type has been selected for the game (using the metadata editor or via scraping).
    - `altemulator` - Will be shown when an alternative emulator is setup for the game.
    - `all` - Including this value will enable all badges. If some badges have been added already they will be shown in the order they were defined and the remaining ones will be added at the end, in the order listed above. Using the `all` value can be used as a way to future-proof the theme, because if additional badges are added in future ES-DE releases, no theme updates would be needed to accomodate them. Just make sure to include space for a few extra badges in the layout, and increase the `lines` and `itemsPerLine` accordingly.
* `controllerPos` - type: NORMALIZED_PAIR
    - The position of the controller icon relative to the parent `controller` badge.
    - Minimum value per axis is `-1` and maximum value per axis is `2`
    - Default is `0.5 0.5` which centers the controller icon on the badge.
* `controllerSize` - type: FLOAT
    - The size of the controller icon relative to the parent `controller` badge.
    - Setting the value to `1` sizes the icon to the same width as the parent badge. The image aspect ratio is always maintained.
    - Minimum value is `0.1` and maximum value is `2`
    - Default is `0.5`
* `customBadgeIcon` - type: PATH
    - A badge icon override. Specify the badge type in the attribute `badge`. The available badges are the ones listed above.
* `customControllerIcon` - type: PATH
    - A controller icon override. Specify the controller type in the attribute `controller`.
    - These are the available types:
    - `gamepad_generic`,
    `gamepad_nintendo_nes`,
    `gamepad_nintendo_snes`,
    `gamepad_nintendo_64`,
    `gamepad_nintendo_gamecube`,
    `gamepad_playstation`,
    `gamepad_sega_master_system`,
    `gamepad_sega_md_3_buttons`,
    `gamepad_sega_md_6_buttons`,
    `gamepad_sega_dreamcast`,
    `gamepad_xbox`,
    `joystick_generic`,
    `joystick_arcade_no_buttons`,
    `joystick_arcade_no_buttons_twin`,
    `joystick_arcade_1_button`,
    `joystick_arcade_2_buttons`,
    `joystick_arcade_3_buttons`,
    `joystick_arcade_4_buttons`,
    `joystick_arcade_5_buttons`,
    `joystick_arcade_6_buttons`,
    `keyboard_generic`,
    `keyboard_and_mouse_generic`,
    `mouse_generic`,
    `mouse_amiga`,
    `lightgun_generic`,
    `lightgun_nintendo`,
    `steering_wheel_generic`,
    `flight_stick_generic`,
    `spinner_generic`,
    `trackball_generic`,
    `wii_remote_nintendo`,
    `wii_remote_and_nunchuk_nintendo`,
    `joycon_left_or_right_nintendo`,
    `joycon_pair_nintendo`,
    `xbox_kinect`,
    `unknown`
* `folderLinkPos` - type: NORMALIZED_PAIR
    - The position of the folder link icon relative to the parent `folder` badge.
    - Minimum value per axis is `-1` and maximum value per axis is `2`
    - Default is `0.5 0.5` which centers the folder link icon on the badge.
* `folderLinkSize` - type: FLOAT
    - The size of the folder link icon relative to the parent `folder` badge.
    - Setting the value to `1` sizes the icon to the same width as the parent badge. The image aspect ratio is always maintained.
    - Minimum value is `0.1` and maximum value is `1`
    - Default is `0.5`
* `customFolderLinkIcon` - type: PATH
    - Folder link icon override.
* `badgeIconColor` - type: COLOR
    - Applies a color shift to the badge icon by multiplying each pixel's color by this color value. For example, an all-white image with `FF0000` applied would become completely red. You can also control the transparency of the image by setting the value to for example `FFFFFFAA`. This keeps all pixels at their normal color and only affects the alpha channel.
    - Default is `FFFFFFFF` (no color shift applied)
* `badgeIconColorEnd` - type: COLOR
    - Works in the exact same way as `badgeIconColor` but can be set as the end color to apply a color shift gradient.
    - Default is the same value as `badgeIconColor`
* `badgeIconGradientType` - type: STRING
    - The direction to apply the color shift gradient if both `badgeIconColor` and `badgeIconColorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `controllerIconColor` - type: COLOR
    - Applies a color shift to the controller icon by multiplying each pixel's color by this color value. For example, an all-white image with `FF0000` applied would become completely red. You can also control the transparency of the image by setting the value to for example `FFFFFFAA`. This keeps all pixels at their normal color and only affects the alpha channel.
    - Default is `FFFFFFFF` (no color shift applied)
* `controllerIconColorEnd` - type: COLOR
    - Works in the exact same way as `controllerIconColor` but can be set as the end color to apply a color shift gradient.
    - Default is the same value as `controllerIconColor`
* `controllerIconGradientType` - type: STRING
    - The direction to apply the color shift gradient if both `controllerIconColor` and `controllerIconColorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `folderLinkIconColor` - type: COLOR
    - Applies a color shift to the folder link icon by multiplying each pixel's color by this color value. For example, an all-white image with `FF0000` applied would become completely red. You can also control the transparency of the image by setting the value to for example `FFFFFFAA`. This keeps all pixels at their normal color and only affects the alpha channel.
    - Default is `FFFFFFFF` (no color shift applied)
* `folderLinkIconColorEnd` - type: COLOR
    - Works in the exact same way as `folderLinkIconColor` but can be set as the end color to apply a color shift gradient.
    - Default is the same value as `folderLinkIconColor`
* `folderLinkIconGradientType` - type: STRING
    - The direction to apply the color shift gradient if both `folderLinkIconColor` and `folderLinkIconColorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `opacity` - type: FLOAT
    - Controls the level of transparency. If set to `0` the element will be disabled.
    - Minimum value is `0` and maximum value is `1`
    - Default is `1`
* `visible` - type: BOOLEAN
    - If set to false, the element will be disabled. This is equivalent to setting `opacity` to `0`
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `35`

#### text

Displays text. This can be literal strings or values based on game metadata or system variables, as described below. For the `gamelist` view it's also possible to place the text inside a scrollable container which is for example useful for longer texts like game descriptions.

Supported views:
* `system`
* `gamelist`

Instances per view:
* `unlimited`

Properties:
* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - Possible combinations:
    - `0 0` - automatically size so text fits on one line (expanding horizontally).
    - `w 0` - automatically wrap text so it doesn't go beyond `w` (expanding vertically).
    - `w h` - works like a "text box". If `h` is non-zero and `h` <= `fontSize` (implying it should be a single line of text), text that goes beyond `w` will be truncated with an elipses (...).
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the text should be rotated. Positive values will rotate clockwise, negative values will rotate counterclockwise. Rotation is not possible if the `container` property has been set to true.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the text will be rotated.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0.5 0.5`
* `text` - type: STRING
    - A string literal to display.
* `systemdata` - type: STRING
    - This translates to some system data including values defined in es_systems.xml as well as some statistics.
    - This property can only be used in the `system` view and you can only define a single value per element.
    - Valid values:
    - `name` - Short system name as defined in es_systems.xml.
    - `fullname` - Full system name as defined in es_systems.xml.
    - `gamecount` - Number of games available for the system. Number of favorites is printed inside brackets if applicable.
    - `gamecountGames` - Number of games available for the system. Does not include the favorites count.
    - `gamecountGamesNoText` - Same as the above but with the text _game_ or _games_ omitted, i.e. only the number is shown.
    - `gamecountFavorites` - Number of favorite games for the system, may be blank if favorites is not applicable.
    - `gamecountFavoritesNoText` - Same as the above but with the text _favorite_ or _favorites_ omitted, i.e. only the number is shown.
* `metadata` - type: STRING
    - This translates to the metadata values that are available for the game. To use this property from the `system` view, you will first need to add a `gameselector` element. You can only define a single metadata value per text element.
     - Valid values:
    - `name` - Game name.
    - `description` - Game description. Should be combined with the `container` property in most cases.
    - `rating` - The numerical representation of the game rating, for example `3` or `4.5`
    - `developer` - Developer.
    - `publisher` - Publisher.
    - `genre` - Genre.
    - `players` - The number of players.
    - `favorite` - Whether the game is a favorite. Will be printed as either `yes` or `no`
    - `completed` - Whether the game has been completed. Will be printed as either `yes` or `no`
    - `kidgame` - Whether the game is suitable for children. Will be printed as either `yes` or `no`
    - `broken` - Whether the game is broken/not working. Will be printed as either `yes` or `no`
    - `playcount` - How many times the game has been played.
    - `controller` - The controller for the game. Will be blank if none has been selected.
    - `altemulator` - The alternative emulator for the game. Will be blank if none has been selected.
    - `emulator` - The emulator used to launch the game, could as such be a per-game alternative emulator entry, a system wide alternative emulator entry or the system's default emulator. This requires that the command tag in es_systems.xml has a label defined, otherwise this value will be blank. Folders will always have blank values as these can't be launched directly.
    - `systemName` - The short system name of the game.
    - `systemFullname` - The full system name of the game.
    - `sourceSystemName` - The source short system name of the game. For regular systems this value will be identical to `systemName` but for collections it will show the actual system that the game is located in instead of the collection system name.
    - `sourceSystemFullname` - The source full system name of the game. For regular systems this value will be identical to `systemFullname` but for collections it will show the actual system that the game is located in instead of the collection system name.
* `defaultValue` - type: STRING
    - This property makes it possible to override the default "unknown" text that is displayed if `metadata` has been set to `developer`, `publisher`, `genre` or `players` and there is no metadata available for the defined type. Any string can be used but you can't set it to a blank value. If you don't want to display anything when there is no metadata available, then set this property to `:space:` in which case a blankspace will be used. This property has no effect on the metadata editor where "unknown" will still be shown for blank values.
* `systemNameSuffix` - type: BOOLEAN
    - Whether to add the system name in square brackets after the game name when inside a collection system (automatic as well as custom collections). If `metadata` has been set to `description` then this property will only apply when inside the root of the grouped custom collections system where a summary of available games for the currently selected collection is displayed.
    - Default is `true`
    - This property can only be used when `metadata` has been set to `name` or `description`
* `letterCaseSystemNameSuffix` - type: STRING
    - Sets the letter case for the system name suffix.
    - Valid values are `uppercase`, `lowercase` or `capitalize`
    - Default is `uppercase`
    - This property can only be used when `systemNameSuffix` is `true`, and if `metadata` has been set to `description` then it only applies if `letterCase` is also set to `none`
* `metadataElement` - type: BOOLEAN
    - By default game metadata and media are faded out during gamelist fast-scrolling. They are also hidden when enabling the _Hide metadata fields_ setting in the metadata editor. This includes the text metadata fields (except `systemName`, `systemFullname`, `sourceSystemName` and `sourceSystemFullname`), ratings and badges. Using this property it's possible to explicitly define additional text elements that should be treated as if they were game metadata entries. This is for example useful for hiding and fading out text labels or icons for the various metadata types like genre, publisher, players etc. Note that it's not possible to disable the metadata hiding functionality for the default metadata fields as that would break basic application behavior. Also note that there is a slight exception to the hiding logic for text containers with the metadata value set to `description`. In this case the element is by default not hidden when enabling the _Hide metadata fields_ setting. To also hide such containers, set this property to true.
    - Default is `false`
* `gameselector` - type: STRING
    - If more than one gameselector element has been defined, then this property makes it possible to state which one to use. If multiple gameselector elements have been defined and this property is missing then the first entry will be chosen and a warning message will be logged. If only a single gameselector has been defined, this property is ignored. The value of this property must match the `name` attribute value of the gameselector element.
    - This property can only be used in the `system` view and only when `metadata` has a value.
* `gameselectorEntry` - type: UNSIGNED_INTEGER
    - This optional property which is only available in the `system` view makes it possible to select which `gameselector` entry to use to populate the `metadata` property. This assumes that a `gameCount` property for the gameselector element has been defined with a value higher than `1`. By defining multiple `text` elements with different values for the `gameselectorEntry` property it's possible to display multiple game entries at the same time, for example listing a couple of games that were last played, or a selection of random games. If the requested entry does not exist (for instance if `gameCount` has been set to 5 and `gameselectorEntry` has been set to `4` but the system only contains 3 games), then the overall element will not get rendered. Note that the first entry is defined as `0`, the second entry as `1` etc.
    - Minimum value is `0` and maximum value is the value of the `gameselector` element property `gameCount` minus 1. If a value outside this range is defined, then it will be automatically clamped to a valid value.
    - Default is `0`
* `container` - type: BOOLEAN
    - Whether the text should be placed inside a scrollable container.
    - Default is `true` if `metadata` is set to `description`, otherwise `false`
    - This property can only be used in the `gamelist` view.
* `containerVerticalSnap` - type: BOOLEAN
    - Whether the text should be vertically snapped to the font height. With this property enabled the container will have its height reduced as needed so that only complete rows of text are displayed at the start and end positions. This will not affect the "real" size of the container as set by the `size` property which means that the overall element placement will still be predictable if a vertical origin other than zero is used.
    - Default is `true`
* `containerScrollSpeed` - type: FLOAT
    - A base speed is automatically calculated based on the container and font sizes, so this property applies relative to the auto-calculated value.
    - Minimum value is `0.1` and maximum value is `10`
    - Default is `1`
* `containerStartDelay` - type: FLOAT
    - Delay in seconds before scrolling starts. Note that the text fade-in animation that plays when resetting from the end position will cause a slight delay even if this property is set to zero.
    - Minimum value is `0` and maximum value is `10`
    - Default is `4.5`
* `containerResetDelay` - type: FLOAT
    - Delay in seconds before resetting to the start position after reaching the scrolling end position.
    - Minimum value is `0` and maximum value is `20`
    - Default is `7`
* `fontPath` - type: PATH
    - Path to a TrueType font (.ttf).
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height for horizontally oriented screens or screen width for vertically oriented screens (e.g. for a value of `0.1`, the text's height would be 10% of the screen height). This calculation is based on the reference 'S' character so other glyphs may not fill this area, or they may exceed this area.
    - Minimum value is `0.001` and maximum value is `1.5`. Note that when running at a really low resolution, the minimum value may get clamped to a larger relative size. The font is allowed to overflow the height of the element by 100%, i.e. `fontSize` can be set to twice that of the y axis of the `size` property. Any value above that will be clamped.
    - Default is `0.045`
* `horizontalAlignment` - type: STRING
    - Controls alignment on the X axis.
    - Valid values are `left`, `center` or `right`
    - Default is `left`
* `verticalAlignment` - type: STRING
    - Controls alignment on the Y axis.
    - Valid values are `top`, `center` or `bottom`
    - Default is `center`
    - This property can only be used if `container` is `false`
* `color` - type: COLOR
    - Default is `000000FF`
* `backgroundColor` - type: COLOR
    - Default is `00000000`
* `letterCase` - type: STRING
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is `none` (original letter case is retained)
* `lineSpacing` - type: FLOAT
    - Controls the space between lines (as a multiple of the font height). Due to the imprecise nature of typefaces where certain glyphs (characters) may exceed the requested font size, it's recommended to keep this value at around `1.1` or higher for multi-line text fields. This way overlapping glyphs or characters being cut off at the top or bottom will be prevented.
    - Minimum value is `0.5` and maximum value is `3`
    - Default is `1.5`
* `opacity` - type: FLOAT
    - Controls the level of transparency. If set to `0` the element will be disabled.
    - Minimum value is `0` and maximum value is `1`
    - Default is `1`
* `visible` - type: BOOLEAN
    - If set to false, the element will be disabled. This is equivalent to setting `opacity` to `0`
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `40`

#### datetime

Displays a date and time as a text string. The format is ISO 8601 (YYYY-MM-DD) by default, but this can be changed using the `format` property. The text _unknown_ will be shown by default if there is no time stamp available. If the property `displayRelative` has been set, the text will be shown as _never_ in case of no time stamp.

Supported views:
* `system`
* `gamelist`

Instances per view:
* `unlimited`

Properties:
* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - Possible combinations:
    - `0 0` - automatically size so text fits on one line (expanding horizontally).
    - `w 0` - automatically wrap text so it doesn't go beyond `w` (expanding vertically).
    - `w h` - works like a "text box". If `h` is non-zero and `h` <= `fontSize` (implying it should be a single line of text), text that goes beyond `w` will be truncated with an elipses (...).
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the text should be rotated. Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the text will be rotated.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0.5 0.5`.
* `metadata` - type: STRING
    - This displays the metadata values that are available for the game. If an invalid metadata field is defined, the text "unknown" or "never" will be printed. To use this property from the `system` view, you will first need to add a `gameselector` element. You can only define a single metadata value per datetime element.
    - Valid values:
    - `releasedate` - The release date of the game.
    - `lastplayed` - The time the game was last played. This will be displayed as a value relative to the current date and time by default, but can be overridden using the `displayRelative` property.
* `defaultValue` - type: STRING
    - This property makes it possible to override the default "unknown" text that is displayed if `metadata` has been set to `releasedate` or the default "never" text that is displayed if `metadata` has been set to `lastplayed`. Any string can be used but you can't set it to a blank value. If you don't want to display anything when there is no metadata available, then set this property to `:space:` in which case a blankspace will be used. This property has no effect on the metadata editor where "unknown" will still be shown for undefined release date values.
* `gameselector` - type: STRING
    - If more than one gameselector element has been defined, this property makes it possible to state which one to use. If multiple gameselector elements have been defined and this property is missing then the first entry will be chosen and a warning message will be logged. If only a single gameselector has been defined, this property is ignored. The value of this property must match the `name` attribute value of the gameselector element. This property is only needed for the `system` view and only if the `metadata` property is utilized.
* `gameselectorEntry` - type: UNSIGNED_INTEGER
    - This optional property which is only available in the `system` view makes it possible to select which `gameselector` entry to use to populate the `metadata` property. This assumes that a `gameCount` property for the gameselector element has been defined with a value higher than `1`. By defining multiple `datetime` elements with different values for the `gameselectorEntry` property it's possible to display multiple game entries at the same time, for example listing a couple of games that were last played, or a selection of random games. If the requested entry does not exist (for instance if `gameCount` has been set to 5 and `gameselectorEntry` has been set to `4` but the system only contains 3 games), then the overall element will not get rendered. Note that the first entry is defined as `0`, the second entry as `1` etc.
    - Minimum value is `0` and maximum value is the value of the `gameselector` element property `gameCount` minus 1. If a value outside this range is defined, then it will be automatically clamped to a valid value.
    - Default is `0`
* `fontPath` - type: PATH
    - Path to a TrueType font (.ttf).
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height for horizontally oriented screens or screen width for vertically oriented screens (e.g. for a value of `0.1`, the text's height would be 10% of the screen height). This calculation is based on the reference 'S' character so other glyphs may not fill this area, or they may exceed this area.
    - Minimum value is `0.001` and maximum value is `1.5`. Note that when running at a really low resolution, the minimum value may get clamped to a larger relative size. The font is allowed to overflow the height of the element by 100%, i.e. `fontSize` can be set to twice that of the y axis of the `size` property. Any value above that will be clamped.
    - Default is `0.045`
* `horizontalAlignment` - type: STRING
    - Controls alignment on the X axis.
    - Valid values are `left`, `center` or `right`
    - Default is `left`
* `verticalAlignment` - type: STRING
    - Controls alignment on the Y axis.
    - Valid values are `top`, `center` or `bottom`
    - Default is `center`
* `color` - type: COLOR
* `backgroundColor` - type: COLOR
* `letterCase` - type: STRING
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is `none` (original letter case is retained)
* `lineSpacing` - type: FLOAT
    - Controls the space between lines (as a multiple of font height).
    - Minimum value is `0.5` and maximum value is `3`
    - Default is `1.5`
* `format` - type: STRING
    - Specifies the date and time format. Has no effect if `displayRelative` has been set to true.
    - %Y: The year, including the century (1900)
    - %m: The month number [01,12]
    - %d: The day of the month [01,31]
    - %H: The hour (24-hour clock) [00,23]
    - %M: The minute [00,59]
    - %S: The second [00,59]
    - Default is the ISO 8601 standard notation `%Y-%m-%d`
* `displayRelative` - type: BOOLEAN.
    - Renders the datetime as a relative string (e.g. 'x days ago').
    - Default is `false` if `metadata` has been set to `releasedate` and `true` if `metadata` has been set to `lastplayed`
* `opacity` - type: FLOAT
    - Controls the level of transparency. If set to `0` the element will be disabled.
    - Minimum value is `0` and maximum value is `1`
    - Default is `1`
* `visible` - type: BOOLEAN
    - If set to false, the element will be disabled. This is equivalent to setting `opacity` to `0`
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `40`

#### gamelistinfo

Displays the game count (all games as well as favorites), any applied filters, and a folder icon if a folder has been entered. If this text is left aligned or center aligned, the folder icon will be placed to the right of the other information, and if it's right aligned, the folder icon will be placed to the left.

Supported views:
* `gamelist`

Instances per view:
* `unlimited`

Properties:
* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - Possible combinations:
    - `0 0` - automatically size so text fits on one line (expanding horizontally).
    - `w 0` - automatically wrap text so it doesn't go beyond `w` (expanding vertically).
    - `w h` - works like a "text box". If `h` is non-zero and `h` <= `fontSize` (implying it should be a single line of text), text that goes beyond `w` will be truncated with an elipses (...).
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the text should be rotated. Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the element will be rotated.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0.5 0.5`
* `fontPath` - type: PATH
    - Path to a TrueType font (.ttf).
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height for horizontally oriented screens or screen width for vertically oriented screens (e.g. for a value of `0.1`, the text's height would be 10% of the screen height). This calculation is based on the reference 'S' character so other glyphs may not fill this area, or they may exceed this area.
    - Minimum value is `0.001` and maximum value is `1.5`. Note that when running at a really low resolution, the minimum value may get clamped to a larger relative size. The font is allowed to overflow the height of the element by 100%, i.e. `fontSize` can be set to twice that of the y axis of the `size` property. Any value above that will be clamped.
    - Default is `0.045`
* `horizontalAlignment` - type: STRING
    - Controls alignment on the X axis.
    - Valid values are `left`, `center` or `right`
    - Default is `left`
* `verticalAlignment` - type: STRING
    - Controls alignment on the Y axis.
    - Valid values are `top`, `center` or `bottom`
    - Default is `center`
* `color` - type: COLOR
* `backgroundColor` - type: COLOR
* `opacity` - type: FLOAT
    - Controls the level of transparency. If set to `0` the element will be disabled.
    - Minimum value is `0` and maximum value is `1`
    - Default is `1`
* `visible` - type: BOOLEAN
    - If set to false, the element will be disabled. This is equivalent to setting `opacity` to `0`
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `45`

#### rating

Displays a graphical representation of the game rating, from 0 to 5.

To display game ratings in the `system` view, you first need to create a `gameselector` element.

Supported views:
* `system`
* `gamelist`

Instances per view:
* `unlimited`

Properties:
* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - These values are mutually exclusive, if an X axis value is defined then the element will be sized based on this, and if an Y axis value is defined then the element will be sized based on that. If both the X and Y axis values are defined then the Y axis value will take precedence and the X axis value will be ignored. This makes sure that the image aspect ratio is always maintained.
  - Minimum value per axis is `0.01` and maximum value for the X axis is `1` and maximum value for the Y axis is `0.5`
  - Default is `0 0.06`
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the rating should be rotated. Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the rating will be rotated.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0.5 0.5`
* `gameselector` - type: STRING
    - If more than one gameselector element has been defined, this property makes it possible to state which one to use. If multiple gameselector elements have been defined and this property is missing then the first entry will be chosen and a warning message will be logged. If only a single gameselector has been defined, this property is ignored. The value of this property must match the `name` attribute value of the gameselector element. This property is only needed for the `system` view.
* `gameselectorEntry` - type: UNSIGNED_INTEGER
    - This optional property which is only available in the `system` view makes it possible to select which `gameselector` entry to use to populate the rating value. This assumes that a `gameCount` property for the gameselector element has been defined with a value higher than `1`. By defining multiple `rating` elements with different values for the `gameselectorEntry` property it's possible to display multiple game entries at the same time, for example listing a couple of games that were last played, or a selection of random games. If the requested entry does not exist (for instance if `gameCount` has been set to 5 and `gameselectorEntry` has been set to `4` but the system only contains 3 games), then the overall element will not get rendered. Note that the first entry is defined as `0`, the second entry as `1` etc.
    - Minimum value is `0` and maximum value is the value of the `gameselector` element property `gameCount` minus 1. If a value outside this range is defined, then it will be automatically clamped to a valid value.
    - Default is `0`
* `interpolation` - type: STRING
    - Interpolation method to use when scaling the images. Nearest neighbor (`nearest`) preserves sharp pixels and linear filtering (`linear`) makes the image smoother. The effect of this property is primarily visible for raster graphic images, but it has a limited effect also when using scalable vector graphics (SVG) images, and even more so if rotation is applied.
    - Valid values are `nearest` or `linear`
    - Default is `nearest`
* `color` - type: COLOR
    - Multiply each pixel's color by this color. For example, an all-white image with `<color>FF0000</color>` would become completely red. You can also control the transparency of an image with `<color>FFFFFFAA</color>` - keeping all the pixels their normal color and only affecting the alpha channel.
    - Default is `FFFFFFFF`
* `filledPath` - type: PATH
    - Path to the "filled" rating image. Any aspect ratio is supported. Note that there is no explicit padding property, so to add spaces between each icon simply make the image content smaller on the canvas. The images should always be centered on the canvas or otherwise the filledPath and unfilledPath textures will not align properly for all rating values. Most common file extensions are supported (including .svg, .jpg, .png, and unanimated .gif).
* `unfilledPath` - type: PATH
    - Path to the "unfilled" rating image. Any aspect ratio is supported. Note that there is no explicit padding property, so to add spaces between each icon simply make the image content smaller on the canvas. The images should always be centered on the canvas or otherwise the filledPath and unfilledPath textures will not align properly for all rating values. Most common file extensions are supported (including .svg, .jpg, .png, and unanimated .gif).
* `overlay` - type: BOOLEAN
    - Whether to overlay the filledPath image on top of the unfilledPath image. If this property is set to false, then the unfilledPath image will only be rendered to the right of the rating value cut position. This property is useful for avoiding image aliasing artifacts that could otherwise occur when combining some rating images. It can also help with avoiding some inconsistent fade-out animations.
    - Default is `true`
* `opacity` - type: FLOAT
    - Controls the level of transparency. If set to `0` the element will be disabled.
    - Minimum value is `0` and maximum value is `1`
    - Default is `1`
* `visible` - type: BOOLEAN
    - If set to false, the element will be disabled. This is equivalent to setting `opacity` to `0`
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `45`

### Special elements

Elements from this group provide special functionality not covered by the primary and secondary elements.

#### gameselector

Selects games from the gamelists when navigating the `system` view. This makes it possible to display game media and game metadata directly from this view. It's possible to make separate gameselector configurations per game system, so that for instance a random game could be displayed for one system and the most recently played game could be displayed for another system. It's also possible to define multiple gameselector elements with different selection criterias per game system which makes it possible to for example set a random fan art background image and at the same time display a box cover image of the most recently played game. The gameselector logic can be used for the `image`, `video`, `text`, `datetime` and `rating` elements.

Note that any games that have the metadata option _Exclude from game counter_ set will be excluded by the gameselector. Also note that setting `gameCount` to a high value may introduce significant lag into the application for large systems, so try to keep this as low as possible and make thorough performance testing with huge game libraries.

Supported views:
* `system`

Instances per view:
* `unlimited`

Properties:
* `selection` - type: STRING
    - This defines the game selection criteria. If set to `random`, the games are refreshed every time the view is navigated. For the other two values the refresh takes place on gamelist reload, i.e. when launching a game, adding a game as favorite, making changes via the metadata editor and so on.
    - Valid values are `random`, `lastplayed` or `mostplayed`
    - Default is `random`
* `gameCount` - type: UNSIGNED_INTEGER
    - Minimum value is `1` and maximum value is `30`
    - Default is `1`
* `allowDuplicates` - type: BOOLEAN
    - If set to true then the same game may appear multiple times, i.e. the amount of entries defined by `gameCount` are always fully populated. This only applies to entries higher than the amount of available games for a system, for example if a system contains 3 games and `gameCount` has been set to `5`, then the first three entries will not contain any duplicate entries, but the last two will.
    - Default is `false`
    - This property can only be used when `selection` is `random`

#### helpsystem

The helpsystem is a special element that displays a context-sensitive list of actions the user can take at any time. You should try and keep the position constant throughout every screen. Note that this element does not have a zIndex value, instead it's always rendered on top of all other elements.

It's possible to set this element as right-aligned or center-aligned using a combination of the `pos` and `origin` properties. For example `<pos>1 1</pos>` and `<origin>1 1</origin>` will place it in the lower right corner of the screen.

Keep in mind that the width of this element can vary depending on a number of factors, for example the _Toggle favorites_ and _Random system or game_ buttons can be enabled or disabled via the _UI Settings_ menu. Test extensively with the menu system as well, especially the virtual keyboard which displays a number of helpsystem entries.

Supported views:
* `system`
* `gamelist`

Instances per view:
* `single`

Properties:
* `pos` - type: NORMALIZED_PAIR
    - Default is `0.012 0.9515` for horizontally oriented screens and `0.012 0.975` for vertically oriented screens
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `textColor` - type: COLOR
    - Default is `777777FF`
* `textColorDimmed` - type: COLOR
    - Text color to use when the background is dimmed (when a menu is open).
    - Default is the same value as textColor.
* `iconColor` - type: COLOR
    - Default is `777777FF`
* `iconColorDimmed` - type: COLOR
    - Icon color to use when the background is dimmed (when a menu is open).
    - Default is the same value as iconColor.
* `fontPath` - type: PATH
* `fontSize` - type: FLOAT
    - This property implicitly sets the icon size and is therefore the means to change the overall size of the helpsystem element. This calculation is based on the reference 'S' character so other glyphs may not fill this area, or they may exceed this area.
    - Minimum value is `0.001` and maximum value is `1.5`. Note that when running at a really low resolution, the minimum value may get clamped to a larger relative size.
    - Default is `0.035` for horizontally oriented screens and `0.025` for vertically oriented screens
* `entrySpacing` - type: FLOAT
    - Spacing between the help element pairs.
    - Minimum value is `0` and maximum value is `0.04`
    - Default is `0.00833`
* `iconTextSpacing` - type: FLOAT
    - Spacing between the icon and text within a help element pair.
    - Minimum value is `0` and maximum value is `0.04`
    - Default is `0.00416`
* `letterCase` - type: STRING
    - Valid values are `uppercase`, `lowercase` or `capitalize`
    - Default is `uppercase`
* `opacity` - type: FLOAT
    - Controls the level of transparency.
    - Minimum value is `0.2` and maximum value is `1`
    - Default is `1`
* `customButtonIcon` - type: PATH
    - A button icon override. Specify the button type in the attribute `button`.
    - The available buttons are: \
      `dpad_updown`,
      `dpad_leftright`,
      `dpad_all`,
      `thumbstick_click`,
      `button_l`,
      `button_r`,
      `button_lr`,
      `button_lt`,
      `button_rt`,
      `button_ltrt`,
      `button_a_XBOX`,
      `button_b_XBOX`,
      `button_x_XBOX`,
      `button_y_XBOX`,
      `button_back_XBOX`,
      `button_start_XBOX`,
      `button_back_XBOX360`,
      `button_start_XBOX360`,
      `button_a_PS`,
      `button_b_PS`,
      `button_x_PS`,
      `button_y_PS`,
      `button_back_PS123`,
      `button_start_PS123`,
      `button_back_PS4`,
      `button_start_PS4`,
      `button_back_PS5`,
      `button_start_PS5`,
      `button_a_switch`,
      `button_b_switch`,
      `button_x_switch`,
      `button_y_switch`,
      `button_back_switch`,
      `button_start_switch`,
      `button_a_SNES`,
      `button_b_SNES`,
      `button_x_SNES`,
      `button_y_SNES`,
      `button_back_SNES`,
      `button_start_SNES`
