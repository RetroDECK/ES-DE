# EmulationStation Desktop Edition (ES-DE) v1.2 (development version) - Themes

**Note:** This document is only relevant for the current ES-DE development version, if you would like to see the documentation for the latest stable release, refer to [THEMES.md](THEMES.md) instead.

If creating theme sets specifically for ES-DE, please add `-DE` to the theme name, as in `rbsimple-DE`. Because the ES-DE theme support has already deviated somehow from the RetroPie EmulationStation fork and will continue to deviate further in the future, the theme set will likely not be backwards compatible. It would be confusing and annoying for a user that downloads and attempts to use an ES-DE theme set in another EmulationStation fork only to get crashes, error messages or corrupted graphics. At least the -DE extension is a visual indicator that it's an ES-DE specific theme set.

Table of contents:

[[_TOC_]]

## Introduction

ES-DE allows the grouping of themes for multiple game systems into a **theme set**. Each theme is a collection of **views** that define some **elements**, each with their own **properties**.

Every game system has its own subdirectory within the theme set directory structure, and these are defined in the systems configuration file `es_systems.xml` either via the optional `<theme>` tag, or otherwise via the mandatory `<name>` tag. When ES-DE populates a system on startup it will look for a file named `theme.xml` in each such directory.

By placing a theme.xml file directly in the root of the theme set directory, that file will be processed as a default if there is no game-specific theme.xml file available.

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

## Simple example

Here is a very simple theme that changes the color of the game description text:

```xml
<theme>
    <formatVersion>7</formatVersion>
    <view name="detailed">
        <text name="description">
            <color>00FF00</color>
        </text>
        <image name="frame" extra="true">
            <pos>0.5 0.5</pos>
            <origin>0.5 0.5</origin>
            <size>0.8 0.8</size>
            <path>./core/frame.png</path>
        </image>
    </view>
</theme>
```

## How it works

All configuration must be contained within a `<theme>` tag pair.

The `<formatVersion>` tag **must** be specified. This is the version of the theming system the theme was designed for.
The current version is 7.

A _view_ can be thought of as a particular "screen" within ES-DE. Views are defined like this:

```xml
<view name="ViewNameHere">
    ... define elements here ...
</view>
```

An *element* is a particular visual element, such as an image or a piece of text.  You can modify an element that already exists for a particular view, as was done for the "description" example:

```xml
<elementTypeHere name="ExistingElementNameHere">
    ... define properties here ...
</elementTypeHere>
```

Or you can create your own elements by adding `extra="true"`, as was done for the "frame" example:

```xml
<elementTypeHere name="YourUniqueElementNameHere" extra="true">
    ... define properties here ...
</elementTypeHere>
```

"Extra" elements will be drawn in the order they are defined (so make sure to define backgrounds first). In what order they get drawn relative to the pre-existing elements depends on the view. Make sure "extra" element names do not clash with existing element names. An easy way to protect against this is to start all your extra element names with a prefix such as "e_".



*Properties* control how a particular *element* looks - for example its position, size, image path etc.  The type of the property determines what kinds of values you can use.  You can read about the types below in the "Reference" section.  Properties are defined like this:

```xml
<propertyNameHere>ValueHere</propertyNameHere>
```


## Advanced features

If you are writing a theme it's recommended to launch ES-DE with the `--debug` flag from a terminal window. If on Unix you can also pass the `--windowed` and `--resolution` flags to avoid having the application window fill the entire screen. On macOS and Windows you only need to pass the `--resolution` flag to accomplish this. By doing so, you can read error messages directly in the terminal window without having to open the log file.  You can also reload the current gamelist view and system view with `Ctrl-R` if the `--debug` flag has been set.

### The \<include\> tag

You can include theme files within theme files, for example:

`~/.emulationstation/themes/mythemeset-DE/fonts.xml`:
```xml
<theme>
    <formatVersion>7</formatVersion>
    <view name="detailed">
        <text name="description">
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
    <formatVersion>7</formatVersion>
    <include>./../fonts.xml</include>
    <view name="detailed">
        <text name="description">
            <color>FF0000</color>
        </text>
    </view>
</theme>
```

The above is equivalent to the following:
```xml

<theme>
    <formatVersion>7</formatVersion>
    <view name="detailed">
        <text name="description">
            <fontPath>./core/font.ttf</fontPath>
            <fontSize>0.035</fontSize>
            <color>FF0000</color>
        </text>
    </view>
</theme>
```

Note that properties can get merged. In the above example the `<color>` tag from `fonts.xml` got overwritten by the equivalent tag in `snes/theme.xml`. This happens because that tag was effectively declared after the first `<color>` tag. Be aware that the included file also needs the `<formatVersion>` tag.


### Theming multiple views simultaneously

Sometimes you want to apply the same properties to the same elements across multiple views.  The `name` attribute actually works as a list (delimited by any characters of `\t\r\n ,` - that is, whitespace and commas).  So for example, to apply the same logo to the basic and detailed views you could write the following:

```xml
<theme>
    <formatVersion>7</formatVersion>
    <view name="basic, detailed">
        <image name="logo">
            <path>./snes/logo.svg</path>
        </image>
    </view>
    <view name="video">
        <image name="logo">
            <path>./snes/logo_video.svg</path>
        </image>
    </view>
</theme>
```

The above is equivalent to:

```xml
<theme>
    <formatVersion>7</formatVersion>
    <view name="basic">
        <image name="logo">
            <path>./snes/logo.svg</path>
        </image>
    </view>
    <view name="detailed">
        <image name="logo">
            <path>./snes/logo.svg</path>
        </image>
    </view>
    <view name="video">
        <image name="logo">
            <path>./snes/logo_video.svg</path>
        </image>
    </view>
</theme>
```


### Theming multiple elements simultaneously

You can theme multiple elements *of the same type* simultaneously.  The `name` attribute actually works as a list (delimited by any characters of `\t\r\n ,` - that is, whitespace and commas). This is useful if you want to, say, apply the same color to all the metadata labels:

```xml
<theme>
    <formatVersion>7</formatVersion>
    <view name="detailed">
        <!-- Weird spaces/newline on purpose -->
        <text name="md_lbl_rating, md_lbl_releasedate, md_lbl_developer, md_lbl_publisher,
        md_lbl_genre,    md_lbl_players,        md_lbl_lastplayed, md_lbl_playcount">
            <color>48474D</color>
        </text>
    </view>
</theme>
```

Which is equivalent to:
```xml
<theme>
    <formatVersion>7</formatVersion>
    <view name="detailed">
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

Navigation sounds are configured globally per theme set, so it needs to be defined as a feature and with the view set to the special "all" category.
It's recommended to put these elements in a separate file and include it from the main theme file (e.g. `<include>./navigationsounds.xml</include>`).
There are seven different navigation sounds that can be configured. The names as well as the element structure should be self-explanatory based
on the example below.
Starting ES-DE with the --debug flag will provide feedback on whether any navigation sound elements were read from the theme set. If no navigation sounds are provided by the theme, ES-DE will use the bundled navigation sounds as a fallback. This is done per sound file, so the theme could provide for example one or two custom sounds while using the bundled ES-DE sounds for the rest.

Example debug output:
```
Jul 12 11:28:58 Debug:  NavigationSounds::loadThemeNavigationSounds(): Theme set includes navigation sound support, loading custom sounds
Jul 12 11:28:58 Debug:  Sound::getFromTheme(): Looking for tag <sound name="systembrowse">
Jul 12 11:28:58 Debug:  Sound::getFromTheme(): Tag found, ready to load theme sound file
Jul 12 11:28:58 Debug:  Sound::getFromTheme(): Looking for tag <sound name="quicksysselect">
Jul 12 11:28:58 Debug:  Sound::getFromTheme(): Tag not found, using fallback sound file
```

Example `navigationsounds.xml`, to be included from the main theme file:

```xml
<theme>
    <formatVersion>7</formatVersion>
    <feature supported="navigationsounds">
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
    </feature>
</theme>
```

### Element rendering order using zIndex

You can change the order in which elements are rendered by setting their `zIndex` values. All elements have a default value so you only need to define it for the ones you wish to explicitly change. Elements will be rendered in order from smallest to largest values.

Below are the default zIndex values per element type:

#### system
* Extra Elements `extra="true"` - 10
* `carousel name="systemcarousel"` - 40
* `text name="systemInfo"` - 50

#### basic, detailed, video, grid
* `image name="background"` - 0
* Extra Elements `extra="true"` - 10
* `textlist name="gamelist"` - 20
* `imagegrid name="gamegrid"` - 20
* Media
    * `image name="md_image"` - 30
    * `video name="md_video"` - 30
    * `image name="md_marquee"` - 35
* Metadata - 40
    * Labels
        * `text name="md_lbl_rating"`
        * `text name="md_lbl_releasedate"`
        * `text name="md_lbl_developer"`
        * `text name="md_lbl_publisher"`
        * `text name="md_lbl_genre"`
        * `text name="md_lbl_players"`
        * `text name="md_lbl_lastplayed"`
        * `text name="md_lbl_playcount"`
    * Values
        * `rating name="md_rating"`
        * `datetime name="md_releasedate"`
        * `text name="md_developer"`
        * `text name="md_publisher"`
        * `text name="md_genre"`
        * `text name="md_players"`
        * `datetime name="md_lastplayed"`
        * `text name="md_playcount"`
        * `text name="md_description"`
        * `text name="md_name"`
* System Logo/Text - 50
    * `image name="logo"`
    * `text name="logoText"`
    * `image name="logoPlaceholderImage"`
    * `text name="logoPlaceholderText"`
* Gamelist information - 50
    * `text name="gamelistInfo"`

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
    <themeColor>8b0000</themeColor>
</variables>
```

#### Usage in themes
Variables can be used to specify the value of a theme property:
```
<color>${themeColor}</color>
```

or to specify only a portion of the value of a theme property:

```
<color>${themeColor}c0</color>
<path>./art/logo/${system.theme}.svg</path>
````


## Reference

### Views, their elements, and themeable properties

#### system

* `helpsystem name="help"` - ALL
    - The help system style for this view.
* `carousel name="systemcarousel"` - ALL
    - The system logo carousel
* `image name="logo"` - PATH | COLOR
    - A logo image, to be displayed in the system logo carousel.
* `image name="logoPlaceholderImage"` - ALL
    - A logo image, to be displayed system name in the system logo carousel when no logo is available. Set the position
      to `0.5 0.5` to center the image.
* `text name="logoPlaceholderText"` - ALL
    - Logo text, to be displayed system name in the system logo carousel when no logo is available. The logo text is
      displayed on top of `logoPlaceholderImage`. Set the position to `0.5 0.5` to center the text.
* `text name="logoText"` - FONT_PATH | COLOR | FORCE_UPPERCASE | LINE_SPACING | TEXT
    - **Deprecated:** A logo text, to be displayed system name in the system logo carousel when no logo is available.
      Ignored when `logoPlaceholderImage` or `logoPlaceholderText` are set.
* `text name="systemInfo"` - ALL
    - Displays details of the system currently selected in the carousel.
* You can use extra elements (elements with `extra="true"`) to add your own backgrounds, etc. They will be displayed
  behind the carousel, and scroll relative to the carousel.


#### basic
* `helpsystem name="help"` - ALL
    - The help system style for this view.
* `image name="background"` - ALL
    - This is a background image that exists for convenience. It goes from (0, 0) to (1, 1).
* `text name="logoText"` - ALL
    - Displays the name of the system.  Only present if no "logo" image is specified.  Displayed at the top of the screen, centered by default.
* `image name="logo"` - ALL
    - A header image.  If a non-empty `path` is specified, `text name="logoText"` will be hidden and this image will be, by default, displayed roughly in its place.
* `textlist name="gamelist"` - ALL
    - The gamelist.  `primaryColor` is for games, `secondaryColor` is for folders.  Centered by default.


#### detailed
* `helpsystem name="help"` - ALL
    - The help system style for this view.
* `image name="background"` - ALL
    - This is a background image that exists for convenience. It goes from (0, 0) to (1, 1).
* `text name="logoText"` - ALL
    - Displays the name of the system.  Only present if no "logo" image is specified.  Displayed at the top of the screen, centered by default.
* `image name="logo"` - ALL
    - A header image.  If a non-empty `path` is specified, `text name="logoText"` will be hidden and this image will be, by default, displayed roughly in its place.
* `textlist name="gamelist"` - ALL
    - The gamelist.  `primaryColor` is for games, `secondaryColor` is for folders.  Left aligned by default.
* `text name="gamelistInfo"` - ALL
    - Displays the game count (all games as well as favorites), any applied filters, and a folder icon if a folder has been entered. If this text is left aligned, the folder icon will be placed to the right of the other information, and if it's right aligned, the folder icon will be placed to the left. Left aligned by default.

- Metadata
    - Labels
        * `text name="md_lbl_rating"` - ALL
        * `text name="md_lbl_releasedate"` - ALL
        * `text name="md_lbl_developer"` - ALL
        * `text name="md_lbl_publisher"` - ALL
        * `text name="md_lbl_genre"` - ALL
        * `text name="md_lbl_players"` - ALL
        * `text name="md_lbl_lastplayed"` - ALL
        * `text name="md_lbl_playcount"` - ALL

    * Values
        - _All values will follow to the right of their labels if a position isn't specified._
        * `image name="md_image"` - POSITION | SIZE | Z_INDEX
            - Path is the "image" metadata for the currently selected game.
        * `rating name="md_rating"` - ALL
            - The "rating" metadata.
        * `datetime name="md_releasedate"` - ALL
            - The "releasedate" metadata.
        * `text name="md_developer"` - ALL
            - The "developer" metadata.
        * `text name="md_publisher"` - ALL
            - The "publisher" metadata.
        * `text name="md_genre"` - ALL
            - The "genre" metadata.
        * `text name="md_players"` - ALL
            - The "players" metadata (number of players the game supports).
        * `datetime name="md_lastplayed"` - ALL
            - The "lastplayed" metadata.  Displayed as a string representing the time relative to "now" (e.g. "3 hours ago").
        * `text name="md_playcount"` - ALL
            - The "playcount" metadata (number of times the game has been played).
        * `text name="md_description"` - POSITION | SIZE | FONT_PATH | FONT_SIZE | COLOR | Z_INDEX
            - Text is the "desc" metadata.  If no `pos`/`size` is specified, will move and resize to fit under the lowest label and reach to the bottom of the screen.
        * `text name="md_name"` - ALL
            - The "name" metadata (the game name). Unlike the others metadata fields, the name is positioned offscreen by default


#### video
* `helpsystem name="help"` - ALL
    - The help system style for this view.
* `image name="background"` - ALL
    - This is a background image that exists for convenience. It goes from (0, 0) to (1, 1).
* `text name="logoText"` - ALL
    - Displays the name of the system.  Only present if no "logo" image is specified.  Displayed at the top of the screen, centered by default.
* `image name="logo"` - ALL
    - A header image.  If a non-empty `path` is specified, `text name="logoText"` will be hidden and this image will be, by default, displayed roughly in its place.
* `textlist name="gamelist"` - ALL
    - The gamelist.  `primaryColor` is for games, `secondaryColor` is for folders.  Left aligned by default.
* `text name="gamelistInfo"` - ALL
    - Displays the game count (all games as well as favorites), any applied filters, and a folder icon if a folder has been entered. If this text is left aligned, the folder icon will be placed to the right of the other information, and if it's right aligned, the folder icon will be placed to the left. Left aligned by default.

- Metadata
    - Labels
        * `text name="md_lbl_rating"` - ALL
        * `text name="md_lbl_releasedate"` - ALL
        * `text name="md_lbl_developer"` - ALL
        * `text name="md_lbl_publisher"` - ALL
        * `text name="md_lbl_genre"` - ALL
        * `text name="md_lbl_players"` - ALL
        * `text name="md_lbl_lastplayed"` - ALL
        * `text name="md_lbl_playcount"` - ALL

    * Values
        - _All values will follow to the right of their labels if a position isn't specified._
        * `image name="md_image"` - POSITION | SIZE | Z_INDEX
            - Path is the "image" metadata for the currently selected game.
        * `image name="md_marquee"` - POSITION | SIZE | Z_INDEX
            - Path is the "marquee" metadata for the currently selected game.
        * `video name="md_video"` - POSITION | SIZE | Z_INDEX
            - Path is the "video" metadata for the currently selected game.
        * `rating name="md_rating"` - ALL
            - The "rating" metadata.
        * `datetime name="md_releasedate"` - ALL
            - The "releasedate" metadata.
        * `text name="md_developer"` - ALL
            - The "developer" metadata.
        * `text name="md_publisher"` - ALL
            - The "publisher" metadata.
        * `text name="md_genre"` - ALL
            - The "genre" metadata.
        * `text name="md_players"` - ALL
            - The "players" metadata (number of players the game supports).
        * `datetime name="md_lastplayed"` - ALL
            - The "lastplayed" metadata.  Displayed as a string representing the time relative to "now" (e.g. "3 hours ago").
        * `text name="md_playcount"` - ALL
            - The "playcount" metadata (number of times the game has been played).
        * `text name="md_description"` - POSITION | SIZE | FONT_PATH | FONT_SIZE | COLOR | Z_INDEX
            - Text is the "desc" metadata.  If no `pos`/`size` is specified, will move and resize to fit under the lowest label and reach to the bottom of the screen.
        * `text name="md_name"` - ALL
            - The "name" metadata (the game name). Unlike the others metadata fields, the name is positioned offscreen by default


#### grid
* `helpsystem name="help"` - ALL
    - The help system style for this view.
* `image name="background"` - ALL
    - This is a background image that exists for convenience. It goes from (0, 0) to (1, 1).
* `text name="logoText"` - ALL
    - Displays the name of the system.  Only present if no "logo" image is specified.  Displayed at the top of the screen, centered by default.
* `image name="logo"` - ALL
    - A header image.  If a non-empty `path` is specified, `text name="logoText"` will be hidden and this image will be, by default, displayed roughly in its place.
* `imagegrid name="gamegrid"` - ALL
    - The gamegrid. The number of tile displayed is controlled by its size, margin and the default tile max size.
* `gridtile name="default"` - ALL
    - Note that many of the default gridtile parameters change the selected gridtile parameters if they are not explicitly set by the theme. For example, changing the background image of the default gridtile also change the background image of the selected gridtile. Refer to the gridtile documentation for more informations.
* `gridtile name="selected"` - ALL
    - See default gridtile description right above.
* `text name="gamelistInfo"` - ALL
    - Displays the game count (all games as well as favorites), any applied filters, and a folder icon if a folder has been entered. If this text is left aligned, the folder icon will be placed to the right of the other information, and if it's right aligned, the folder icon will be placed to the left. Left aligned by default.

- Metadata
    - Labels
        * `text name="md_lbl_rating"` - ALL
        * `text name="md_lbl_releasedate"` - ALL
        * `text name="md_lbl_developer"` - ALL
        * `text name="md_lbl_publisher"` - ALL
        * `text name="md_lbl_genre"` - ALL
        * `text name="md_lbl_players"` - ALL
        * `text name="md_lbl_lastplayed"` - ALL
        * `text name="md_lbl_playcount"` - ALL

    * Values
        - _All values will follow to the right of their labels if a position isn't specified._
        * `rating name="md_rating"` - ALL
            - The "rating" metadata.
        * `datetime name="md_releasedate"` - ALL
            - The "releasedate" metadata.
        * `text name="md_developer"` - ALL
            - The "developer" metadata.
        * `text name="md_publisher"` - ALL
            - The "publisher" metadata.
        * `text name="md_genre"` - ALL
            - The "genre" metadata.
        * `text name="md_players"` - ALL
            - The "players" metadata (number of players the game supports).
        * `datetime name="md_lastplayed"` - ALL
            - The "lastplayed" metadata.  Displayed as a string representing the time relative to "now" (e.g. "3 hours ago").
        * `text name="md_playcount"` - ALL
            - The "playcount" metadata (number of times the game has been played).
        * `text name="md_description"` - POSITION | SIZE | FONT_PATH | FONT_SIZE | COLOR | Z_INDEX
            - Text is the "desc" metadata.  If no `pos`/`size` is specified, will move and resize to fit under the lowest label and reach to the bottom of the screen.
        * `text name="md_name"` - ALL
            - The "name" metadata (the game name). Unlike the others metadata fields, the name is positioned offscreen by default


### Types of properties

* NORMALIZED_PAIR - two decimals, in the range [0..1], delimited by a space.  For example, `0.25 0.5`.  Most commonly used for position (x and y coordinates) and size (width and height).
* NORMALIZED_RECT - four decimals, in the range [0..1], delimited by a space. For example, `0.25 0.5 0.10 0.30`.  Most commonly used for padding to store top, left, bottom and right coordinates.
* PATH - a path.  If the first character is a `~`, it will be expanded into the environment variable for the home path (`$HOME` for Linux or `%HOMEPATH%` for Windows) unless overridden using the --home command line option.  If the first character is a `.`, it will be expanded to the theme file's directory, allowing you to specify resources relative to the theme file, like so: `./../general_art/myfont.ttf`.
* BOOLEAN - `true`/`1` or `false`/`0`.
* COLOR - a hexidecimal RGB or RGBA color (6 or 8 digits).  If 6 digits, will assume the alpha channel is `FF` (not transparent).
* FLOAT - a decimal.
* STRING - a string of text.


### Types of elements and their properties

Common to almost all elements is a `pos` and `size` property of the NORMALIZED_PAIR type.  They are normalized in terms of their "parent" object's size; 99% of the time, this is just the size of the screen.  In this case, `<pos>0 0</pos>` would correspond to the top left corner, and `<pos>1 1</pos>` the bottom right corner (a positive Y value points further down).  `pos` almost always refers to the top left corner of your element.  You *can* use numbers outside of the [0..1] range if you want to place an element partially or completely off-screen.

The order you define properties in does not matter.
Remember, you do *not* need to specify every property!
*Note that a view may choose to only make only certain properties on a particular element themeable!*

#### image

Can be created as an extra.

* `pos` - type: NORMALIZED_PAIR.
* `size` - type: NORMALIZED_PAIR.
    - If only one axis is specified (and the other is zero), the other will be automatically calculated in accordance with the image's aspect ratio.
* `maxSize` - type: NORMALIZED_PAIR.
    - The image will be resized as large as possible so that it fits within this size and maintains its aspect ratio.  Use this instead of `size` when you don't know what kind of image you're using so it doesn't get grossly oversized on one axis (e.g. with a game's image metadata).
* `origin` - type: NORMALIZED_PAIR.
    - Where on the image `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the image exactly in the middle of the screen.  If the "POSITION" and "SIZE" attributes are themeable, "ORIGIN" is implied.
* `rotation` - type: FLOAT.
    - angle in degrees that the image should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
* `rotationOrigin` - type: NORMALIZED_PAIR.
    - Point around which the image will be rotated. Defaults to `0.5 0.5`.
* `path` - type: PATH.
    - Path to the image file.  Most common extensions are supported (including .jpg, .png, and unanimated .gif).
* `default` - type: PATH.
    - Path to default image file.  Default image will be displayed when selected game does not have an image.
* `tile` - type: BOOLEAN.
    - If true, the image will be tiled instead of stretched to fit its size.  Useful for backgrounds.
* `color` - type: COLOR.
    - Multiply each pixel's color by this color. For example, an all-white image with `<color>FF0000</color>` would become completely red.  You can also control the transparency of an image with `<color>FFFFFFAA</color>` - keeping all the pixels their normal color and only affecting the alpha channel.
* `visible` - type: BOOLEAN.
    - If true, component will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view.
* `zIndex` - type: FLOAT.
    - z-index value for component.  Components will be rendered in order of z-index value from low to high.

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
    - z-index value for component.  Components will be rendered in order of z-index value from low to high.

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

#### video

* `pos` - type: NORMALIZED_PAIR.
* `size` - type: NORMALIZED_PAIR.
    - If only one axis is specified (and the other is zero), the other will be automatically calculated in accordance with the video's aspect ratio.
* `maxSize` - type: NORMALIZED_PAIR.
    - The video will be resized as large as possible so that it fits within this size and maintains its aspect ratio.  Use this instead of `size` when you don't know what kind of video you're using so it doesn't get grossly oversized on one axis (e.g. with a game's video metadata).
* `origin` - type: NORMALIZED_PAIR.
    - Where on the image `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the image exactly in the middle of the screen.  If the "POSITION" and "SIZE" attributes are themeable, "ORIGIN" is implied.
* `rotation` - type: FLOAT.
    - angle in degrees that the text should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
* `rotationOrigin` - type: NORMALIZED_PAIR.
    - Point around which the text will be rotated. Defaults to `0.5 0.5`.
* `delay` - type: FLOAT.  Default is false.
    - Delay in seconds before video will start playing.
* `default` - type: PATH.
    - Path to default video file.  Default video will be played when selected game does not have a video.
* `showSnapshotNoVideo` - type: BOOLEAN
    - If true, image will be shown when selected game does not have a video and no `default` video is configured.
* `showSnapshotDelay` - type: BOOLEAN
    - If true, playing of video will be delayed for `delayed` seconds, when game is selected.
* `visible` - type: BOOLEAN.
    - If true, component will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view.
* `zIndex` - type: FLOAT.
    - z-index value for component.  Components will be rendered in order of z-index value from low to high.

#### text

Can be created as an extra.

* `pos` - type: NORMALIZED_PAIR.
* `size` - type: NORMALIZED_PAIR.
    - Possible combinations:
    - `0 0` - automatically size so text fits on one line (expanding horizontally).
    - `w 0` - automatically wrap text so it doesn't go beyond `w` (expanding vertically).
    - `w h` - works like a "text box."  If `h` is non-zero and `h` <= `fontSize` (implying it should be a single line of text), text that goes beyond `w` will be truncated with an elipses (...).
* `origin` - type: NORMALIZED_PAIR.
    - Where on the component `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the component exactly in the middle of the screen.  If the "POSITION" and "SIZE" attributes are themeable, "ORIGIN" is implied.
* `rotation` - type: FLOAT.
    - angle in degrees that the text should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
* `rotationOrigin` - type: NORMALIZED_PAIR.
    - Point around which the text will be rotated. Defaults to `0.5 0.5`.
* `text` - type: STRING.
* `color` - type: COLOR.
* `backgroundColor` - type: COLOR;
* `fontPath` - type: PATH.
    - Path to a truetype font (.ttf).
* `fontSize` - type: FLOAT.
    - Size of the font as a percentage of screen height (e.g. for a value of `0.1`, the text's height would be 10% of the screen height).
* `alignment` - type: STRING.
    - Valid values are "left", "center", or "right".  Controls alignment on the X axis.  "center" will also align vertically.
* `forceUppercase` - type: BOOLEAN.  Draw text in uppercase.
* `lineSpacing` - type: FLOAT.  Controls the space between lines (as a multiple of font height).  Default is 1.5.
* `visible` - type: BOOLEAN.
    - If true, component will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view.
* `zIndex` - type: FLOAT.
    - z-index value for component.  Components will be rendered in order of z-index value from low to high.

#### textlist

* `pos` - type: NORMALIZED_PAIR.
* `size` - type: NORMALIZED_PAIR.
* `origin` - type: NORMALIZED_PAIR.
    - Where on the component `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the component exactly in the middle of the screen.  If the "POSITION" and "SIZE" attributes are themeable, "ORIGIN" is implied.
* `selectorColor` - type: COLOR.
    - Color of the "selector bar."
* `selectorImagePath` - type: PATH.
    - Path to image to render in place of "selector bar."
* `selectorImageTile` - type: BOOLEAN.
    - If true, the selector image will be tiled instead of stretched to fit its size.
* `selectorHeight` - type: FLOAT.
    - Height of the "selector bar".
* `selectorOffsetY` - type: FLOAT.
    - Allows moving of the "selector bar" up or down from its computed position.  Useful for fine tuning the position of the "selector bar" relative to the text.
* `selectedColor` - type: COLOR.
    - Color of the highlighted entry text.
* `primaryColor` - type: COLOR.
    - Primary color; what this means depends on the text list.  For example, for game lists, it is the color of a game.
* `secondaryColor` - type: COLOR.
    - Secondary color; what this means depends on the text list.  For example, for game lists, it is the color of a folder.
* `fontPath` - type: PATH.
* `fontSize` - type: FLOAT.
* `alignment` - type: STRING.
    - Valid values are "left", "center", or "right".  Controls alignment on the X axis.
* `horizontalMargin` - type: FLOAT.
    - Horizontal offset for text from the alignment point.  If `alignment` is "left", offsets the text to the right.  If `alignment` is "right", offsets text to the left.  No effect if `alignment` is "center".  Given as a percentage of the element's parent's width (same unit as `size`'s X value).
* `forceUppercase` - type: BOOLEAN.  Draw text in uppercase.
* `lineSpacing` - type: FLOAT.  Controls the space between lines (as a multiple of font height).  Default is 1.5.
* `zIndex` - type: FLOAT.
    - z-index value for component.  Components will be rendered in order of z-index value from low to high.

#### ninepatch

* `pos` - type: NORMALIZED_PAIR.
* `size` - type: NORMALIZED_PAIR.
* `path` - type: PATH.
* `visible` - type: BOOLEAN.
    - If true, component will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view.
* `zIndex` - type: FLOAT.
    - z-index value for component.  Components will be rendered in order of z-index value from low to high.

ES-DE borrows the concept of "nine patches" from Android (or "9-Slices"). Currently the implementation is very simple and hard-coded to only use 48x48px images (16x16px for each "patch"). Check the `data/resources` directory for some examples (button.png, frame.png).

#### rating

* `pos` - type: NORMALIZED_PAIR.
* `size` - type: NORMALIZED_PAIR.
    - Only one value is actually used. The other value should be zero.  (e.g. specify width OR height, but not both.  This is done to maintain the aspect ratio.)
* `origin` - type: NORMALIZED_PAIR.
    - Where on the component `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the component exactly in the middle of the screen.  If the "POSITION" and "SIZE" attributes are themeable, "ORIGIN" is implied.
* `rotation` - type: FLOAT.
    - angle in degrees that the rating should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
* `rotationOrigin` - type: NORMALIZED_PAIR.
    - Point around which the rating will be rotated. Defaults to `0.5 0.5`.
* `filledPath` - type: PATH.
    - Path to the "filled star" image.  Image must be square (width equals height).
* `unfilledPath` - type: PATH.
    - Path to the "unfilled star" image.  Image must be square (width equals height).
* `color` - type: COLOR.
    - Multiply each pixel's color by this color. For example, an all-white image with `<color>FF0000</color>` would become completely red.  You can also control the transparency of an image with `<color>FFFFFFAA</color>` - keeping all the pixels their normal color and only affecting the alpha channel.
* `visible` - type: BOOLEAN.
    - If true, component will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view.
* `zIndex` - type: FLOAT.
    - z-index value for component.  Components will be rendered in order of z-index value from low to high.

#### datetime
* `pos` - type: NORMALIZED_PAIR.
* `size` - type: NORMALIZED_PAIR.
    - Possible combinations:
    - `0 0` - automatically size so text fits on one line (expanding horizontally).
    - `w 0` - automatically wrap text so it doesn't go beyond `w` (expanding vertically).
    - `w h` - works like a "text box."  If `h` is non-zero and `h` <= `fontSize` (implying it should be a single line of text), text that goes beyond `w` will be truncated with an elipses (...).
* `origin` - type: NORMALIZED_PAIR.
    - Where on the component `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the component exactly in the middle of the screen.  If the "POSITION" and "SIZE" attributes are themeable, "ORIGIN" is implied.
* `rotation` - type: FLOAT.
    - angle in degrees that the text should be rotated.  Positive values will rotate clockwise, negative values will rotate counterclockwise.
* `rotationOrigin` - type: NORMALIZED_PAIR.
    - Point around which the text will be rotated. Defaults to `0.5 0.5`.
* `color` - type: COLOR.
* `backgroundColor` - type: COLOR;
* `fontPath` - type: PATH.
    - Path to a truetype font (.ttf).
* `fontSize` - type: FLOAT.
    - Size of the font as a percentage of screen height (e.g. for a value of `0.1`, the text's height would be 10% of the screen height).
* `alignment` - type: STRING.
    - Valid values are "left", "center", or "right".  Controls alignment on the X axis.  "center" will also align vertically.
* `forceUppercase` - type: BOOLEAN.  Draw text in uppercase.
* `lineSpacing` - type: FLOAT.  Controls the space between lines (as a multiple of font height).  Default is 1.5.
* `visible` - type: BOOLEAN.
    - If true, component will be rendered, otherwise rendering will be skipped.  Can be used to hide elements from a particular view.
* `zIndex` - type: FLOAT.
    - z-index value for component.  Components will be rendered in order of z-index value from low to high.
* `displayRelative` - type: BOOLEAN.  Renders the datetime as a a relative string (ex: 'x days ago')
* `format` - type: STRING. Specifies format for rendering datetime.
    - %Y: The year, including the century (1900)
    - %m: The month number [01,12]
    - %d: The day of the month [01,31]
    - %H: The hour (24-hour clock) [00,23]
    - %M: The minute [00,59]
    - %S: The second [00,59]

#### helpsystem

* `pos` - type: NORMALIZED_PAIR. Default is "0.012 0.9515"
* `origin` - type: NORMALIZED_PAIR.
    - Where on the component `pos` refers to. For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place
      the component exactly in the middle of the screen.
* `textColor` - type: COLOR. Default is 777777FF.
* `textColorDimmed` - type: COLOR. Default is the same value as textColor. Must be placed under the 'system' view.
* `iconColor` - type: COLOR. Default is 777777FF.
* `iconColorDimmed` - type: COLOR. Default is the same value as iconColor. Must be placed under the 'system' view.
* `fontPath` - type: PATH.
* `fontSize` - type: FLOAT.
* `entrySpacing` - type: FLOAT. Default is 16.0.
    - Spacing in pixels between the help system components.
* `iconTextSpacing` - type: FLOAT. Default is 8.0.
    - Spacing in pixels within a help system component between it's icon and text.
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

#### carousel

* `type` - type: STRING.
    - Sets the scoll direction of the carousel.
    - Accepted values are "horizontal", "vertical", "horizontal_wheel" or "vertical_wheel".
    - Default is "horizontal".
* `size` - type: NORMALIZED_PAIR. Default is "1 0.2325"
* `pos` - type: NORMALIZED_PAIR.  Default is "0 0.38375".
* `origin` - type: NORMALIZED_PAIR.
    - Where on the carousel `pos` refers to.  For example, an origin of `0.5 0.5` and a `pos` of `0.5 0.5` would place the carousel exactly in the middle of the screen.  If the "POSITION" and "SIZE" attributes are themeable, "ORIGIN" is implied.
* `color` - type: COLOR.
    - Controls the color of the carousel background.
    - Default is FFFFFFD8
* `logoSize` - type: NORMALIZED_PAIR.  Default is "0.25 0.155"
* `logoScale` - type: FLOAT.
    - Selected logo is increased in size by this scale
    - Default is 1.2
* `logoRotation` - type: FLOAT.
    - Angle in degrees that the logos should be rotated.  Value should be positive.
    - Default is 7.5
    - This property only applies when `type` is "horizontal_wheel" or "vertical_wheel".
* `logoRotationOrigin` - type: NORMALIZED_PAIR.
    - Point around which the logos will be rotated. Defaults to `-5 0.5`.
    - This property only applies when `type` is "horizontal_wheel" or "vertical_wheel".
* `logoAlignment` - type: STRING.
    - Sets the alignment of the logos relative to the carousel.
    - Accepted values are "top", "bottom" or "center" when `type` is "horizontal" or "horizontal_wheel".
    - Accepted values are "left", "right" or "center" when `type` is "vertical" or "vertical_wheel".
    - Default is "center"
* `maxLogoCount` - type: FLOAT.
    - Sets the number of logos to display in the carousel.
    - Default is 3
* `zIndex` - type: FLOAT.
    - z-index value for component.  Components will be rendered in order of z-index value from low to high.

The help system is a special element that displays a context-sensitive list of actions the user can take at any time.  You should try and keep the position constant throughout every screen.  Keep in mind the "default" settings (including position) are used whenever the user opens a menu.


## Example theme sets

To see some example EmulationStation themes, the following resources are recommended:

https://aloshi.com/emulationstation#themes

https://github.com/RetroPie

https://gitlab.com/recalbox/recalbox-themes

https://wiki.batocera.org/themes
