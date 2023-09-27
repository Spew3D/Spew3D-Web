/* Copyright (c) 2023, ellie/@ell1e & Spew3D-Web Team (see AUTHORS.md).

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

#ifndef SPEW3DWEB_MARKDOWN_H_
#define SPEW3DWEB_MARKDOWN_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>  // for size_t

S3DEXP char *spew3dweb_markdown_GetIChunkFromCustomIO(
    int (*read_func)(char *buff, size_t amount, void *userdata),
    int (*checkeof)(void *userdata),
    int (*seekback_func)(size_t backward_amount, void *userdata),
    void *userdata,
    size_t opt_maxchunklen,
    size_t *out_len
);

S3DEXP char *spew3dweb_markdown_GetIChunkFromVFSFile(
    SPEW3DVFS_FILE *f, size_t opt_maxchunklen, size_t *out_len
);

S3DEXP char *spew3dweb_markdown_GetIChunkFromDiskFile(
    FILE *f, size_t opt_maxchunklen, size_t *out_len
);



S3DHID char *_internal_spew3dweb_markdown_GetIChunkFromVFSFileEx(
    SPEW3DVFS_FILE *f,
    char *optionalbuf, size_t optionalbufsize,
    size_t opt_maxchunklen, size_t opt_minchunklen,
    size_t *out_len
);

S3DHID char *_internal_spew3dweb_markdown_GetIChunkFromCustomIOEx(
    int (*read_func)(char *buff, size_t amount, void *userdata),
    int (*checkeof)(void *userdata),
    int (*seekback_func)(size_t backward_amount, void *userdata),
    void *userdata,
    char *optionalbuf, size_t optionalbufsize,
    size_t opt_maxchunklen, size_t opt_minchunklen,
    size_t *out_len
);

S3DHID char *_internal_spew3dweb_markdown_GetIChunkExFromMem(
    const char *original_buffer, size_t original_buffer_len,
    char *optionalbuf, size_t optionalbufsize,
    size_t opt_maxchunklen, size_t opt_minchunklen,
    size_t *out_len
);

static char *_internal_spew3dweb_markdown_GetIChunkExFromStr(
        const char *test_str,
        char *optionalbuf, size_t optionalbufsize,
        size_t opt_maxchunklen, size_t opt_minchunklen,
        size_t *out_len
        ) {
    return _internal_spew3dweb_markdown_GetIChunkExFromMem(
        test_str, strlen(test_str),
        optionalbuf, optionalbufsize,
        opt_maxchunklen, opt_minchunklen,
        out_len
    );
}

char *spew3dweb_markdown_CleanChunk(
    const char *inputstr, size_t input_len,
    size_t *out_len, size_t *out_alloc
);

#endif  // SPEW3DWEB_MARKDOWN_H_

