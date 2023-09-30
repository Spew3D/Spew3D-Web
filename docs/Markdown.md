
Documentation for `spew3d_markdown.h`
=====================================

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
#include <stdio.h>

#define SPEW3D_IMPLEMENTATION  // Only if not already in another file!
#include <spew3d.h>
#define SPEW3DWEB_IMPLEMENTATION  // Only if not already in another file!
#include <spew3d-web.h>

void main(int argc, const char **argv) {

    printf("Markdown to markdown example:\n");
    char *converted = spew3dweb_markdown_Clean(
        "Small test example\n"
        "=\n"
        "Hello World from Spew3D-Web's markdown!\n"
        "- List item 1 ` [ ( <- dangerous nonsense\n"
        "- List item 2\n"
        "Done!",
        1, NULL
    );
    printf("Result: %s\n", converted);
    free(converted);

    printf("Markdown to HTML example:\n");
    char *converted = spew3dweb_markdown_Clean( 
        "Small test example\n" 
        "=\n"
        "Hello World from Spew3D-Web's markdown!\n" 
        "- List item 1\n"
        "- List item 2\n"
        "Done!" 
    ); 
    printf("Result: %s\n", converted);
    free(converted);

}
```

Dialect
-------

Spew3D-Web's markdown parser implements the following markdown
dialect or variant:

- The basic feature set e.g. found in [CommonMark](
    https://commonmark.org) although not [always the same
  corner-case handling](#parser-design-philosophy), and
  with additional support for:

- Basic **markdown tables** as done by GitLab or GitHub markdown:

      |Column 1      |Column 2      |
      |--------------|--------------|
      |Row 1 cell 1  |Row 1 cell 2  |
      |Row 2 cell 1  |Row 1 cell 2  |

- Extra syntax for **image dimensions**, useful for High-DPI images:

      ![Descriptive text](image.png){width=100 height=100px}

- Option to disable unsafe HTML for untrusted sources.

Parser Design Philosophy
------------------------

The parser follows mostly [CommonMark](https://commonmark.org),
except where it might not follow what a visual human user expects.
This mostly centers around indent, where if indent visually
breaks out of a nested block, then Spew3D's parser will also
break out of that block while CommonMark will not.

Example of indent breaking out of block:

    This is an example text before a list.
      - List item.
    Since this text isn't indented like the list item,
    it doesn't look like it's part of it.

Spew3D markdown's output:

> This is an example text before a list.
>   - List item.
>
> Since this text isn't indented like the list item,
> it doesn't look like it's part of it.

[CommonMark](https://commonmark.org) interpretation:

> This is an example text before a list.
>   - List item. Since this text isn't indented
> like the list item, it doesn't look like it's part of it.

To automatically convert a possibly ambiguous markdown text
where CommonMark's interpretation would differ into a more
safe variant, [use `spew3dweb_markdown_Clean` like in
the basic example](#basic-usage).

Advanced functionality
----------------------

Check [the header file (already baked into spew3d-web.h, don't
separately download and/or include!!)](/include/spew3d_markdown.h)
for advanced functionality, like file streaming and/or loading
in chunks.

