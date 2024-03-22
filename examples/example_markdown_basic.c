/* Copyright (c) 2023, ellie/@ell1e & Spew3D Web Team (see AUTHORS.md).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

Alternatively, at your option, this file is offered under the Apache 2
license, see accompanied LICENSE.md.
*/

// Taken from docs/Markdown.md, please keep in sync.

#define SPEW3D_IMPLEMENTATION  // Only if not already in another file!
#define SPEW3D_OPTION_DISABLE_SDL  // Optional, drops graphical stuff.
#include <spew3d.h>
#define SPEW3DWEB_IMPLEMENTATION  // Only if not already in another file!
#include <spew3dweb.h>
#include <stdio.h>

int main(int argc, const char **argv) {

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

    return 0;
}

