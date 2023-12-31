
Spew3D Web
==========

On top of [Spew3D](https://codeberg.org/Spew3D/Spew3D), this
library **Spew3D Web** implements various formats and helpers
that might be used by Web and document applications.
Since most basic games won't need them, they're in a separate
library.

**Provided formats:**

- **HTML parser:** for parsing HTML and XML tag soups, with optional
  low level streaming parsing for fast processing.

- **Markdown parser:** for the common document text format, supporting
  the common basics along with tables, and more.
  [Read tutorial here...](/docs/Markdown.md)

- **URI parser:** parse URIs and file paths in complex and sophisticated
  ways, and related helper functions e.g. for URI encoding.
  [See funcs here...](/include/spew3dweb_uri.h)

- **Big number parser:** Parse big fractional numbers from strings of
  format `-DDDDD.DDDDD` with `D` being a digit from `0` to `9`, the
  leading minus and the fractional dot being optonal, and allow various
  operations on them.

  Some operations will trim down the fractional part
  to limited precision, but the non-fractional number has no limit
  other than the total system memory available.
  [See funcs here...](/include/spew3dweb_bignum.h)

For everything else, the [header files](./include/) themselves
may include guidance and basic functionality, as well as
the ['examples' folder](./examples/).


Compiling and usage
-------------------

*(Get `spew3d.h` [from here](https://codeberg.org/Spew3D/Spew3D/releases),
and `spew3dweb.h`
[from here](https://codeberg.org/Spew3D/Spew3DWeb/releases).)*

**Step 1:** Add `spew3d.h` and `spew3dweb.h` into your project's
code folder, and put this in all your files where you want to use
them from:

  ```C
  #include <spew3d.h>
  #include <spew3dweb.h>  // must be included after spew3d.h!
  ```

**Step 2:** In only a single object file, add a `#define
SPEW3DWEB_IMPLEMENTATION` which will make it contain the actual
implementation code and not just its API header:

  ```C
  #define SPEW3D_IMPLEMENTATION
  #include <spew3d.h>
  #define SPEW3DWEB_IMPLEMENTATION
  #include <spew3dweb.h>
  ```

**Step 3:** When you link your final program, make sure to add [SDL2](
https://libsdl.org) to your linked libraries, unless you're using
the [Spew3D option to not use SDL](
https://codeberg.org/Spew3D/Spew3D#options).


Run tests
---------

Currently, running the tests is only supported on Linux.
This will also generate the `spew3dweb.h` file if you checked out
the development version of Spew3D, at `include/spew3dweb.h`.

To run the tests, install SDL2 and libcheck (the GNU unit
test library for C) system-wide, then use: `make test`


License
-------

Spew3D Web is licensed under a BSD license or an Apache 2
License, [see here for details](LICENSE.md).
It includes other projects baked in, see `vendor` folder in the
repository.


Supported platforms and compilers
---------------------------------

Same as Spew3D.

