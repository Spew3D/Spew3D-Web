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

/// A small example cool to convert markdown to HTML!

#define SPEW3D_IMPLEMENTATION  // Only if not already in another file!
#define SPEW3D_OPTION_DISABLE_SDL  // Optional, drops graphical stuff.
#include <spew3d.h>
#define SPEW3DWEB_IMPLEMENTATION  // Only if not already in another file!
#include <spew3dweb.h>
#include <stdio.h>
#include <string.h>

static const char *replace_ext_old = NULL;
static const char *replace_ext_new = NULL;

// This little helper function lets us change the file extension
// of URIs in our converted markdown, if the globals to request
// such a change were set:
char *our_little_uri_transform_helper(
        const char *uristr, void *userdata
        ) {
    if (!replace_ext_new)
        return strdup(uristr);

    if (replace_ext_old &&
            !s3d_uri_HasFileExtension(uristr, replace_ext_old)
            ) {
        return strdup(uristr);
    }

    char *newuristr = (
        s3d_uri_SetFileExtension(uristr, replace_ext_new)
    );
    return newuristr;
}

int main(int argc, const char **argv) {
    const char *filepath = NULL;
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "--") == 0) {
            if (i + 1 < argc)
                filepath = argv[i + 1];
            break;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("A small example tool for markdown to HTML!\n");
            return 0;
        } else if (strcmp(argv[i], "--replace-rel-link-ext") == 0) {
            if (i + 2 < argc) {
                replace_ext_old = argv[i + 1];
                replace_ext_new = argv[i + 2];
            }
            i += 2;
            continue;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("example_markdown_convert.c v0.1\n");
            return 0;
        } else if (filepath == NULL &&
                argv[i][0] != '-') {
            filepath = argv[i];
        } else {
            fprintf(stderr, "warning: unrecognized "
                "argument: %s\n", argv[i]);
            return 1;
        }
        i += 1;
    }
    if (!filepath) {
        fprintf(stderr, "error: please specify a file path as a "
            "command line argument!\n");
        return 1;
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "error: failed to open file: %s\n", filepath);
        return 1;
    }

    while (1) {
        size_t chunklen = 0;
        char *chunk = spew3dweb_markdown_GetIChunkFromDiskFile(
            f, 10 * 1024,
            &chunklen
        );
        if (chunk && chunklen == 0) {  // End of file.
            free(chunk);
            break;
        }
        char *html = (chunk ? spew3dweb_markdown_ToHTMLEx(
            chunk, 1, our_little_uri_transform_helper, NULL,
            NULL) : NULL);
        if (!chunk || !html) {
            free(chunk);
            free(html);
            fprintf(stderr, "error: I/O or out of memory error\n");
            return 1;
        }
        printf("%s\n", html);
        free(chunk);
        free(html);
    }
    fclose(f);
    return 0;
}

