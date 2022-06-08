# EmulationStation Desktop Edition (ES-DE) v2.0 (development version) - Themes

**Note:** This document is only relevant for the current ES-DE development version, if you would like to see the documentation for the latest stable release, refer to [THEMES.md](THEMES.md) instead.

If creating theme sets specifically for ES-DE, please add `-DE` to the theme name, as in `slate-DE`. Because ES-DE theme functionality has deviated greatly from the RetroPie EmulationStation fork on which it was originally based, any newer themes will not work on such older forks. It would be confusing and annoying for users that attempt to use ES-DE theme sets in older EmulationStation forks as they would get unthemed systems, crashes, error messages or corrupted graphics. At least the -DE extension is a visual indicator that it's an ES-DE specific theme set.

Table of contents:

[[_TOC_]]

## Introduction

ES-DE allows the grouping of themes for multiple game systems into a **theme set**. A theme is a collection of **elements**, each with their own **properties** that define the way they look and behave. These elements include things like text lists, carousels, images and animations.

Internally ES-DE uses the concept of **components** to actually implement the necessary building blocks to parse and render the elements, and although this is normally beyond the scope of what a theme author needs to consider, it's still good to be aware of the term as it's sometimes used in the documentation.

Every game system has its own subdirectory within the theme set directory structure, and these are defined in the systems configuration file `es_systems.xml` either via the optional `<theme>` tag, or otherwise via the mandatory `<name>` tag. When ES-DE populates a system on startup it will look for a file named `theme.xml` in each such directory.

By placing a theme.xml file directly in the root of the theme set directory, that file will be processed as a default if there is no system-specific theme.xml file available.

In the example below, we have a theme set named `mythemeset-DE` which includes the `snes` and `nes` systems. Assuming you have some games installed for these systems, the files `mythemeset-DE/nes/theme.xml` and `mythemeset-DE/snes/theme.xml` will be processed on startup. If there are no games available for a system, its theme.xml file will be skipped.

The directory structure of our example theme set could look something like the following:

```
...
   themes/
      mythemeset-DE/
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
`/usr/share/emulationstation/themes/slate-DE/`

If a theme set with the same name exists in both locations, the one in the home directory will be loaded and the other one will be skipped.

## Differences to legacy RetroPie themes

If you are not familiar with theming for RetroPie or similar forks of EmulationStation you can skip this section as it only describes the key differences between the updated ES-DE themes and these _legacy_ theme sets. The term _legacy_ is used throughout this document to refer to this older style of themes which ES-DE still fully supports for backward compatibility reasons. The old theme format is described in [THEMES-LEGACY.md](THEMES-LEGACY.md) although this document is basically a historical artifact by now.

With ES-DE v2.0 a new theme engine was introduced that fundamentally changed some aspects of how theming works. The previous design used specific view styles (basic, detailed, video and grid) and this was dropped completely and replaced with _variants_ that can accomplish the same thing while being much more powerful and flexible.

In the past EmulationStation basically had hardcoded view styles with certain elements always being present and only a limited ability to manipulate these via positioning, resizing, coloring etc. As well so-called _extras_ were provided to expand theming support somehow but even this was quite limited.

With the new theme engine the view presets were removed and the only views now available, _system_ and _gamelist_, were rewritten to be much more flexible. Essentially the element selection and placement is now unlimited; any number of elements of any type can be used, although with a few notable exceptions as explained throughout this document.

In addition to the variant support which provides an unlimited flexibility for creating custom theme profiles, support for specific aspect ratios was introduced. This makes it possible to define different theme configuration for different display aspect ratios and to provide the user with the option to choose between these from the _UI Settings_ menu. That could for example be choice between a 16:9 and a 4:3 layout, or perhaps also a vertical screen orientation layout in addition to these.

As well new theming abilities like Lottie animations were added with the new theme engine.

The following are the most important changes compared to the legacy theme structure:

* View styles are now limited to only _system_ and _gamelist_ (there is a special _all_ view style as well but that is only used for navigation sounds as explained later in this document)
* The hardcoded metadata attributes like _md_image_ and _md_developer_ are gone, but a new `<metadata>` property is available for populating views with metadata information
* The concept of _extras_ is gone as all element can now be used however the theme author wishes
* The concept of _features_ is gone
* The `<formatVersion>` tag is gone as tracking theme versions doesn't make much sense after all
* The `video` element properties `showSnapshotNoVideo` and `showSnapshotDelay` have been removed
* The ambiguous `alignment` property has been replaced with the `horizontalAlignment` and `verticalAlignment` properties (the same is true for `logoAlignment` for the `carousel` element)
* The `forceUppercase` property has been replaced with the more versatile `letterCase` property
* Many property names for the carousel have been renamed, with _logo_ being replaced by _item_ as this element can now be used in both the gamelist and system views. As well, setting the alignment will not automatically add any margins as is the case for legacy themes. These can still be set manually using the `horizontalOffset` and `verticalOffset` properties if needed. The way that alignment works in general for both carousel items and the overall carousel has also changed
* The carousel text element hacks `systemInfo` and `logoText` have been removed and replaced with proper carousel properties
* The carousel property maxLogoCount is now in float format for more granular control of logo placement compared to integer format for legacy themes. However some legacy theme authors thought this property supported floats (as the theme documentation incorrectly stated this) and have therefore set it to fractional values such as 3.5. This was actually rounded up when loading the theme configuration, and this logic is retained for legacy themes for backward compatibility. But for current themes the float value is correctly interpreted which means a manual rounding of the value is required in order to retain an identical layout when porting theme sets to the new theme engine
* The helpsystem `textColorDimmed` and `iconColorDimmed` properties (which apply when opening a menu) were always defined under the system view configuration which meant these properties could not be separately set for the gamelist views. Now these properties work as expected with the possibility to configure separate values for the system and gamelist views
* Correct theme structure is enforced more strictly than before, and deviations will generate error log messages and make the theme loading fail
* Many additional elements and properties have been added, refer to the [Reference](THEMES-DEV.md#reference) section for more information

Attempting to use any of the legacy logic in the new theme structure will make the theme loading fail, for example adding the _extra="true"_ attribute to any element.

Except the points mentioned above, theme configuration looks pretty similar to the legacy theme structure, so anyone having experience with these older themes should hopefully feel quite at home with the new theme engine. Probably the most important thing to keep in mind is that as there are no longer any view presets available, some more effort is needed from the theme developer to define values for some elements. This is especially true for zIndex values as elements could now be hidden by other elements if care is not taken to explicitly set the zIndex for each of them. This additional work is however a small price to pay for the much more powerful and flexible theming functionality provided by the new theme engine.

## Simple example

Here is a very simple theme that changes the color of the game name text:

```xml
<theme>
    <view name="gamelist">
        <text name="game_name">
            <color>00FF00</color>
        </text>
        <image name="frame_1">
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
<view name="ViewNameHere">
    ... define elements here ...
</view>
```

An element is a particular visual component such as an image, an animation or a piece of text. It has a mandatory _name_ attribute which is used by ES-DE to track each element entry. By using this name attribute it's possible to split up the definition of an element to different locations. For example you may want to define the color properties separately from where the size and position are configured (see the example below). The name attribute can be set to any string value.

This is the element structure:

```xml
<ElementTypeHere name="ElementNameHere">
    ... define properties here ...
</ElementTypeHere>
```

Finally _properties_ control how a particular element looks and behaves, for example its position, size, image path, animation controls etc. The property type determines what kinds of values you can use. You can read about each type below in the
[Reference](THEMES-DEV.md#reference) section. Properties are defined like this:

```xml
<propertyNameHere>ValueHere</propertyNameHere>
```
Let's now put it all together. The following is a simple example of a text element which has its definition split across two separate XML files.

`themes.xml`:
```xml
<theme>
    <view name="gamelist">
        <text name="system_name">
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
        <text name="system_name">
            <color>707070</color>
        </text>
    </view>
</theme>
```

As long as the name attribute is identical, the element configuration will be combined automatically. But that is only true for elements of the same type, so for instance an image element could be defined that also uses _system_name_ for its name attribute without colliding with the text element:
```xml
<theme>
    <view name="gamelist">
        <text name="system_name">
            <pos>0.27 0.32</pos>
            <origin>0.5 0.5</origin>
            <size>0.12 0.41</size>
            <zIndex>40</zIndex>
        </text>
        <!-- Does not cause a collision, but is probably a bad idea for readability reasons -->
        <image name="system_name">
            <pos>0.49 0.8</pos>
            <maxSize>0.4 0.28</maxSize>
            <zIndex>35</zIndex>
        </text>
    </view>
</theme>
```

Whether this is a good idea is another question, it would probably be better to set the name attribute for the image to _system_logo_ or similar for this example.

In addition to this, if the name is used for the same element type but for different views, then there will also not be any collision:

```xml
<theme>
    <view name="system">
        <text name="system_name">
        <pos>0.04 0.73</pos>
        <origin>0.5 0.5</origin>
        <size>0.12 0.22</size>
        <zIndex>40</zIndex>
    </view>
    <!-- This will not cause a collision as these two text elements are defined for different views -->
    <view name="gamelist">
        <text name="system_name">
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
Jan 28 17:17:30 Error:  ThemeData::parseElement(): "/home/myusername/.emulationstation/themes/mythemeset-DE/theme.xml": Property "origin" for element "image" has no value defined
```

Sanitization for valid data format and structure is done in this manner, but verification that property values are actually correct (or reasonable) is handled by the individual component that takes care of creating and rendering the specific theme element. What happens in many instances is that a log warning entry is created and the invalid property is reset to its default value. So for these situations, the system will not become unthemed. Here's an example where a badges element accidentally had its horizontalAlignment property set to _leftr_ instead of _left_:
```
Jan 28 17:25:27 Warn:   BadgeComponent: Invalid theme configuration, <horizontalAlignment> set to "leftr"
```

Note however that warnings are not printed for all invalid properties as that would lead to an excessive amount of logging code. This is especially true for numeric values which are commonly just clamped to the allowable range without notifying the theme author. So make sure to check the [Reference](THEMES-DEV.md#reference) section of this document for valid values for each property.

### Variants

A core concept of ES-DE is the use of theme set _variants_ to provide different theme profiles. These are not fixed presets and a theme author can instead name and define whatever variants he wants for his theme (or possibly use no variants at all as they are optional).

The variants could be purely cosmetic, such as providing light and dark mode versions of the theme set, or they could provide different functionality by for instance using different primary elements such as a carousel or a text list.

Before a variant can be used it needs to be declared, which is done in the `capabilities.xml` file that must be stored in the root of the theme set directory tree. How to setup this file is described in detailed later in this document.

The use of variants is straightforward, a section of the configuration that should be included for a certain variant is enclosed inside the `<variant>` tag pair. This has to be placed inside the `<theme>` tag pair, and it can only be used on this level of the hierarchy and not inside a `<view>` tag pair for example.

The mandatory _name_ attribute is used to specificy which variants to use, and multiple variants can be specified at the same time by separating them by commas or by whitespace characters (tabs, spaces or line breaks).

It could be a good idea to separate the various variant configuration into separate files that are then included from the main theme file as this could improve the structure and readability of the theme set configuration.

Here are some example uses of the `<variant>` functionality:

```xml
<!-- Implementing the variants by separate include files could be a good idea -->
<theme>
    <variant name="lightMode, lightModeNoVideo">
        <include>./../colors_light.xml</include>
    </variant>
    <variant name="darkMode, darkModeNoVideo">
        <include>./../colors_dark.xml</include>
    </variant>

    <view name="gamelist">
        <text name="info_text_01">
            <fontPath>./core/font.ttf</fontPath>
            <fontSize>0.035</fontSize>
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>

<!-- In other instances it may make more sense to apply the variant configuration inline -->
<theme>
    <variant name="lightModeNoVideo, darkModeNoVideo">
        <view name="gamelist">
            <image name="game_image">
                <imageType>titlescreen</imageType>
                <scrollFadeIn>true</scrollFadeIn>
                <zIndex>42</zIndex>
            </image>
        </view>
    </variant>

    <variant name="lightMode, darkMode">
        <view name="gamelist">
            <video name="game_video">
                <imageType>cover</imageType>
                <delay>1.7</delay>
                <scrollFadeIn>true</scrollFadeIn>
                <zIndex>42</zIndex>
            </video>
        </view>
    </variant>
</theme>

<!-- The following is NOT supported as <variant> tags can't be located inside <view> tag pairs -->
<theme>
    <view name="gamelist">
        <variant name="lightModeNoVideo">
            <image name="game_image">
                <imageType>titlescreen</imageType>
                <scrollFadeIn>true</scrollFadeIn>
                <zIndex>42</zIndex>
            </image>
        </variant>
    </view>
</theme>
```

### Aspect ratios

The aspect ratio support works almost identically to the variants support with the main difference that the available aspect ratios are hardcoded into ES-DE. The theme set can still decide which of the aspect ratios to support (or none at all in which cast the theme aspect ratios is left undefined) but it can't create entirely new aspect ratios.

The reason for why aspect ratios were implemented at all instead of only using variants was that the amount of defined variants would have grown exponentially for all possible combinations, making the theme sets very hard to create and maintain.

In the same manner as for the variants, the aspect ratios that the theme set provides need to be declared in the `capabilities.xml` file that must be stored in the root of the theme set directory tree. How to setup this file is described in detailed later in this document.

Once the aspect ratios have been defined, they are applied to the theme configuration in exactly the same way as the variants:

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
        <text name="info_text_01">
            <fontPath>./core/font.ttf</fontPath>
            <fontSize>0.035</fontSize>
        </text>
    </view>
</theme>

<!-- In other instances it may make more sense to apply the aspect ratio configuration inline -->
<theme>
    <aspectRatio name="4:3, 5:4">
        <view name="gamelist">
            <image name="image_logo">
                <pos>0.3 0.56</pos>
            </image>
        </view>
    </aspectRatio>

    <aspectRatio name="16:9, 16:10, 21:9">
        <view name="gamelist">
            <image name="image_logo">
                <pos>0.42 0.31</pos>
            </image>
        </view>
    </aspectRatio>
</theme>

<!-- The following is NOT supported as <aspectRatio> tags can't be located inside <view> tag pairs -->
<theme>
    <view name="gamelist">
        <aspectRatio name="4:3, 5:4">
            <image name="image_logo">
                <pos>0.3 0.56</pos>
            </image>
        </aspectRatio>
    </view>
</theme>
```

### capabilities.xml

Variant and aspect ratio values need to be declared before they can be used inside the actual theme set configuration files and that is done in the `capabilities.xml` file. This file needs to exist in the root of the theme directory, for example:
```
~/.emulationstation/themes/mythemeset-DE/capabilities.xml
```

This file type was introduced with the new ES-DE theme engine in v2.0 and is an indicator that the theme set is of the new generation instead of being of the legacy type (i.e. a theme set backward compatible with RetroPie EmulationStation). In other words, if the capabilities.xml file is absent, the theme will get loaded as a legacy set.

The structure of the file is simple, it just contains the declarations for the variants and aspect ratios, such as in this example:

```xml
<!-- Theme capabilities for mythemeset-DE -->
<themeCapabilities>
    <aspectRatio>16:9</aspectRatio>
    <aspectRatio>4:3</aspectRatio>
    <aspectRatio>4:3_vertical</aspectRatio>

    <variant name="lightMode">
        <label>Light mode</label>
        <selectable>true</selectable>
    </variant>

    <variant name="darkMode">
        <label>Dark mode</label>
        <selectable>true</selectable>
    </variant>
</themeCapabilities>
```
The file format is hopefully mostly self-explanatory; this example provides three aspect ratios and two variants. The `<label>` tag for the variant is the text that will show up in the _UI Settings_ menu where the variants can be selected, assuming `<selectable>` has been set to true.

Both the variant name and its label can be set to an arbitrary value, but the name has to be unique. If two variant entries are declared with the same name, a warning will be generated on startup and the duplicate entry will not get loaded. Variants will be listed in the _UI Settings_ menu in the order that they are defined in capabilities.xml.

Unlike the variants, the aspectRatio entries can not be set to arbitrary values, instead they have to use a value from the _horizontal name_ or _vertical name_ columns in the following table:

| Horizontal name  | Vertical name  | Common resolutions                             |
| :--------------- | :------------- | :--------------------------------------------- |
| 16:9             | 16:9_vertical  | 1280x720, 1920x1080, 2560x1440, 3840x2160      |
| 16:10            | 16:10_vertical | 1280x800, 1440x900, 1920x1200                  |
| 3:2              | 3:2_vertical   | 2160x1440                                      |
| 4:3              | 4:3_vertical   | 320x240, 640x480, 800x600, 1024x768, 1600x1200 |
| 5:4              | 5:4_vertical   | 1280x1024                                      |
| 21:9             | 21:9_vertical  | 2560x1080, 2560x1440, 3840x1600, 5120x1440     |
| 32:9             | 32:9_vertical  | 3840x1080, 5120x1440                           |

The 21:9 and 32:9 aspect ratios are approximate as monitors of slightly different ratios are collectively marketed using these numbers.

It's normally not necessary to define all or even most of these for a theme set, instead only a few are likely to be needed. The element placement will always adapt to the screen resolution as relative positions are utilized, so in most cases similar aspect ratios like 4:3 and 5:4 could be used interchangeably. The same is true for instance for 16:9 and 16:10. But if precise element placement is required, a separate configuration can still be made for each aspect ratio.

The declared aspect ratios will always get displayed in the _UI settings_ menu in the order listed in the table above, so they can be declared in any order in the capabilities.xml file. If an unsupported aspect ratio value is entered, a warning will be generated on startup and the entry will not get loaded.

The use of variants and aspect ratios is optional, i.e. a theme set does not need to provide either of them. There must however be a capabilities.xml file present in the root of the theme set directory. So if you don't wish to provide this functionality, simply create an empty file or perhaps add a short XML comment to it to clarify that the theme set does not provide this functionality. In this case the theme will still load and work correctly but the menu options for selecting variants and aspect ratios will be grayed out.

### The \<include\> tag

You can include theme files within theme files, for example:

`~/.emulationstation/themes/mythemeset-DE/fonts.xml`:
```xml
<theme>
    <view name="gamelist">
        <text name="info_text_01">
            <fontPath>./core/font.ttf</fontPath>
            <fontSize>0.035</fontSize>
            <color>00FF00</color>
        </text>
    </view>
</theme>
```

`~/.emulationstation/themes/mythemeset-DE/snes/theme.xml`:
```xml
<theme>
    <include>./../fonts.xml</include>
    <view name="gamelist">
        <text name="info_text_01">
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
        <text name="info_text_01">
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

The paths defined for the `<include>` entry and `<fontPath>` property are set as relative to the theme file by adding "./" as a prefix. That is usually how paths would be defined as you commonly want to access files only within the theme set directory structure. This prefix works for all path properties. Windows-style backslashes are also supported as directory separators but their use is not recommended.

You can add `<include>` tags directly inside the `<theme>` tags or inside either the `<variant>` or `<aspectRatio>` tags, but not inside `<view>` tags:

```xml
<!-- Adding <include> directly inside <theme> is supported -->
<theme>
    <include>./../colors.xml</include>
    <view name="gamelist">
        <text name="info_text_01">
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>

<!-- Adding <include> inside <variant> is supported -->
<theme>
    <variant name="lightMode">
        <include>./../colors.xml</include>
    </variant>
    <view name="gamelist">
        <text name="info_text_01">
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>

<!-- Adding <include> inside <aspectRatio> is supported -->
<theme>
    <aspectRatio name="4:3">
        <include>./../colors.xml</include>
    </aspectRatio>
    <view name="gamelist">
        <text name="info_text_01">
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>

<!-- Adding <include> inside <view> is NOT supported -->
<theme>
    <view name="gamelist">
        <include>./../colors.xml</include>
        <text name="info_text_01">
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>

<!-- Adding <include> outside <theme> is NOT supported -->
<include>./../colors.xml</include>
<theme>
    <view name="gamelist">
        <text name="info_text_01">
            <pos>0.3 0.56</pos>
        </text>
    </view>
</theme>
```

### Theming the system and gamelist views simultaneously

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

### Theming multiple elements simultaneously

You can theme multiple elements of the same type simultaneously, which can lead to a more compact and easier to understand theme configuration. To accomplish this you simply define multiple entries inside a single `name` attribute, separated by commas or whitespace characters (tabs, spaces or line breaks).

Here's an example of defining a common color to multiple text elements:

```xml
<theme>
    <view name="gamelist">
        <!-- Weird spaces/newline on purpose -->
        <text name="md_lbl_rating, md_lbl_releasedate, md_lbl_developer, md_lbl_publisher,
        md_lbl_genre,    md_lbl_players,        md_lbl_lastplayed, md_lbl_playcount">
            <color>48474D</color>
        </text>
    </view>
</theme>
```

The above is equivalent to:

```xml
<theme>
    <view name="gamelist">
        <text name="md_lbl_rating">
            <color>48474D</color>
        </text>
        <text name="md_lbl_releasedate">
            <color>48474D</color>
        </text>
        <text name="md_lbl_developer">
            <color>48474D</color>
        </text>
        <text name="md_lbl_publisher">
            <color>48474D</color>
        </text>
        <text name="md_lbl_genre">
            <color>48474D</color>
        </text>
        <text name="md_lbl_players">
            <color>48474D</color>
        </text>
        <text name="md_lbl_lastplayed">
            <color>48474D</color>
        </text>
        <text name="md_lbl_playcount">
            <color>48474D</color>
        </text>
    </view>
</theme>
```

Just remember, _this only works if the elements have the same type._


### Navigation sounds

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

### Element rendering order using zIndex

You can change the order in which elements are rendered by setting their `zIndex` values. All elements have a default value so you only need to define it for the ones you wish to explicitly change. Elements will be rendered in order from smallest to largest values. A complete description of each element including all supported properties can be found in the [Reference](THEMES-DEV.md#reference) section.

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
| textlist         | 50            |

The `helpsystem` element does not really have a zIndex value and is always rendered on top of all other elements.

### Theme variables

Theme variables can be used to simplify theme construction. There are 2 types of variables available.
* System variables
* Theme defined variables

#### System variables

System variables are system specific and are derived from the values in es_systems.xml (except for collections).
* `system.name`
* `system.name.collections`
* `system.name.noCollections`
* `system.fullName`
* `system.fullName.collections`
* `system.fullName.noCollections`
* `system.theme`
* `system.theme.collections`
* `system.theme.noCollections`

#### Theme defined variables
Variables can also be defined in the theme.
```
<variables>
    <themeColor>8B0000</themeColor>
</variables>
```

#### Usage in themes
Variables can be used to specify the value of a theme property:
```
<color>${themeColor}</color>
```

It can also be used to specify only a portion of the value of a theme property:

```
<color>${themeColor}C0</color>
<path>./core/images/${system.theme}.svg</path>
````

Nesting of variables is supported, so the following could be done:
```
<variables>
    <colorRed>8b0000</colorRed>
    <themeColor>${colorRed}</themeColor>
</variables>
```

## Reference

This section describes each element and their properties in detail and also contains example configuration snippets.

### Property data types

* NORMALIZED_PAIR - two decimals, in the range [0..1], delimited by a space. For example, `0.25 0.5`. Most commonly used for position (x and y coordinates) and size (width and height).
* NORMALIZED_RECT - four decimals, in the range [0..1], delimited by a space. For example, `0.25 0.5 0.10 0.30`. Most commonly used for padding to store top, left, bottom and right coordinates.
* PATH - a path. If the first character is a `~`, it will be expanded into the environment variable for the home path (`$HOME` for Unix and macOS or `%HOMEPATH%` for Windows) unless overridden using the --home command line option.  f the first character is a `.`, it will be expanded to the theme file's directory, allowing you to specify resources relative to the theme file, like so: `./../core/fonts/myfont.ttf`.
* BOOLEAN - `true`/`1` or `false`/`0`.
* COLOR - a hexadecimal RGB or RGBA color (6 or 8 digits). If 6 digits, will assume the alpha channel is `FF` (completely opaque).
* UNSIGNED_INTEGER - an unsigned integer.
* FLOAT - a decimal.
* STRING - a string of text.

### Element types and their properties

Common to almost all elements is a `pos` and `size` property of the NORMALIZED_PAIR type. They are normalized in terms of their "parent" object's size; 99% of the time this is just the size of the screen. In this case, `<pos>0 0</pos>` would correspond to the top left corner, and `<pos>1 1</pos>` the bottom right corner (a positive Y value points further down). You can also use numbers outside of the [0..1] range if you want to place an element partially or completely off-screen.

The order in which you define properties does not matter and you only need to define the ones where you want to override the default value.

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
    - If only one axis is specified (and the other is zero), then the other will be automatically calculated in accordance with the image's aspect ratio. Setting both axes to 0 is an error and the size will be clamped to `0.001 0.001` in this case.
    - Minimum value per axis is `0.001` and maximum value per axis is `2`. If specifying a value outside the allowed range then no attempt will be made to preserve the aspect ratio.
* `maxSize` - type: NORMALIZED_PAIR
    - The image will be resized as large as possible so that it fits within this size while maintaining its aspect ratio. Use this instead of `size` when you don't know what kind of image you're using so it doesn't get grossly oversized on one axis (e.g. with a game's image metadata).
    - Minimum value per axis is `0.001` and maximum value per axis is `2`
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0.5 0.5`
* `rotation` - type: FLOAT
    - Angle in degrees that the image should be rotated. Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the image will be rotated.
    - Default is `0.5 0.5`
* `path` - type: PATH
    - Explicit path to an image file. Most common extensions are supported (including .jpg, .png, and unanimated .gif). If `imageType` is also defined then this will take precedence as these two properties are not intended to be used together. If you need a fallback image in case of missing game media, use the `default` property instead.
* `default` - type: PATH
    - Path to a default image file. The default image will be displayed when the selected game does not have an image of the type defined by the `imageType` property (i.e. this `default` property does nothing unless a valid `imageType` property has been set). It's also applied to any custom collection that does not contain any games when browsing the grouped custom collections system.
* `imageType` - type: STRING
    - This displays a game image of a certain media type. Multiple types can be defined, in which case the entries should be delimited by commas or by whitespace characters (tabs, spaces or line breaks). The media will be searched for in the order that the entries have been defined. If no image is found, the space will be left blank unless the `default` property has been set. To use this property from the `system` view, you will first need to add a `gameselector` element.
    - Valid values:
    - `image` - This will look for a `miximage`, and if that is not found `screenshot` is tried next, then `titlescreen` and finally `cover`
    - `miximage` - This will look for a miximage.
    - `marquee` - This will look for a marquee (wheel) image.
    - `screenshot` - This will look for a screenshot image.
    - `titlescreen` - This will look for a title screen image.
    - `cover` - This will look for a box front cover image.
    - `backcover` - This will look for a box back cover image.
    - `3dbox` - This will look for a 3D box image.
    - `physicalmedia` - This will look for a physical media image.
    - `fanart` - This will look for a fan art image.
* `gameselector` - type: STRING
    - If more than one gameselector element has been defined, this property makes it possible to state which one to use. If multiple gameselector elements have been defined and this property is missing then the first entry will be chosen and a warning message will be logged. If only a single gameselector has been defined, this property is ignored. The value of this property must match the `name` attribute value of the gameselector element. This property is only needed for the `system` view and only if the `imageType` property is utilized.
* `tile` - type: BOOLEAN
    - If true, the image will be tiled instead of stretched to fit its size. Useful for backgrounds.
* `interpolation` - type: STRING
    - Interpolation method to use when scaling raster images. Nearest neighbor (`nearest`) preserves sharp pixels and linear filtering (`linear`) makes the image smoother. This property has no effect on scalable vector graphics (SVG) images.
    - Valid values are `nearest` or `linear`
    - Default is `nearest`
* `color` - type: COLOR
    - Multiply each pixel's color by this color. For example, an all-white image with `<color>FF0000</color>` would become completely red. You can also control the transparency of an image with `<color>FFFFFFAA</color>` - keeping all the pixels their normal color and only affecting the alpha channel.
* `colorEnd` - type: COLOR
    - Works exactly in the same way as `color` but can be set as the end color to apply a color shift gradient to the image.
* `gradientType` - type: STRING
    - The direction to apply the color shift gradient if both `color` and `colorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `scrollFadeIn` - type: BOOLEAN
    - If enabled, a short fade-in animation will be applied when scrolling through games in the gamelist view. This usually looks best if used for the main game image.
    - Default is `false`
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
    - If only one axis is specified (and the other is zero), then the other will be automatically calculated in accordance with the static image's aspect ratio and the video's aspect ratio. Setting both axes to 0 is an error and the size will be clamped to `0.01 0.01` in this case.
    - Minimum value per axis is `0.01` and maximum value per axis is `2`. If specifying a value outside the allowed range then no attempt will be made to preserve the aspect ratio.
* `maxSize` - type: NORMALIZED_PAIR
    - The static image and video will be resized as large as possible so that they fit within this size while maintaining their aspect ratios. Use this instead of `size` when you don't know what kind of video you're using so it doesn't get grossly oversized on one axis (e.g. with a game's video metadata).
    - Minimum value per axis is `0.01` and maximum value per axis is `2`
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0.5 0.5`
* `path` - type: PATH
    - Path to a video file. Setting a value for this property will make the video static, i.e. it will only play this video regardless of whether there is a game video available or not (this also applies to the `system` view if you have a `gameselector` element defined). If the `default` property has also been set, it will be overridden as the `path` property takes precedence.
* `default` - type: PATH
    - Path to a default video file. The default video will be played when the selected game does not have a video. This property is also applied to any custom collection that does not contain any games when browsing the grouped custom collections system.
* `defaultImage` - type: PATH
    - Path to a default image file. The default image will be displayed when the selected game does not have an image of the type defined by the `imageType` property (i.e. this `default` property does nothing unless a `imageType` property has been set). It's also applied to any custom collection that does not contain any games when browsing the grouped custom collections system.
* `imageType` - type: STRING
    - This displays a game image of a certain media type. Multiple types can be defined, in which case the entries should be delimited by commas or by whitespace characters (tabs, spaces or line breaks). The media will be searched for in the order that the entries have been defined. If no image is found, the space will be left blank unless the `default` property has been set. To use this property from the `system` view, you will first need to add a `gameselector` element. If `delay` is set to zero, then this property is ignored.
    - Valid values:
    - `image` - This will look for a `miximage`, and if that is not found `screenshot` is tried next, then `titlescreen` and finally `cover`
    - `miximage` - This will look for a miximage.
    - `marquee` - This will look for a marquee (wheel) image.
    - `screenshot` - This will look for a screenshot image.
    - `titlescreen` - This will look for a title screen image.
    - `cover` - This will look for a box front cover image.
    - `backcover` - This will look for a box back cover image.
    - `3dbox` - This will look for a 3D box image.
    - `physicalmedia` - This will look for a physical media image.
    - `fanart` - This will look for a fan art image.
* `gameselector` - type: STRING
    - If more than one gameselector element has been defined, this property makes it possible to state which one to use. If multiple gameselector elements have been defined and this property is missing then the first entry will be chosen and a warning message will be logged. If only a single gameselector has been defined, this property is ignored. The value of this property must match the `name` attribute value of the gameselector element.
* `audio` - type: BOOLEAN
    - Whether to enable or disable audio playback for the video.
    - Default is `true`
* `interpolation` - type: STRING
    - Interpolation method to use when scaling raster images. Nearest neighbor (`nearest`) preserves sharp pixels and linear filtering (`linear`) makes the image smoother. Note that this property only affects the static image, not the video scaling. This property also has no effect on scalable vector graphics (SVG) images.
    - Valid values are `nearest` or `linear`
    - Default is `nearest`
* `pillarboxes` - type: BOOLEAN
    - Whether to render black pillarboxes (and to a lesses extent letterboxes) for videos with aspect ratios where this is applicable. This is for instance useful for arcade game videos in vertical orientation.
    - Default is `true`
* `scanlines` - type: BOOLEAN
    - Whether to use a shader to render scanlines.
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
* `opacity` - type: FLOAT
    - Controls the level of transparency. If set to `0` the element will be disabled.
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

GIF and Lottie (vector graphics) animations. The type of animation is automatically selected based on the file extension with `.gif` for GIF animations and `.json` for Lottie animations. Note that Lottie animations take a lot of memory and CPU resources if scaled up to large sizes so it's adviced to not add too many of them to the same view and to not make them too large. GIF animations on the other hand are not as demanding except if they're really long and/or high-resolution.

Supported views:
* `system `
* `gamelist`

Instances per view:
* `unlimited`

Properties:
* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - If only one axis is specified (and the other is zero), the other will be automatically calculated in accordance with the animation's aspect ratio. Note that this is sometimes not entirely accurate as some animations contain invalid size information.
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0.5 0.5`
* `rotation` - type: FLOAT
    - Angle in degrees that the animation should be rotated. Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the animation will be rotated.
    - Default is `0.5 0.5`
* `path` - type: PATH
    - Path to the animation file. Only the .json extension is supported.
* `speed` - type: FLOAT.
    - The relative speed at which to play the animation.
    - Minimum value is `0.2` and maximum value is `3`
    - Default is `1`
* `direction` - type: STRING
    - The direction that the animation should be played. Valid values are `normal` (forwards), `reverse` (backwards), `alternate` (bouncing forwards/backwards) and `alternateReverse` (bouncing backwards/forwards, i.e. starting with playing backwards).
    - Default is `normal`
* `keepAspectRatio` - type: BOOLEAN.
    - If true, aspect ratio will be preserved. If false, animation will stretch to the defined size. Note that setting to `false` is incompatible with only defining one of the axes for the `size` element.
    - Default is `true`
* `interpolation` - type: STRING
    - Interpolation method to use when scaling GIF animations. Nearest neighbor (`nearest`) preserves sharp pixels and linear filtering (`linear`) makes the image smoother. This property has no effect on Lottie animations.
    - Valid values are `nearest` or `linear`
    - Default is `nearest`
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
    - Default is `0.5 0.5`
* `rotation` - type: FLOAT
    - Angle in degrees that the badges should be rotated. Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the image will be rotated.
    - Default is `0.5 0.5`.
* `horizontalAlignment` - type: STRING.
    - Valid values are `left` or `right`
* `direction` - type: STRING
    - Valid values are "row" or "column". Controls the primary layout direction (line axis) for the badges. Lines will fill up in the specified direction.
    - Default is `row`
* `lines` - type: UNSIGNED_INTEGER
    - The number of lines available.
    - Default is `3`
* `itemsPerLine` - type: UNSIGNED_INTEGER
    - Number of badges that fit on a line. When more badges are available a new line will be started.
    - Default is `4`
* `itemMargin` - type: NORMALIZED_PAIR
    - The horizontal and vertical margins between badges - `x y`
    - If one of the axis is set to `-1` the margin of the other axis (in pixels) will be used, which makes it possible to get identical spacing between all items regardless of screen aspect ratio.
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
    `gamepad_playstation`,
    `gamepad_sega_md_3_buttons`,
    `gamepad_sega_md_6_buttons`,
    `gamepad_xbox`,
    `joystick_generic`,
    `joystick_arcade_no_buttons`,
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

Displays text. This can be literal strings or values based on game metadata or system variables, as described below. For the `gamelist` view it's also possible to place the text inside a scrollable container which is for example useful for longer texts like the game descriptions.

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
    - Default is `0.5 0.5`
* `text` - type: STRING
    - A string literal to display.
* `systemdata` - type: STRING
    - This translates to some system data including values defined in es_systems.xml as well as some statistics. If an invalid systemdata field is defined, it will be printed as a string literal. This property can only be used in the `system` view.
    - Valid values:
    - `name` - Short system name as defined in es_systems.xml.
    - `fullname` - Full system name as defined in es_systems.xml.
    - `gamecount` - Number of games available for the system. Number of favorites are printed inside brackets if applicable.
    - `gamecount_games` - Number of games available for the system. Does not print the favorites count.
    - `gamecount_favorites` - Number of favorite games for the system, may be blank if favorites are not applicable.
* `metadata` - type: STRING
    - This translates to the metadata values that are available for the game. If an invalid metadata field is defined, it will be printed as a string literal. To use this property from the `system` view, you will first need to add a `gameselector` element.
     - Valid values:
    - `name` - Game name.
    - `description` - Game description. Should be combined with the `container` property in most cases.
    - `rating` - The numerical representation of the game rating, for example `3` or `4.5`.
    - `developer` - Developer.
    - `publisher` - Publisher.
    - `genre` - Genre.
    - `players` - The number of players.
    - `favorite` - Whether the game is a favorite. Will be printed as either `yes` or `no`.
    - `completed` - Whether the game has been completed. Will be printed as either `yes` or `no`.
    - `kidgame` - Whether the game is suitable for children. Will be printed as either `yes` or `no`.
    - `broken` - Whether the game is broken/not working. Will be printed as either `yes` or `no`.
    - `playcount` - How many times the game has been played.
    - `controller` - The controller for the game. Will be blank if none has been selected.
    - `altemulator` - The alternative emulator for the game. Will be blank if none has been selected.
* `gameselector` - type: STRING
    - If more than one gameselector element has been defined, this property makes it possible to state which one to use. If multiple gameselector elements have been defined and this property is missing then the first entry will be chosen and a warning message will be logged. If only a single gameselector has been defined, this property is ignored. The value of this property must match the `name` attribute value of the gameselector element. This property is only needed for the `system` view and only if the `metadata` property is utilized.
* `container` - type: BOOLEAN
    - Whether the text should be placed inside a scrollable container. Only available for the `gamelist` view.
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
    - Size of the font as a percentage of screen height (e.g. for a value of `0.1`, the text's height would be 10% of the screen height).
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
    - Default is `0.5 0.5`.
* `metadata` - type: STRING
    - This displays the metadata values that are available for the game. If an invalid metadata field is defined, the text "unknown" will be printed. To use this property from the `system` view, you will first need to add a `gameselector` element.
    - Valid values:
    - `releasedate` - The release date of the game.
    - `lastplayed` - The time the game was last played. This will be displayed as a value relative to the current date and time by default, but can be overridden using the `displayRelative` property.
* `gameselector` - type: STRING
    - If more than one gameselector element has been defined, this property makes it possible to state which one to use. If multiple gameselector elements have been defined and this property is missing then the first entry will be chosen and a warning message will be logged. If only a single gameselector has been defined, this property is ignored. The value of this property must match the `name` attribute value of the gameselector element. This property is only needed for the `system` view and only if the `metadata` property is utilized.
* `fontPath` - type: PATH
    - Path to a TrueType font (.ttf).
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height (e.g. for a value of `0.1`, the text's height would be 10% of the screen height).
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
* `displayRelative` - type: BOOLEAN.
    - Renders the datetime as a relative string (e.g. 'x days ago').
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
    - Default is `0.5 0.5`
* `fontPath` - type: PATH
    - Path to a TrueType font (.ttf).
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height (e.g. for a value of `0.1`, the text's height would be 10% of the screen height).
* `color` - type: COLOR
* `backgroundColor` - type: COLOR
* `horizontalAlignment` - type: STRING
    - Controls alignment on the X axis.
    - Valid values are `left`, `center` or `right`
    - Default is `left`
* `verticalAlignment` - type: STRING
    - Controls alignment on the Y axis.
    - Valid values are `top`, `center` or `bottom`
    - Default is `center`
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
    - Only one value is actually used. The other value should be zero (e.g. specify width OR height, but not both. This is done to maintain the aspect ratio.)
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the rating should be rotated. Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the rating will be rotated.
    - Default is `0.5 0.5`
* `gameselector` - type: STRING
    - If more than one gameselector element has been defined, this property makes it possible to state which one to use. If multiple gameselector elements have been defined and this property is missing then the first entry will be chosen and a warning message will be logged. If only a single gameselector has been defined, this property is ignored. The value of this property must match the `name` attribute value of the gameselector element. This property is only needed for the `system` view.
* `color` - type: COLOR
    - Multiply each pixel's color by this color. For example, an all-white image with `<color>FF0000</color>` would become completely red. You can also control the transparency of an image with `<color>FFFFFFAA</color>` - keeping all the pixels their normal color and only affecting the alpha channel.
* `filledPath` - type: PATH
    - Path to the "filled" rating image. Image must be square (width equals height).
* `unfilledPath` - type: PATH
    - Path to the "unfilled" rating image. Image must be square (width equals height).
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

#### carousel

A carousel for navigating and selecting games or systems.

On the system view when using fade transitions, any elements placed below or at the same zIndex value as the carousel will be faded to black during transitions, and any elements with a higher zIndex value than the carousel will be faded to transparent.

Supported views:
* `system`
* `gamelist`

Instances per view:
* `single`

Properties:
* `type` - type: STRING
    - Sets the carousel type and scroll direction.
    - Valid values are `horizontal`, `vertical`, `horizontal_wheel` or `vertical_wheel`.
    - Default is `horizontal`
* `size` - type: NORMALIZED_PAIR
    - Minimum value per axis is `0.05` and maximum value per axis is `1.5`
    - Default is `1 0.2325`
* `pos` - type: NORMALIZED_PAIR
    - Default is `0 0.38375`
* `origin` - type: NORMALIZED_PAIR
    - Where on the carousel `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the carousel exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
    - Default is `0 0`
* `color` - type: COLOR
    - Color of the carousel background panel. Setting a value of `00000000` makes the background panel transparent.
    - Default is `FFFFFFD8`
* `colorEnd` - type: COLOR
    - Setting this to something other than what is defined for `color` creates a color gradient on the background panel.
    - Default is `FFFFFFD8`
* `gradientType` - type: STRING
    - The direction to apply the color gradient if both `color` and `colorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `staticItem` - type: PATH
    - Path to a static image file. Most common extensions are supported (including .jpg, .png, and unanimated .gif). This property can only be used in the `system` view.
* `itemType` - type: STRING
    - This displays a game image of a certain media type, and can only be used in the `gamelist` view.
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
    - Default is `marquee`
* `defaultItem` - type: PATH
    - Path to the default image file which will be displayed if the image defined via the `staticItem` or `itemType` property is not found. Most common extensions are supported (including .jpg, .png, and unanimated .gif).
* `itemSize` - type: NORMALIZED_PAIR
    - Minimum value per axis is `0.05` and maximum value per axis is `1`
    - Default is `0.25 0.155`
* `itemScale` - type: FLOAT.
    - Selected item is increased in size by this scale
    - Minimum value is `0.5` and maximum value is `3`
    - Default is `1.2`
* `itemRotation` - type: FLOAT
    - Angle in degrees that the item should be rotated. Value should be positive.
    - This property only applies when `type` is "horizontal_wheel" or "vertical_wheel".
    - Default is `7.5`
* `itemRotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the item will be rotated.
    - This property only applies when `type` is "horizontal_wheel" or "vertical_wheel".
    - Default is `-3 0.5`
* `itemHorizontalAlignment` - type: STRING
    - Sets `staticItem` / `itemType` and `text` alignment relative to the carousel on the X axis, which applies when `type` is "vertical", "horizontal_wheel" or "vertical_wheel".
    - Valid values are `left`, `center` or `right`
    - Default is `center`
* `itemVerticalAlignment` - type: STRING
    - Sets `staticItem` / `itemType` and `text` alignment relative to the carousel on the Y axis, which applies when `type` is "horizontal", "horizontal_wheel" or "vertical_wheel".
    - Valid values are `top`, `center` or `bottom`
    - Default is `center`
* `wheelHorizontalAlignment` - type: STRING
    - Sets the alignment of the actual carousel inside the overall element area. This property only applies when `type` is "horizontal_wheel" or "vertical_wheel".
    - Valid values are `left`, `center` or `right`
    - Default is `center`
* `horizontalOffset` - type: FLOAT
    - Offsets the carousel horizontally inside its designated area, as defined by the `size` property. The value of this property is relative to the width of the carousel (with `1` being equivalent to its entire width). This property can be used to add a margin if using `itemHorizontalAlignment`.
    - Minimum value is `-1.0` and maximum value is `1`
    - Default is `0`
* `verticalOffset` - type: FLOAT
    - Offsets the carousel vertically inside its designated area, as defined by the `size` property. The value of this property is relative to the height of the carousel (with `1` being equivalent to its entire height). This can be used to add a margin if using `itemVerticalAlignment` but is even more useful if `reflections` has been set as it allows the control of how much of the reflections to display by relocating the carousel inside its clipping area.
    - Minimum value is `-1.0` and maximum value is `1`
    - Default is `0`
* `reflections` - type: BOOLEAN
    - Enables reflections beneath the carousel items. This is only available for the `horizontal` carousel type. It's probably a good idea to combine this with the `verticalOffset` property to define how much of the reflections should be visible.
* `reflectionsOpacity` - type: FLOAT
    - Defines the base opacity for the reflections.
    - Minimum value is `0.1` and maximum value is `1`
    - Default is `0.5`
* `reflectionsFalloff` - type: FLOAT
    - Defines the opacity falloff for the reflections, starting from the base opacity value. Setting this property to `1` will fade the bottom of the reflection to complete transparency. Setting it above `1` will lead to a more aggressive falloff.
    - Minimum value is `0` and maximum value is `5`
    - Default is `1`
* `unfocusedItemOpacity` - type: FLOAT
    - Sets the opacity for the items that are not currently focused.
    - Minimum value is `0.1` and maximum value is `1`
    - Default is `0.5`
* `maxItemCount` - type: FLOAT
    - Sets the number of items to display in the carousel.
    - Minimum value is `0.5` and maximum value is `30`
    - Default is `3`
* `text` - type: STRING
    - A string literal to display if there is no `staticItem` / `itemType` or `defaultItem` properties defined or if no images were found.
* `textColor` - type: COLOR
    - Default is `000000FF`
* `textBackgroundColor` - type: COLOR
    - Default is `FFFFFF00`
* `letterCase` - type: STRING
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is `none` (original letter case is retained)
* `fontPath` - type: PATH
    - Path to a TrueType font (.ttf) used as fallback if there is no `staticItem` / `itemType` image defined or found, and if `defaultItem` has not been defined.
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height (e.g. for a value of `0.1`, the text's height would be 10% of the screen height).
    - Default is `0.085`
* `lineSpacing` - type: FLOAT
    - Controls the space between lines (as a multiple of font height).
    - Minimum value is `0.5` and maximum value is `3`
    - Default is `1.5`
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
* `size` - type: NORMALIZED_PAIR
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen. If the position and size attributes are themeable, origin is implied.
    - Minimum value per axis is `0` and maximum value per axis is `1`
* `selectorHeight` - type: FLOAT
    - Height of the "selector bar".
* `selectorOffsetY` - type: FLOAT
    - Allows moving of the "selector bar" up or down from its computed position. Useful for fine tuning the position of the "selector bar" relative to the text.
* `selectorColor` - type: COLOR
    - Color of the selector bar.
    - Default is `000000FF`
* `selectorColorEnd` - type: COLOR
    - Setting this to something other than what is defined for `selectorColor` creates a color gradient.
    - Default is `000000FF`
* `selectorGradientType` - type: STRING
    - The direction to apply the color gradient if both `selectorColor` and `selectorColorEnd` have been defined.
    - Valid values are `horizontal` or `vertical`
    - Default is `horizontal`
* `selectorImagePath` - type: PATH
    - Path to image to render in place of "selector bar."
* `selectorImageTile` - type: BOOLEAN
    - If true, the selector image will be tiled instead of stretched to fit its size.
* `selectedColor` - type: COLOR
    - Color of the highlighted entry text.
* `primaryColor` - type: COLOR
    - Primary color; what this means depends on the text list. For example, for game lists, it is the color of a game.
* `secondaryColor` - type: COLOR
    - Secondary color; what this means depends on the text list. For example, for game lists, it is the color of a folder.
* `fontPath` - type: PATH
* `fontSize` - type: FLOAT
* `horizontalAlignment` - type: STRING
    - Controls alignment on the X axis.
    - Valid values are `left`, `center` or `right`
    - Default is `left`
* `horizontalMargin` - type: FLOAT
    - Horizontal offset for text from the alignment point. If `horizontalAlignment` is "left", offsets the text to the right. If `horizontalAlignment` is "right", offsets text to the left. No effect if `horizontalAlignment` is "center". Given as a percentage of the element's parent's width (same unit as `size`'s X value).
* `letterCase` - type: STRING
    - Valid values are `none`, `uppercase`, `lowercase` or `capitalize`
    - Default is `none` (original letter case is retained)
* `lineSpacing` - type: FLOAT
    - Controls the space between lines (as a multiple of font height).
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
* `zIndex` - type: FLOAT
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.
    - Default is `50`

#### gameselector

Selects games from the gamelists when navigating the `system` view. This makes it possible to display game media and game metadata directly from this view. It's possible to make separate gameselector configurations per game system, so that for instance a random game could be displayed for one system and the most recently played game could be displayed for another system. It's also possible to define multiple gameselector elements with different selection criterias per game system which makes it possible to for example set a random fan art background image and at the same time display a box cover image of the most recently played game. The gameselector logic can be used for the `image`, `video`, `text`, `datetime` and `rating` elements.

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
    - How many games to select. This property is only intended for future use.
    - Minimum value is `1` and maximum value is `30`
    - Default is `1`

#### helpsystem

The helpsystem is a special element that displays a context-sensitive list of actions the user can take at any time. You should try and keep the position constant throughout every screen. Note that this element does not have a zIndex value, instead it's always rendered on top of all other elements.

It's possible to set this element as right-aligned or center-aligned using a combination of the `pos` and `origin` properties. For example `<pos>0.99 0.954</pos>` and `<origin>1 0</origin>` will place it in the lower right corner of the screen.

Supported views:
* `system`
* `gamelist`

Instances per view:
* `single`

Properties:
* `pos` - type: NORMALIZED_PAIR
    - Default is `0.012 0.9515`
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
    - This property also implicitly sets the icon size and is therefore the means to change the overall size of the helpsystem element.
    - Default is `0.035`
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
      `button_a_SNES`,
      `button_b_SNES`,
      `button_x_SNES`,
      `button_y_SNES`,
      `button_back_SNES`,
      `button_start_SNES`,
      `button_a_PS`,
      `button_b_PS`,
      `button_x_PS`,
      `button_y_PS`,
      `button_back_PS4`,
      `button_start_PS4`,
      `button_back_PS5`,
      `button_start_PS5`,
      `button_a_XBOX`,
      `button_b_XBOX`,
      `button_x_XBOX`,
      `button_y_XBOX`,
      `button_back_XBOX`,
      `button_start_XBOX`,
      `button_back_XBOX360`,
      `button_start_XBOX360`

#### imagegrid

Deprecated.

* `pos` - type: NORMALIZED_PAIR.
* `size` - type: NORMALIZED_PAIR.
    - The size of the grid. Take care the selected tile can go out of the grid size, so don't position the grid too close to another element or the screen border.
* `margin` - type: NORMALIZED_PAIR. Margin between tiles.
* `padding` - type: NORMALIZED_RECT.
    - NEW : Padding for displaying tiles.
* `autoLayout` - type: NORMALIZED_PAIR.
    - NEW : Number of column and rows in the grid (integer values).
* `autoLayoutSelectedZoom` - type: FLOAT.
    - NEW : Zoom factor to apply when a tile is selected.
* `gameImage` - type: PATH.
    - The default image used for games which doesn't have an image.
* `folderImage` - type: PATH.
    - The default image used for folders which doesn't have an image.
* `imageSource` - type: STRING.
    - Selects the image to display. `thumbnail` by default, can also be set to `image`, `miximage`, `screenshot`, `cover`, `marquee` or `3dbox`. If selecting `image`, the media type `miximage` will be tried first, with fallback to `screenshot` and then `cover`.
* `scrollDirection` - type: STRING.
    - `vertical` by default, can also be set to `horizontal`. Not that in `horizontal` mod, the tiles are ordered from top to bottom, then from left to right.
* `centerSelection` - type: BOOLEAN.
    - `false` by default, when `true` the selected tile will be locked to the center of the grid.
* `scrollLoop` - type: BOOLEAN.
    - `false` by default, when `true` the grid will seamlessly loop around when scrolling reaches the end of the list. Only works when `centerSelection` is `true`.
* `animate` - type : BOOLEAN.
    - `true` by default, when  `false` the grid scrolling will not be animated.
* `zIndex` - type: FLOAT.
    - z-index value for element. Elements will be rendered in order of zIndex value from low to high.

#### gridtile

Deprecated.

* `size` - type: NORMALIZED_PAIR.
    - The size of the default gridtile is used to calculate how many tiles can fit in the imagegrid. If not explicitly set, the size of the selected gridtile is equal the size of the default gridtile * 1.2
* `padding` - type: NORMALIZED_PAIR.
    - The padding around the gridtile content. Default `16 16`. If not explicitly set, the selected tile padding will be equal to the default tile padding.
* `imageColor` - type: COLOR.
    - The default tile image color and selected tile image color have no influence on each others.
* `backgroundImage` - type: PATH.
    - If not explicitly set, the selected tile background image will be the same as the default tile background image.
* `backgroundCornerSize` - type: NORMALIZED_PAIR.
    - The corner size of the ninepatch used for the tile background. Default is `16 16`.
* `backgroundColor` - type: COLOR.
    - A shortcut to define both the center color and edge color at the same time. The default tile background color and selected tile background color have no influence on each others.
* `backgroundCenterColor` - type: COLOR.
    - Sets the color of the center part of the ninepatch. The default tile background center color and selected tile background center color have no influence on each others.
* `backgroundEdgeColor` - type: COLOR.
    - Sets the color of the edge parts of the ninepatch. The default tile background edge color and selected tile background edge color have no influence on each others.
