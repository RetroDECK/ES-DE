# EmulationStation Desktop Edition (ES-DE) - Contributing

### Help needed

If you are experienced with developing cross-platform client applications in C++ then ES-DE could be an interesting project for you! Although the application does not have a huge codebase, it's fairly complex as it covers many different areas. To work on the project you need to be able to test your code on Linux, macOS and Windows.

Merge requests are only accepted from project members so if you would like to contribute to ES-DE then please get in touch and we can discuss what you would like to work on. But please only consider joining if you intend to be contributing long term as the project is quite large in scope and to train someone to be a team member is a substantial time investment.

Development is tracked using a Kanban board which is publicly visible at the project's GitLab site:

[https://gitlab.com/es-de/emulationstation-de/-/boards](https://gitlab.com/es-de/emulationstation-de/-/boards)

Development takes place in the master branch, and bug fixes/point releases are handled in the stable branches (only the latest stable version is maintained).

### Coding style

Code formatting is applied automatically using clang-format, so to understand the exact formatting rules refer to the .clang-format file in the root of the ES-DE repository. You can read in [INSTALL.md](INSTALL.md#using-clang-format-for-automatic-code-formatting) how clang-format is installed and used.

But as clang-format won't change actual code content or fix all code style choices, here are some additional key points:

* Always write comments in C++ style, i.e. `//` instead of `/* */`
* Comments should be proper sentences, starting with a capital letter and ending with a dot
* Use C++ and not C, for example `static_cast<int>(someFloatVariable)` instead of `(int)someFloatVariable`
* Always declare one variable per line, never combine multiple declarations of the same type
* Name member variables starting with an `m` such as `mMyMemberVariable` and name static variables starting with an `s` such as `sMyStaticVariable`
* Use braced initializations when possible, e.g. `float myFloat {1.5f}` as this is the safest way to do it
* Short function definitions can be placed in either the .h or .cpp file depending on situation and context
* Try to be coherent with the existing codebase regarding names, structure etc., it should not be obvious that different persons wrote different sections of the code
* For the rest, check the code and have fun :)
