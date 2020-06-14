Coding Style Guide
==================

The coding style is mostly a combination the Linux Kernel and Google C++ coding styles.
Please refer to these guides here:
	https://www.kernel.org/doc/html/v4.10/process/coding-style.html
	https://google.github.io/styleguide/cppguide.html
There are some deviations though, mostly due to historical reasons as the original code did not use this coding style.

The most obvious and important points to consider:

* Column width (line length) is 100 characters
* Indentation is 4 spaces, don't use tabs as they can be interpreted differently!
* Comments always in C++ style, i.e. // instead of /* */
* Comments should be proper sentences, starting with a capital letter and ending with a dot
* K&R placements of braces, read the Linux Kernel coding style document for clarifications
* Always use spaces between keywords and opening brackets, i.e. 'if ()', 'for ()', 'while (' etc.
* Avoid excessive inline functions as it bloats the binary and provides dubious performance gains
* In C++ functions are called functions (or member functions) and not methods like in Java :)
* For the rest, check the code!


Development Environment
=======================

EmulationStation-DE is developed and compiled using GCC and GDB.
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
