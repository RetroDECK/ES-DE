Gamelists
=========

The gamelist.xml file for a system defines metadata for a system's games, such as a name, description, release date, and rating.

ES only checks for the `gamelist.xml` files in the user's home directory:

`~/.emulationstation/gamelists/[SYSTEM_NAME]/gamelist.xml`

An example gamelist.xml:
```xml
<gameList>
	<game>
		<path>./mm2.nes</path>
		<name>Mega Man 2</name>
		<desc>Mega Man 2 is a classic NES game which follows Mega Man as he murders eight robot masters in cold blood.</desc>
	</game>
</gameList>
```

Everything is enclosed in a `<gameList>` tag.  The information for each game or folder is enclosed in a corresponding tag (`<game>` or `<folder>`).  Each piece of metadata is encoded as a string.

As of EmulationStation Desktop Edition v1.0.0, there are no longer any references to game media files in gamelist.xml. Instead a media directory is used where the images and videos are simply matched against the ROM file names. As well, no absolute paths are used for the ROM files any longer. Instead a global ROM directory is configured and there are only relative references in the gamelist.xml files, starting with `./` as can be seen in the example above.

Please refer to [INSTALL.md](INSTALL.md) for more information on how the ROM and media directories are configured.

Reference
=========

(if you suspect this section is out of date, check out `src/MetaData.cpp`)

There are a few types of metadata:

* `string` - just text.
* `float` - a floating-point decimal value (written as a string).
* `integer` - an integer value (written as a string).
* `datetime` - a date and, potentially, a time.  These are encoded as an ISO string, in the following format: "%Y%m%dT%H%M%S%F%q".  For example, the release date for Chrono Trigger is encoded as "19950311T000000" (no time specified).

Some metadata is also marked as "statistic" - these are kept track of by ES and do not show up in the metadata editor. They are shown in certain views (for example, the detailed view and the video view both show `lastplayed`, although the label can be disabled by the theme).

#### `<game>`

* `name` - string, the displayed name for the game.
* `desc` - string, a description of the game.  Longer descriptions will automatically scroll, so don't worry about size.
* `rating` - float, the rating for the game, expressed as a floating point number between 0 and 1. ES will round fractional values to half-stars.
* `releasedate` - datetime, the date the game was released.  Displayed as date only, time is ignored.
* `developer` - string, the developer for the game.
* `publisher` - string, the publisher for the game.
* `genre` - string, the (primary) genre for the game.
* `players` - integer, the number of players the game supports.
* `favorite` - bool, indicates whether the game is a favorite.
* `completed`- bool, indicates whether the game has been completed.
* `broken` - bool, indicates a game that doesn't work (useful for MAME).
* `kidgame` - bool, indicates whether the game is suitable for children, used by the `kid' UI mode.
* `playcount` - integer, the number of times this game has been played.
* `lastplayed` - statistic, datetime, the last date and time this game was played.
* `sortname` - string, used in sorting the gamelist in a system, instead of `name`.
* `launchstring` - optional tag that is used to override the emulator and core settings on a per-game basis.

#### `<folder>`
* `name` - string, the displayed name for the folder.
* `desc` - string, the description for the folder.
* `developer` - string, developer(s).
* `publisher` - string, publisher(s).
* `genre` - string, genre(s).
* `players` - integer, the number of players the game supports.


Things to be aware of
=====================

* You can use ES's built-in [scraping](http://en.wikipedia.org/wiki/Web_scraping) tools to avoid creating a gamelist.xml by hand, as described in README.md.

* If a value matches the default for a particular piece of metadata, ES will not write it to the gamelist.xml (for example, if `genre` isn't specified, ES won't write an empty genre tag).

* A `game` can actually point to a folder/directory if the folder has a matching extension.

* `folder` metadata will only be used if a game is found inside of that folder.

* ES will keep entries for games and folders that it can't find the files for.

* The switch `--gamelist-only` can be used to skip automatic searching, and only display games defined in the system's gamelist.xml.

* The switch `--ignore-gamelist` can be used to ignore the gamelist upon start of the application.
