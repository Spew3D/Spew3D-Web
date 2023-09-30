
Spew3D-Web
==========

On top of [Spew3D](https://codeberg.org/Spew3D/Spew3D), this
library **Spew3D-Web** implements various formats and helpers
that might be used by Web and document applications.
Since most basic games won't need them, they're in a separate
library.

**Provided formats:**

- **Markdown:** A self-contained markdown parser, which implements
  the [common markdown subsect including tables](
      /docs/Markdown.md#dialect).

- **JSON:** upcoming.

Compiling / Usage
-----------------

*(Get `spew3d.h` [from here](https://codeberg.org/Spew3D/Spew3D/releases),
and `spew3d-web.h`
[from here](https://codeberg.org/Spew3D/Spew3D-Web/releases).)*

**Step 1:** Add `spew3d.h` and `spew3d-web.h` into your project's
code folder, and put this in all your files where you want to use
them from:

  ```C
  #include <spew3d.h>
  #include <spew3d-web.h>  // must be included after spew3d.h!
  ```

**Step 2:** In only a single object file, add this define which
will make it contain the actual implementation code and not just its API:

  ```C
  #define SPEW3D_IMPLEMENTATION
  #include <spew3d.h>
  #define SPEW3DWEB_IMPLEMENTATION
  #include <spew3d-web.h>
  ```

**Step 3:** When you link your final program, make sure to add [SDL2](
https://libsdl.org) to your linked libraries, unless you're using
the [Spew3D option to not use SDL](
https://codeberg.org/Spew3D/Spew3D#options).

Documentation
-------------

Spew3D-Web offers:

- **Markdown parser:** for the common document text format, supporting
  most of [CommonMark](https://commonmark.org), tables, and more.
  [Read tutorial here...](
    /docs/Markdown.md
  ).

- **URI parser:** parse URIs and file paths in complex and sophisticated
  ways, and related helper functions e.g. for URI encoding.
  [See funcs here...](/include/spew3d_uri.h)

For everything else, the [header files](./include/) themselves
may include guidance and basic functionality, as well as
the ['examples' folder](./examples/).

Run Tests
---------

Currently, running the tests is only supported on Linux.
This will also generate the `spew3d-web.h` file if you checked out
the development version of Spew3D, at `include/spew3d-web.h`.

To run the tests, install SDL2 and libcheck (the GNU unit
test library for C) system-wide, then use: `make test`

License
-------

Spew3D-Web is licensed under a BSD license or an Apache 2
License, [see here for details](LICENSE.md).
It includes other projects baked in, see `vendor` folder in the
repository.

Supported Platforms / Compilers
-------------------------------

Same as Spew3D.

