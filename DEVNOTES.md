Coding Style
============

The coding style for EmulationStation-DE is mostly a combination of the Linux Kernel and Google C++ coding guidelines. \
Please refer to these documents here: \
https://www.kernel.org/doc/html/v4.10/process/coding-style.html \
https://google.github.io/styleguide/cppguide.html \
There are some deviations though, mostly due to historical reasons as the original code did not use this coding style.

Some key points:

* Column width (line length) is 100 characters
* Indentation is 4 spaces, don't use tabs as they can be interpreted differently!
* Line break is Unix-style (line feed only, no carriage return)
* Do not leave trailing whitespaces at the end of the lines (a good source code editor should have a setting to automatically trim these for you)
* When breaking up long lines into multiple lines, consider what could be useful data to grep for so you don't break in the middle of such a string
* Comments always in C++ style, i.e. // instead of /* */
* Comments should be proper sentences, starting with a capital letter and ending with a dot
* Use K&R placements of braces, read the Linux Kernel coding style document for clarifications
* Always use spaces between keywords and opening brackets, i.e. `if ()`, `for ()`, `while ()` etc.
* Use `std::string` instead of `char *` or `char []` unless there is a very specific reason requiring the latter
* If the arguments (and initializer list) for a function or class exceeds 4 items, arrange them vertically to make the code easier to read
* Always declare one variable per line, never combine multiple declarations of the same type
* Name local variables with the first word in small letters and the proceeding words starting with capital letters, e.g. myExampleVariable
* Name member variables starting with a small 'm', e.g. mMyMemberVariable
* Don't pad variable declarations with spaces to make them align in columns, I'm sure it's well intended but it looks terrible
* Use the same naming convention for functions as for local variables, e.g. someFunction()
* Inline functions can be used but don't overdo it by using them for functions that won't be called very frequently
* Never put more than one statement on a single line, except for lambda expressions
* Avoid overoptimizations, especially if it sacrifices readability, makes the code hard to expand on or is error prone
* For the rest, check the code and have fun! :)


Development Environment
=======================

EmulationStation-DE is developed and compiled using GCC and GDB. \
For debugging purposes, starting the application like this could make sense:

`emulationstation --windowed --debug --resolution 1280 720`


Creating a new GuiComponent
===========================

You probably want to override:

	`bool input(InputConfig* config, Input input);`
		Check if some input is mapped to some action with `config->isMappedTo("a", input);`.
		Check if an input is "pressed" with `input.value != 0` (input.value *can* be negative in the case of axes).

	`void update(int deltaTime);`
		`deltaTime` is in milliseconds.

	`void render(const Transform4x4f& parentTrans);`
		You probably want to do `Transform4x4f trans = parentTrans * getTransform();` to get your final "modelview" matrix.
		Apply the modelview matrix with `Renderer::setMatrix(const Transform4x4f&)`.
		Render any children the component may have with `renderChildren(parentTrans);`.


Creating a new GameListView Class
=================================

1. Don't allow the user to navigate to the root node's parent. If you use a stack of some sort to keep track of past cursor states this will be a natural side effect.


Creating a new Component
========================

If your component is not made up of other components, and you draw something to the screen with OpenGL, make sure:

* Your vertex positions are rounded before you render (you can use round(float) in Util.h to do this).
* Your transform matrix's translation is rounded (you can use roundMatrix(affine3f) in Util.h to do this).
