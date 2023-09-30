
// Taken from docs/Markdown.md, please keep in sync.

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
    char *converted2 = spew3dweb_markdown_ToHTML(
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

