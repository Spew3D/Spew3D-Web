
<!-- For license of this file, see LICENSE.md in the base folder. -->

Documentation for Spew3D Markdown
=================================

This header provides access to a markdown parser, with two main
functionalities: markdown to HTML conversion, and cleaning up
markdown by removing all possibly ambiguous corner cases
that might trip up less intricate parsers. It also has a chunked
mode for use for really large files without needing to load them
all into memory.


Basic usage
-----------

For a simple markdown to HTML or markdown to markdown where you
don't mind having everything in memory, use this:

```c
#define SPEW3D_IMPLEMENTATION  // Only if not already in another file!
#define SPEW3D_OPTION_DISABLE_SDL  // Optional, drops graphical stuff.
#include <spew3d.h>
#define SPEW3DWEB_IMPLEMENTATION  // Only if not already in another file!
#include <spew3d-web.h>
#include <stdio.h>

void main(int argc, const char **argv) {

    printf("Markdown to markdown example:\n");
    char *converted = spew3dweb_markdown_Clean(
        "Small test example\n"
        "=\n"
        "Hello World from Spew3D-Web's markdown!\n"
        "- List item 1 ` [ ( <- dangerous nonsense\n"
        "- List item 2\n"
        "Done!"
    );
    printf("Result: \n%s\n\n", converted);
    free(converted);

    printf("Markdown to HTML example:\n");
    char *converted2 = spew3dweb_markdown_ToHTL(
        "Small test example\n"
        "=\n"
        "Hello World from Spew3D-Web's markdown!\n"
        "- List item 1\n"
        "- List item 2\n"
        "Done!"
    );
    printf("Result: \n%s\n\n", converted2);
    free(converted2);

}
```


Dialect
-------

Spew3D-Web's markdown parser implements the following markdown
dialect or variant, translated to HTML:

- Most of the basic feature set e.g. found in [CommonMark](
    https://commonmark.org) or [Markdown.pl](
    https://daringfireball.net/projects/markdown/syntax)
  although [not always the same
  corner-case handling](#parser-design-philosophy), and
  with additional support for:

- Basic **markdown tables** (e.g. seen in GitLab or GitHub markdown):

      |Column 1      |Column 2      |
      |--------------|--------------|
      |Row 1 cell 1  |Row 1 cell 2  |
      |Row 2 cell 1  |Row 1 cell 2  |

- Extra syntax for **image dimensions**, useful for High-DPI images
  (e.g. seen in GitLab markdown):

      ![Descriptive text](image.png){width=50% height=100px}

- Option to disable unsafe HTML for untrusted sources.


Parser Design Philosophy
------------------------

Spew3D-Web's markdown parser follows mostly
[CommonMark](https://commonmark.org), but tries to
stick more closely to a naive user's likely assumptions
based on the raw text's visual layout:

Example of indent breaking out of a list item:

    This is an example text before a list.
      - List item.
    Since this text isn't indented like the list item,
    it doesn't look like it's part of it.

Spew3D markdown's output, adhering to the visual break of the indent:

> This is an example text before a list.
>
>   - List item.
>
> Since this text isn't indented like the list item,
> it doesn't look like it's part of it.

[CommonMark](https://commonmark.org) interpretation:

> This is an example text before a list.
>
>   - List item. Since this text isn't indented
>     like the list item, it doesn't look like
>     it's part of it.

This diverges from `Markdown.pl`'s "lazy" rule, but allows
for more control and is more visually intuitive.

To automatically convert a possibly ambiguous markdown text
where other parser's interpretation would differ into a more
safe variant, [use `spew3dweb_markdown_Clean` like in
the basic example](#basic-usage).


Advanced functionality
----------------------

Check [the header file (already baked into spew3d-web.h, don't
separately download and/or include!!)](/include/spew3d_markdown.h)
for advanced functionality, like file streaming and/or loading
in chunks.

