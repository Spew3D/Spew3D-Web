
/// A small example cool to convert markdown to HTML!

#define SPEW3D_IMPLEMENTATION  // Only if not already in another file!
#define SPEW3D_OPTION_DISABLE_SDL  // Optional, drops graphical stuff.
#include <spew3d.h>
#define SPEW3DWEB_IMPLEMENTATION  // Only if not already in another file!
#include <spew3d-web.h>
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

