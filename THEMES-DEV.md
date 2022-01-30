# EmulationStation Desktop Edition (ES-DE) v1.3 (development version) - Themes

**Note:** This document is only relevant for the current ES-DE development version, if you would like to see the documentation for the latest stable release, refer to [THEMES.md](THEMES.md) instead.

If creating theme sets specifically for ES-DE, please add `-DE` to the theme name, as in `rbsimple-DE`. Because the ES-DE theme support has already deviated somehow from the RetroPie EmulationStation fork and will continue to deviate further in the future, the theme set will likely not be backwards compatible. It would be confusing and annoying for a user that downloads and attempts to use an ES-DE theme set in another EmulationStation fork only to get crashes, error messages or corrupted graphics. At least the -DE extension is a visual indicator that it's an ES-DE specific theme set.

Table of contents:

[[_TOC_]]

## Introduction

ES-DE allows the grouping of themes for multiple game systems into a **theme set**. A theme is a collection of **elements**, each with their own **properties** that define the way they look and behave. These elements include things like text lists, carousels, images and animations.

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
`/usr/share/emulationstation/themes/rbsimple-DE/`

If a theme set with the same name exists in both locations, the one in the home directory will be loaded and the other one will be skipped.

## Differences to legacy RetroPie themes

If you are not familiar with theming for RetroPie or similar forks of EmulationStation you can skip this section as it only describes the key differences between the updated ES-DE themes and these _legacy_ theme sets. The term _legacy_ is used throughout this document to refer to this older style of themes which ES-DE still fully supports for backward compatibility reasons. The old theme format is described in [THEMES-LEGACY.md](THEMES-LEGACY.md) although this document is basically a historical artifact by now.

With ES-DE v1.3 a new theme engine was introduced that fundamentally changed some aspects of how theming works. The previous design used specific view styles (basic, detailed, video and grid) and this was dropped completely and replaced with _variants_ that can accomplish the same thing while being much more powerful and flexible.

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
* Correct theme structure is enforced more strictly than before, and deviations will generate error log messages and make the theme loading fail

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

An element is a particular visual component such as an image, an animation or a piece of text. It has a mandatory _name_ attribute which is used by ES-DE to track each element entry. By using this name attribute it's possible to split up the definition of an element to different locations. For example you may want to define the color properties separately from where the size and position are configured (see the example below).

This is the element structure:

```xml
<ElementTypeHere name="ElementNameHere">
    ... define properties here ...
</ElementTypeHere>
```

Finally _properties_ control how a particular element looks and behaves, for example its position, size, image path, animation controls etc.  The property type determines what kinds of values you can use.  You can read about each type below in the
[Reference](THEMES-DEV.md#reference) section.  Properties are defined like this:

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

If you are writing a theme it's recommended to launch ES-DE with the `--debug` flag from a terminal window. You can also pass the `--resolution` flag to avoid having the application window fill the entire screen. By doing so, you can read error messages directly in the terminal window without having to open the es_log.txt file.  You can also reload the current gamelist or system view with `Ctrl+r` if the `--debug` flag has been set. There is also support for highlighting the size and position of each image and animation element by using the `Ctrl+i` key combination and likewise to highlight each text element by using the `Ctrl+t` keys. Again, both of these require that ES-DE has been launched with the `--debug` command line option, for example:
```
emulationstation --debug --resolution 1280 720
```

Enforcement of a correct theme configuration is quite strict, and most errors will abort the theme loading, leading to an unthemed system. In each such situation the log output will be very clear of what happened, for instance:
```
Jan 28 17:17:30 Error:  ThemeData::parseElement(): "/home/myusername/.emulationstation/themes/mythemeset-DE/theme.xml": Property "origin" for element "image" has no value defined
```

Sanitization for valid data format and structure is done in this manner, but verification that property values are actually correct (or reasonable) is handled by the individual component that takes care of creating and rendering the specific theme element. What happens in most instances is that a log warning entry is created and the invalid property is reset to its default value. So for these situations, the system will not become unthemed. Here's an example where a badges element accidentally had its alignment property set to _leftr_ instead of _left_:
```
Jan 28 17:25:27 Warn:   BadgeComponent: Invalid theme configuration, <alignment> set to "leftr"
```

### Variants

A core concept of ES-DE is the use of theme set _variants_ to provide different theme profiles. These are not fixed presets and a theme author can instead name and define whatever variants he wants for his theme (or possibly use no variants at all as they are optional).

The variants could be purely cosmetic, such as providing light and dark mode versions of the theme set, or they could provide different functionality by for instance using different primary components such as a carousel or a text list.

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
            <image name="md_image">
                <metadata>md_image</metadata>
                <zIndex>40</zIndex>
            </image>
        </view>
    </variant>

    <variant name="lightMode, darkMode">
        <view name="gamelist">
            <video name="md_video">
                <metadata>md_video</metadata>
                <delay>1.7</delay>
                <showSnapshotNoVideo>true</showSnapshotNoVideo>
                <showSnapshotDelay>true</showSnapshotDelay>
            </video>
        </view>
    </variant>
</theme>

<!-- The following is NOT supported as <variant> tags can't be located inside <view> tag pairs -->
<theme>
    <view name="gamelist">
        <variant name="lightModeNoVideo">
            <image name="md_image">
                <metadata>md_image</metadata>
                <zIndex>40</zIndex>
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
            <image name="md_image">
                <pos>0.3 0.56</pos>
            </image>
        </view>
    </aspectRatio>

    <aspectRatio name="16:9, 16:10, 21:9">
        <view name="gamelist">
            <image name="md_image">
                <pos>0.42 0.31</pos>
            </image>
        </view>
    </aspectRatio>
</theme>

<!-- The following is NOT supported as <aspectRatio> tags can't be located inside <view> tag pairs -->
<theme>
    <view name="gamelist">
        <aspectRatio name="4:3, 5:4">
            <image name="md_image">
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

This file type was introduced with the new ES-DE theme engine in v1.3 and is an indicator that the theme set is of the new generation instead of being of the legacy type (i.e. a theme set backward compatible with RetroPie EmulationStation). In other words, if the capabilities.xml file is absent, the theme will get loaded as a legacy set.

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
| 12:5             |                | 3840x1600                                      |
| 43:18            |                | 3440x1440                                      |
| 64:27            |                | 2560x1080                                      |

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

Theme variables can be used to simplify theme construction.  There are 2 types of variables available.
* System variables
* Theme defined variables

#### System variables

System variables are system specific and are derived from the values in es_systems.xml.
* `system.name`
* `system.fullName`
* `system.theme`

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

## Reference

This section describes each element and their properties in detail and also contains example configuration snippets.

### Property data types

* NORMALIZED_PAIR - two decimals, in the range [0..1], delimited by a space.  For example, `0.25 0.5`.  Most commonly used for position (x and y coordinates) and size (width and height).
* NORMALIZED_RECT - four decimals, in the range [0..1], delimited by a space. For example, `0.25 0.5 0.10 0.30`.  Most commonly used for padding to store top, left, bottom and right coordinates.
* PATH - a path.  If the first character is a `~`, it will be expanded into the environment variable for the home path (`$HOME` for Unix and macOS or `%HOMEPATH%` for Windows) unless overridden using the --home command line option.  If the first character is a `.`, it will be expanded to the theme file's directory, allowing you to specify resources relative to the theme file, like so: `./../core/fonts/myfont.ttf`.
* BOOLEAN - `true`/`1` or `false`/`0`.
* COLOR - a hexidecimal RGB or RGBA color (6 or 8 digits).  If 6 digits, will assume the alpha channel is `FF` (completely opaque).
* FLOAT - a decimal.
* STRING - a string of text.

### Element types and their properties

Common to almost all elements is a `pos` and `size` property of the NORMALIZED_PAIR type.  They are normalized in terms of their "parent" object's size; 99% of the time this is just the size of the screen.  In this case, `<pos>0 0</pos>` would correspond to the top left corner, and `<pos>1 1</pos>` the bottom right corner (a positive Y value points further down). You can also use numbers outside of the [0..1] range if you want to place an element partially or completely off-screen.

The order in which you define properties does not matter and you only need to define the ones where you want to override the default value.

#### image

* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - If only one axis is specified (and the other is zero), the other will be automatically calculated in accordance with the image's aspect ratio.
* `maxSize` - type: NORMALIZED_PAIR
    - The image will be resized as large as possible so that it fits within this size and maintains its aspect ratio.  Use this instead of `size` when you don't know what kind of image you're using so it doesn't get grossly oversized on one axis (e.g. with a game's image metadata).
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen.  If the position and size attributes are themeable, origin is implied.
    - Default is `0.5 0.5`
* `rotation` - type: FLOAT
    - Angle in degrees that the image should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the image will be rotated.
    - Default is `0.5 0.5`
* `path` - type: PATH
    - Path to the image file.  Most common extensions are supported (including .jpg, .png, and unanimated .gif).
* `default` - type: PATH
    - Path to a default image file. The default image will be displayed when the selected game does not have an image of the type defined by the `metadata` property. It's also applied to any custom collection that does not contain any games when browsing the grouped custom collections system.
* `tile` - type: BOOLEAN
    - If true, the image will be tiled instead of stretched to fit its size.  Useful for backgrounds.
* `metadata` - type: STRING
    - This displays a game image of a certain media type. If no image of the requested type is found, the space will be left blank unless the `default` property has been set. If the metadata type is set to an invalid value, no image will be displayed regardless of whether the `default` property has been defined or not.
    - Possible values:
    - `md_image` - This will look for a miximage, but if that is not found screenshot is tried next, then title screen and finally box front cover.
    - `md_miximage` - This will look for a miximage.
    - `md_marquee` - This will look for a marquee image.
    - `md_screenshot` - This will look for a screenshot image.
    - `md_titlescreen` - This will look for a title screen image.
    - `md_cover` - This will look for a box front cover image.
    - `md_backcover` - This will look for a box back cover image.
    - `md_3dbox` - This will look for a 3D box image.
    - `md_fanart` - This will look for a fan art image.
* `color` - type: COLOR
    - Multiply each pixel's color by this color. For example, an all-white image with `<color>FF0000</color>` would become completely red.  You can also control the transparency of an image with `<color>FFFFFFAA</color>` - keeping all the pixels their normal color and only affecting the alpha channel.
* `colorEnd` - type: COLOR
    - Works exactly in the same way as `color` but can be set as the end color to apply a color shift gradient to the image.
* `gradientType` - type: STRING
    - The direction to apply the color shift gradient if both `color` and `colorEnd` have been defined.
    - Possible values are `horizontal` or `vertical`
    - Default is `horizontal`
* `scrollFadeIn` - type: BOOLEAN
    - If enabled, a short fade-in animation will be applied when scrolling through games in the gamelist view. This usually looks best if used for the main game image.
    - Default is `false`
* `visible` - type: BOOLEAN
    - If true, element will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view.
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element.  Elements will be rendered in order of zIndex value from low to high.
    - Default is `30`

#### video

* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - If only one axis is specified (and the other is zero), the other will be automatically calculated in accordance with the video's aspect ratio.
* `maxSize` - type: NORMALIZED_PAIR
    - The video will be resized as large as possible so that it fits within this size and maintains its aspect ratio.  Use this instead of `size` when you don't know what kind of video you're using so it doesn't get grossly oversized on one axis (e.g. with a game's video metadata).
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen.  If the position and size attributes are themeable, origin is implied.
    - Default is `0.5 0.5`
* `rotation` - type: FLOAT
    - Angle in degrees that the text should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the text will be rotated.
    - Default is `0.5 0.5`
* `delay` - type: FLOAT
    - Delay in seconds before video will start playing. During the delay period the game image defined via the `imageMetadata` property will be displayed. If that property is not set, then the `delay` property will be ignored.
    - Default is `0`
* `path` - type: PATH
    - Path to a video file. Setting a value for this property will make the video static, i.e. it will only play this video regardless of whether there is a game video available or not. If the `default` property has also been set, it will be overridden as the `path` property takes precedence.
* `default` - type: PATH
    - Path to a default video file. The default video will be played (unless the `path` property has been set) when the selected game does not have a video. If an image type has been defined using `imageMetadata` that image will be searched for first and only if no such image could be found this `default` video will be shown. This property is also applied to any custom collection that does not contain any games when browsing the grouped custom collections system.
* `defaultImage` - type: PATH
    - This works exactly as the `default` property but it allows displaying an image instead of playing a video.
* `imageMetadata` - type: STRING
    - This displays a game image of a certain media type. This occurs if there is no video found for the game and the `path` and `default` properties have not been set, or if a video start delay has been defined via the `delay` attribute.
    - Possible values:
    - `md_image` - This will look for a miximage, but if that is not found screenshot is tried next, then title screen and finally box front cover.
    - `md_miximage` - This will look for a miximage.
    - `md_marquee` - This will look for a marquee image.
    - `md_screenshot` - This will look for a screenshot image.
    - `md_titlescreen` - This will look for a title screen image.
    - `md_cover` - This will look for a box front cover image.
    - `md_backcover` - This will look for a box back cover image.
    - `md_3dbox` - This will look for a 3D box image.
    - `md_fanart` - This will look for a fan art image.
* `scrollFadeIn` - type: BOOLEAN
    - If enabled, a short fade-in animation will be applied when scrolling through games in the gamelist view. This animation is only applied to images and not to actual videos, so if no image metadata has been defined then this property has no effect. For this to work correctly the `delay` property also needs to be set.
    - Default is `false`
* `visible` - type: BOOLEAN
    - If true, element will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view.
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element.  Elements will be rendered in order of zIndex value from low to high.
    - Default is `30`

#### animation

Lottie (vector graphics) animation.

* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - If only one axis is specified (and the other is zero), the other will be automatically calculated in accordance with the animation's aspect ratio. Note that this is sometimes not entirely accurate as some animations contain invalid size information.
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen.  If the position and size attributes are themeable, origin is implied.
    - Default is `0.5 0.5`
* `rotation` - type: FLOAT
    - Angle in degrees that the animation should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the animation will be rotated.
    - Default is `0.5 0.5`
* `path` - type: PATH
    - Path to the animation file.  Only the .json extension is supported.
* `speed` - type: FLOAT.
    - The relative speed at which to play the animation.
    - Minimum value is `0.2` and maximum value is `3.0`
    - Default is `1.0`
* `direction` - type: STRING
    - The direction that the animation should be played. Valid values are `normal` (forwards), `reverse` (backwards), `alternate` (bouncing forwards/backwards) and `alternateReverse` (bouncing backwards/forwards, i.e. starting with playing backwards).
    - Default is `normal`
* `keepAspectRatio` - type: BOOLEAN.
    - If true, aspect ratio will be preserved. If false, animation will stretch to the defined size. Note that setting to `false` is incompatible with only defining one of the axes for the `size` element.
    - Default is `true`
* `visible` - type: BOOLEAN
    - If true, element will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view.
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element.  Elements will be rendered in order of zIndex value from low to high.
    - Default is `35`

#### badges

It's strongly recommended to use the same image dimensions for all badges as varying aspect ratios will lead to alignment issues. For the controller images it's recommended to keep to the square canvas size used by the default bundled graphics as otherwise sizing and placement will be inconsistent (unless all controller graphic files are customized of course).

* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - Possible combinations:
    - `w h` - Dimensions of the badges container. The badges will be scaled to fit within these dimensions.
    - Minimum value per axis is `0.03` and maximum value per axis is `1.0`
    - Default is `0.15 0.20`
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen.  If the position and size attributes are themeable, origin is implied.
    - Default is `0.5 0.5`
* `rotation` - type: FLOAT
    - Angle in degrees that the badges should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the image will be rotated.
    - Default is `0.5 0.5`.
* `direction` - type: STRING
    - Valid values are "row" or "column". Controls the primary layout direction (line axis) for the badges. Lines will fill up in the specified direction.
    - Default is `row`
* `lines` - type: FLOAT
    - The number of lines available.
    - Default is `2`
* `itemsPerLine` - type: FLOAT
    - Number of badges that fit on a line. When more badges are available a new line will be started.
    - Default is `4`
* `itemMargin` - type: NORMALIZED_PAIR
    - The horizontal and vertical margins between badges - `x y`
    - If one of the axis is set to `-1` the margin of the other axis (in pixels) will be used, which makes it possible to get identical spacing between all items regardless of screen aspect ratio.
    - Minimum value per axis is `0` and maximum value per axis is `0.2`
    - Default is `0.01 0.01`.
* `slots` - type: STRING
    - The badge types that should be displayed. Should be specified as a list of strings delimited by commas or by whitespace characters (tabs, spaces or line breaks). The order in which they are defined will be followed when placing badges on screen. Available badges are:
    - `favorite` - Will be shown when the game is marked as favorite.
    - `completed` - Will be shown when the game is marked as completed.
    - `kidgame` - Will be shown when the game is marked as a kids game.
    - `broken` - Will be shown when the game is marked as broken.
    - `controller` - Will be shown and overlaid by the corresponding controller icon if a controller type has been selected for the game (using the metadata editor or via scraping).
    - `altemulator` - Will be shown when an alternative emulator is setup for the game.
* `controllerPos` - type: NORMALIZED_PAIR
    - The position of the controller icon relative to the parent `controller` badge.
    - Minimum value per axis is `-1.0` and maximum value per axis is `2.0`
    - Default is `0.5 0.5` which centers the controller icon on the badge.
* `controllerSize` - type: FLOAT
    - The size of the controller icon relative to the parent `controller` badge.
    - Setting the value to `1.0` sizes the controller icon to the same width as the parent badge. The image aspect ratio is always maintained.
    - Minimum value is `0.1` and maximum value is `2.0`
* `customBadgeIcon` - type: PATH
    - A badge icon override. Specify the badge type in the attribute `badge`. The available badges are the ones listed above.
* `customControllerIcon` - type: PATH
    - A controller icon override. Specify the controller type in the attribute `controller`. These are the available types:
    - `gamepad_generic`, `gamepad_nintendo_nes`, `gamepad_nintendo_snes`, `gamepad_nintendo_64`, `gamepad_playstation`, `gamepad_sega_md_3_buttons`, `gamepad_sega_md_6_buttons`, `gamepad_xbox`, `joystick_generic`, `joystick_arcade_no_buttons`, `joystick_arcade_1_button`, `joystick_arcade_2_buttons`, `joystick_arcade_3_buttons`, `joystick_arcade_4_buttons`, `joystick_arcade_5_buttons`, `joystick_arcade_6_buttons`, `keyboard_generic`, `keyboard_and_mouse_generic`, `mouse_generic`, `mouse_amiga`, `lightgun_generic`, `lightgun_nintendo`, `steering_wheel_generic`, `flight_stick_generic`, `spinner_generic`, `trackball_generic`, `wii_remote_nintendo`, `wii_remote_and_nunchuk_nintendo`, `joycon_left_or_right_nintendo`, `joycon_pair_nintendo`, `unknown`
* `visible` - type: BOOLEAN
    - If true, element will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view.
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element.  Elements will be rendered in order of zIndex value from low to high.
    - Default is `35`

#### text

* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - Possible combinations:
    - `0 0` - automatically size so text fits on one line (expanding horizontally).
    - `w 0` - automatically wrap text so it doesn't go beyond `w` (expanding vertically).
    - `w h` - works like a "text box."  If `h` is non-zero and `h` <= `fontSize` (implying it should be a single line of text), text that goes beyond `w` will be truncated with an elipses (...).
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen.  If the position and size attributes are themeable, origin is implied.
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the text should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise. Rotation is not possible if the `container` property has been set to true.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the text will be rotated.
    - Default is `0.5 0.5`
* `text` - type: STRING
    - A string literal to display.
* `metadata` - type: STRING
    - This translates to the metadata values that are available for the game. If an invalid metadata field is defined, it will be printed as a string literal.
    - Possible values:
    - `md_name` - Game name.
    - `md_description` - Game description. Should be combined with the `container` property in most cases.
    - `md_rating` - The numerical representation of the game rating, for example `3` or `4.5`.
    - `md_developer` - Developer.
    - `md_publisher` - Publisher.
    - `md_genre` - Genre.
    - `md_players` - The number of players.
    - `md_favorite` - Whether the game is a favorite. Will be printed as either `Yes` or `No`.
    - `md_completed` - Whether the game has been completed. Will be printed as either `Yes` or `No`.
    - `md_kidgame` - Whether the game is suitable for children. Will be printed as either `Yes` or `No`.
    - `md_broken` - Whether the game is broken/not working. Will be printed as either `Yes` or `No`.
    - `md_playcount` - How many times the game has been played.
    - `md_controller` - The controller for the game. Will be blank if none has been selected.
    - `md_altemulator` - The alternative emulator for the game. Will be blank if none has been selected.
* `container` - type: BOOLEAN
    - Whether the text should be placed inside a scrollable container.
* `fontPath` - type: PATH
    - Path to a truetype font (.ttf).
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height (e.g. for a value of `0.1`, the text's height would be 10% of the screen height).
* `alignment` - type: STRING
    - Valid values are `left`, `center`, or `right`.  Controls alignment on the X axis and `center` will also align vertically.
* `color` - type: COLOR
* `backgroundColor` - type: COLOR
* `forceUppercase` - type: BOOLEAN
    - Draw text in uppercase.
* `lineSpacing` - type: FLOAT
    - Controls the space between lines (as a multiple of font height).
    - Default is `1.5`
* `visible` - type: BOOLEAN
    - If true, element will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view. This property will be ignored for elements defined for the `gamelist` view where a `metadata` property has been set. That is so because the "Hide metadata fields" functionality will hide and unhide metadata text fields as needed.
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element.  Elements will be rendered in order of zIndex value from low to high.
    - Default is `40`

#### datetime
* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - Possible combinations:
    - `0 0` - automatically size so text fits on one line (expanding horizontally).
    - `w 0` - automatically wrap text so it doesn't go beyond `w` (expanding vertically).
    - `w h` - works like a "text box."  If `h` is non-zero and `h` <= `fontSize` (implying it should be a single line of text), text that goes beyond `w` will be truncated with an elipses (...).
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen.  If the position and size attributes are themeable, origin is implied.
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the text should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the text will be rotated.
    - Default is `0.5 0.5`.
* `metadata` - type: STRING
    - This displays the metadata values that are available for the game. If an invalid metadata field is defined, the text "unknown" will be printed.
    - Possible values:
    - `md_releasedate` - The release date of the game.
    - `md_lastplayed` - The time the game was last played. This will be displayed as a value relative to the current date and time.
* `fontPath` - type: PATH
    - Path to a truetype font (.ttf).
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height (e.g. for a value of `0.1`, the text's height would be 10% of the screen height).
* `alignment` - type: STRING
    - Valid values are `left`, `center`, or `right`.  Controls alignment on the X axis and `center` will also align vertically.
* `color` - type: COLOR
* `backgroundColor` - type: COLOR
* `forceUppercase` - type: BOOLEAN
    - Draw text in uppercase.
* `lineSpacing` - type: FLOAT
    - Controls the space between lines (as a multiple of font height).
    - Default is `1.5`
* `format` - type: STRING. Specifies format for rendering datetime.
    - %Y: The year, including the century (1900)
    - %m: The month number [01,12]
    - %d: The day of the month [01,31]
    - %H: The hour (24-hour clock) [00,23]
    - %M: The minute [00,59]
    - %S: The second [00,59]
* `displayRelative` - type: BOOLEAN.
    - Renders the datetime as a a relative string (ex: 'x days ago').
* `visible` - type: BOOLEAN
    - If true, element will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view. This property will be ignored for elements defined for the `gamelist` view. That is so because the "Hide metadata fields" functionality will hide and unhide metadata text fields as needed.
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element.  Elements will be rendered in order of zIndex value from low to high.
    - Default is `40`

#### gamelistinfo

Displays the game count (all games as well as favorites), any applied filters, and a folder icon if a folder has been entered. If this text is left aligned or center aligned, the folder icon will be placed to the right of the other information, and if it's right aligned, the folder icon will be placed to the left.

* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - Possible combinations:
    - `0 0` - automatically size so text fits on one line (expanding horizontally).
    - `w 0` - automatically wrap text so it doesn't go beyond `w` (expanding vertically).
    - `w h` - works like a "text box."  If `h` is non-zero and `h` <= `fontSize` (implying it should be a single line of text), text that goes beyond `w` will be truncated with an elipses (...).
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen.  If the position and size attributes are themeable, origin is implied.
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the text should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the element will be rotated.
    - Default is `0.5 0.5`
* `backgroundColor` - type: COLOR
* `fontPath` - type: PATH
    - Path to a truetype font (.ttf).
* `fontSize` - type: FLOAT
    - Size of the font as a percentage of screen height (e.g. for a value of `0.1`, the text's height would be 10% of the screen height).
* `color` - type: COLOR
* `alignment` - type: STRING
    - Valid values are `left`, `center`, or `right`.  Controls alignment on the X axis and `center` will also align vertically.
* `visible` - type: BOOLEAN
    - If true, element will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view.
    - Default is `true`
* `zIndex` - type: FLOAT
    - z-index value for element.  Elements will be rendered in order of zIndex value from low to high.
    - Default is `45`

#### rating

* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
    - Only one value is actually used. The other value should be zero.  (e.g. specify width OR height, but not both.  This is done to maintain the aspect ratio.)
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen.  If the position and size attributes are themeable, origin is implied.
    - Default is `0 0`
* `rotation` - type: FLOAT
    - Angle in degrees that the rating should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
    - Default is `0`
* `rotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the rating will be rotated.
    - Default is `0.5 0.5`
* `color` - type: COLOR
    - Multiply each pixel's color by this color. For example, an all-white image with `<color>FF0000</color>` would become completely red.  You can also control the transparency of an image with `<color>FFFFFFAA</color>` - keeping all the pixels their normal color and only affecting the alpha channel.
* `filledPath` - type: PATH
    - Path to the "filled" rating image.  Image must be square (width equals height).
* `unfilledPath` - type: PATH
    - Path to the "unfilled" rating image.  Image must be square (width equals height).
* `zIndex` - type: FLOAT
    - z-index value for element.  Elements will be rendered in order of zIndex value from low to high.
    - Default is `45`

#### carousel

* `type` - type: STRING
    - Sets the scoll direction of the carousel.
    - Accepted values are `horizontal`, `vertical`, `horizontal_wheel` or `vertical_wheel`.
    - Default is `horizontal`
* `size` - type: NORMALIZED_PAIR
    - Default is `1 0.2325`
* `pos` - type: NORMALIZED_PAIR
    - Default is `0 0.38375`
* `origin` - type: NORMALIZED_PAIR
    - Where on the carousel `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the carousel exactly in the middle of the screen.  If the position and size attributes are themeable, origin is implied.
* `color` - type: COLOR
    - Controls the color of the carousel background.
    - Default is `FFFFFFD8`
* `logoSize` - type: NORMALIZED_PAIR
    - Default is `0.25 0.155`
* `logoScale` - type: FLOAT.
    - Selected logo is increased in size by this scale
    - Default is `1.2`
* `logoRotation` - type: FLOAT
    - Angle in degrees that the logos should be rotated.  Value should be positive.
    - This property only applies when `type` is "horizontal_wheel" or "vertical_wheel".
    - Default is `7.5`
* `logoRotationOrigin` - type: NORMALIZED_PAIR
    - Point around which the logos will be rotated.
    - This property only applies when `type` is "horizontal_wheel" or "vertical_wheel".
    - Default is `-5 0.5`
* `logoAlignment` - type: STRING
    - Sets the alignment of the logos relative to the carousel.
    - Accepted values are `top`, `bottom` or `center` when `type` is "horizontal" or "horizontal_wheel".
    - Accepted values are `left`, `right` or `center` when `type` is "vertical" or "vertical_wheel".
    - Default is `center`
* `maxLogoCount` - type: FLOAT
    - Sets the number of logos to display in the carousel.
    - Default is `3`
* `zIndex` - type: FLOAT
    - z-index value for element.  Elements will be rendered in order of zIndex value from low to high.
    - Default is `50`
* `legacyZIndexMode` - type: BOOLEAN
    - If true, the carousel will ignore zIndex and always render on top of other elements.
    - Default is `true`

#### textlist

This is a list containing rows of text which can be navigated using the keyboard or a controller.

* `pos` - type: NORMALIZED_PAIR
* `size` - type: NORMALIZED_PAIR
* `origin` - type: NORMALIZED_PAIR
    - Where on the element `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the element exactly in the middle of the screen.  If the position and size attributes are themeable, origin is implied.
* `selectorColor` - type: COLOR
    - Color of the "selector bar."
* `selectorImagePath` - type: PATH
    - Path to image to render in place of "selector bar."
* `selectorImageTile` - type: BOOLEAN
    - If true, the selector image will be tiled instead of stretched to fit its size.
* `selectorHeight` - type: FLOAT
    - Height of the "selector bar".
* `selectorOffsetY` - type: FLOAT
    - Allows moving of the "selector bar" up or down from its computed position.  Useful for fine tuning the position of the "selector bar" relative to the text.
* `selectedColor` - type: COLOR
    - Color of the highlighted entry text.
* `primaryColor` - type: COLOR
    - Primary color; what this means depends on the text list.  For example, for game lists, it is the color of a game.
* `secondaryColor` - type: COLOR
    - Secondary color; what this means depends on the text list.  For example, for game lists, it is the color of a folder.
* `fontPath` - type: PATH
* `fontSize` - type: FLOAT
* `alignment` - type: STRING
    - Valid values are `left`, `center`, or `right`.  Controls alignment on the X axis.
* `horizontalMargin` - type: FLOAT
    - Horizontal offset for text from the alignment point.  If `alignment` is "left", offsets the text to the right.  If `alignment` is "right", offsets text to the left.  No effect if `alignment` is "center".  Given as a percentage of the element's parent's width (same unit as `size`'s X value).
* `forceUppercase` - type: BOOLEAN
    - Draw text in uppercase.
* `lineSpacing` - type: FLOAT
    - Controls the space between lines (as a multiple of font height).
    - Default is `1.5`
* `zIndex` - type: FLOAT
    - z-index value for element.  Elements will be rendered in order of zIndex value from low to high.
    - Default is `50`

#### helpsystem

The help system is a special element that displays a context-sensitive list of actions the user can take at any time.  You should try and keep the position constant throughout every screen.  Keep in mind the "default" settings (including position) are used whenever the user opens a menu.

* `pos` - type: NORMALIZED_PAIR. Default is "0.012 0.9515"
* `origin` - type: NORMALIZED_PAIR.
    - Where on the element `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place
      the element exactly in the middle of the screen.
* `textColor` - type: COLOR. Default is 777777FF.
* `textColorDimmed` - type: COLOR. Default is the same value as textColor. Must be placed under the 'system' view.
* `iconColor` - type: COLOR. Default is 777777FF.
* `iconColorDimmed` - type: COLOR. Default is the same value as iconColor. Must be placed under the 'system' view.
* `fontPath` - type: PATH.
* `fontSize` - type: FLOAT.
* `entrySpacing` - type: FLOAT. Default is 16.0.
    - Spacing in pixels between the help system elements.
* `iconTextSpacing` - type: FLOAT. Default is 8.0.
    - Spacing in pixels within a help system element between it's icon and text.
* `textStyle` - type: STRING. Default is `uppercase`.
    - The style of the text. Options: `uppercase`, `lowercase`, `camelcase`.
* `customButtonIcon` - type: PATH.
    - A button icon override. Specify the button type in the attribute `button`. The available buttons are:
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
      `button_start_XBOX360`.

#### imagegrid

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
    - `false` by default, when `true` the grid will seamlessly loop around when scrolling reaches the end of the list.  Only works when `centerSelection` is `true`.
* `animate` - type : BOOLEAN.
    - `true` by default, when  `false` the grid scrolling will not be animated.
* `zIndex` - type: FLOAT.
    - z-index value for element.  Elements will be rendered in order of zIndex value from low to high.

#### gridtile

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
    - Set the color of the center part of the ninepatch. The default tile background center color and selected tile background center color have no influence on each others.
* `backgroundEdgeColor` - type: COLOR.
    - Set the color of the edge parts of the ninepatch. The default tile background edge color and selected tile background edge color have no influence on each others.

## Example legacy theme sets

To see some example EmulationStation legacy themes, the following resources are recommended:

https://aloshi.com/emulationstation#themes

https://github.com/RetroPie

https://gitlab.com/recalbox/recalbox-themes

https://wiki.batocera.org/themes
