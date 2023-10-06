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

#ifdef SPEW3DWEB_IMPLEMENTATION

#include <assert.h>

S3DEXP int s3dw_html_IsValidTagContinuationByte(char s) {
    if (s >= 'a' && s <= 'z')
        return 1;
    if (s >= 'A' && s <= 'Z')
        return 1;
    if (s >= '0' && s <= '9')
        return 1;
    if (s == '-' || s == '.' || s == '_' || (int)((unsigned char)s) > 127)
        return 1;
    return 0;
}

S3DEXP size_t s3dw_html_GetTagLengthByteBuf(
        const char *s, size_t slen,
        const char **out_tagname_start,
        size_t *out_tagname_len
        ) {
    if (slen <= 0 || *s != '<')
        return 0;
    char inquote = '\0';
    size_t i = 1;
    while (i < slen && (s[i] == ' ' ||
            s[i] == '\t')) {
        i += 1;
    }
    if (i < slen && (s[i] == '/' || s[i] == '\t'))
        i += 1;
    while (i < slen && (s[i] == ' ' ||
            s[i] == '\t')) {
        i += 1;
    }
    const char *tagname_start = s + i;
    int tagname_len = 1;
    if ((s[i] < 'a' || s[i] > 'z') &&
            (s[i] < 'A' || s[i] > 'Z')) {
        tagname_start = NULL;
        tagname_len = 0;
        i += 1;
    } else {
        i += 1;
        while (i < slen &&
                s3dw_html_IsValidTagContinuationByte(s[i])) {
            tagname_len += 1;
            i += 1;
        }
    }
    while (i < slen) {
        if (inquote == '\0' && (
                s[i] == '\'' ||
                s[i] == '\"')) {
            inquote = s[i];
        } else if (inquote == s[i]) {
            inquote = '\0';
        } else if (inquote == '\0' && s[i] == '>') {
             if (out_tagname_start)
                *out_tagname_start = tagname_start;
            if (out_tagname_len)
                *out_tagname_len = tagname_len;
            return i + 1;
        }
        i += 1;
    }
    return 0;
}

S3DEXP size_t s3dw_html_GetTagLengthStr(
        const char *s,
        const char **out_tagname_start,
        size_t *out_tagname_len
        ) {
    return s3dw_html_GetTagLengthByteBuf(s, strlen(s),
        out_tagname_start, out_tagname_len
    );
}

#endif  // SPEW3DWEB_IMPLEMENTATION

