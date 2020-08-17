Contributing to EmulationStation Desktop Edition
================================================


### Help needed:

There are many cards in the Kanban backlog that would be nice to implement. And for this, code contributions are very welcome. There may be a bug or two as well that needs fixing ;)

Apart from code commits, help is especially needed for thorough testing of the software and for working on the rbsimple-DE theme.

It's impossible for me to test every game system (rbsimple-DE has support for more than a 100 different systems!) so it would be especially useful to hear about any issues with starting games using the default es_systems.cfg configuration file and also if there are any problems with scraping for certain systems.

In general, a review of the [es_systems.cfg_unix](resources/templates/es_systems.cfg_unix) and [es_systems.cfg_windows](resources/templates/es_systems.cfg_windows) files including the supported file extensions would be great!

As for rbsimple-DE there are quite some missing graphic files and other customizations for a number of game systems. \
Check out [MISSING.md](themes/rbsimple-DE/MISSING.md) for more details of what needs to be added or updated.

### Coding style:

The coding style for EmulationStation-DE is mostly a combination of the Linux Kernel style (although that's C it's close enough to C++ as far as I'm concerned) and Google's C++ guidelines. Google is not a very nice company, but they write nice code.

Please refer to these documents here:

https://www.kernel.org/doc/html/v4.10/process/coding-style.html \
https://google.github.io/styleguide/cppguide.html

**Some key points:**

* Column width (line length) is 100 characters
* Indentation is 4 spaces, don't use tabs as they can be interpreted differently!
* Line break is Unix-style (line feed only, no carriage return)
* Do not leave trailing whitespaces at the end of the lines (a good source code editor should have a setting to automatically trim these for you)
* When breaking up long lines into multiple lines, consider what could be useful data to grep for so you don't break in the middle of such a string
* Comments always in C++ style, i.e. // instead of /* */
* Comments should be proper sentences, starting with a capital letter and ending with a dot
* Use K&R placements of braces, read the Linux Kernel coding style document for clarifications
* Always use spaces between keywords and opening brackets, i.e. `if ()`, `for ()`, `while ()` etc.
* Indentation of switch/case statements is optional, but it's usually easier to read the code with indentations in place
* Use `std::string` instead of `char *` or `char []` unless there is a specific reason requiring the latter
* If the arguments (and initializer list) for a function or class exceeds 4 items, arrange them vertically to make the code easier to read
* Always declare one variable per line, never combine multiple declarations of the same type
* Name local variables with the first word in small letters and the proceeding words starting with capital letters, e.g. myExampleVariable
* Name member variables starting with a small 'm', e.g. mMyMemberVariable
* Use the same naming convention for functions as for local variables, e.g. someFunction()
* Inline functions makes perfect sense to use, but don't overdo it by using them for functions that won't be called very frequently
* Never put more than one statement on a single line (there are some exceptions though like lambda expressions and some switch statements)
* Avoid overoptimizations, especially if it sacrifices readability, makes the code hard to expand on or is error prone
* For the rest, check the code and have fun! :)

### Building and configuring:

Please refer to the [INSTALL.md](INSTALL.md) file for details on everything related to building and configuring EmulationStation Desktop Edition.
