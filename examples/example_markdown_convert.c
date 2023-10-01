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

// This little helper function lets us change the file extension
// of URIs in our converted markdown:
char *our_little_uri_transform_helper(
        const char *uristr, void *userdata
        ) {
    s3d_uriinfo *uri = s3d_uri_ParseAny(uristr, "https");
    if (!uristr)
        return NULL;
    
}

int main(int argc, const char **argv) {
    const char *filepath = NULL;
    const char *replace_ext_old = NULL;
    const char *replace_ext_new = NULL;
    int i = 1;
    while (i < argc) {
        if (argv[i] == "--") {
            if (i + 1 < argc)
                filepath = argv[i + 1];
            break;
        } else if (strcmp(argv[i], "--help")) {
            printf("A small example tool for markdown to HTML!\n");
            return 0;
        } else if (strcmp(argv[i], "--replace-rel-link-ext")) {
            if (i + 2 < argc) {
                replace_ext_old = argv[i + 1];
                replace_ext_new = argv[i + 2];
            }
            i += 2;
            continue;
        } else if (strcmp(argv[i], "--version")) {
            printf("example_markdown_convert.c v0.1\n");
            return 0;
        } else {
            printf("warning: unrecognized argument: %s\n", argv[i]);
            return 1;
        }
        i += 1;
    }
    if (!filepath) {
        printf("error: please specify a file path as a command "
            "line argument!\n");
        return 1;
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        printf("error: failed to open file: %s\n", filepath);
        return 1;
    }

    /*while (1) {
        char *chunk = spew3dweb_markdown_GetIChunkFromDiskFile(
            f, 0,
            char (*opt_uritransformcallback)(const char *uri),
            size_t *out_len, int *out_endoffile
        );
    */
    fclose(f);
    return 0;
}

