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
    if (s == '-' || s == '.' || s == '_' ||
            (int)((unsigned char)s) > 127)
        return 1;
    return 0;
}

S3DEXP size_t s3dw_html_GetTagLengthByteBuf(
        const char *s, size_t slen
        ) {
    return s3dw_html_GetTagLengthByteBufEx(
        s, slen, 0, NULL, NULL, NULL, NULL, NULL, NULL
    );
}

S3DEXP size_t s3dw_html_GetTagLengthByteBufEx(
        const char *s, size_t slen,
        int opt_force_keep_invalid_attributes,

        const char **out_tag_name_start,
        size_t *out_tag_name_len,
        int *out_invalid_in_suspicious_ways,
        int *out_tag_syntax_type,
        void (*out_attr_callback)(
            const char *attr_name_start, size_t attr_name_len,
            const char *attr_value_start, size_t attr_value_len,
            void *userdata
        ), void *attr_callback_userdata
        ) {
    int tagsyntaxtype = S3DW_HTML_TAG_SYNTAX_OPENINGTAG;

    // Browse opening part:
    if (slen <= 0 || *s != '<')
        return 0;
    char inquote = '\0';
    size_t i = 1;
    if (i < slen && (s[i] == ' ' ||
            s[i] == '\t'))
        return 0;
    if (i < slen && s[i] == '/') {
        tagsyntaxtype = S3DW_HTML_TAG_SYNTAX_CLOSINGTAG;
        i += 1;
        while (i < slen && (s[i] == ' ' ||
                s[i] == '\t')) {
            i += 1;
        }
    }

    // Get the tag name:
    const char *tag_name_start = s + i;
    int tag_name_len = 1;
    if ((s[i] < 'a' || s[i] > 'z') &&
            (s[i] < 'A' || s[i] > 'Z')) {
        tag_name_start = NULL;
        tag_name_len = 0;
        i += 1;
    } else {
        i += 1;
        while (i < slen &&
                s3dw_html_IsValidTagContinuationByte(s[i])) {
            tag_name_len += 1;
            i += 1;
        }
    }

    // Go through attributes up to closing bracket:
    int invalid_in_suspicious_ways = 0;
    size_t current_attr_start = 0;
    size_t current_attr_name_end = 0;
    size_t current_attr_value_start = 0;
    int last_nonwhitespace_was_attr_equals = 0;
    while (i < slen) {
        if (inquote == '\0' && (
                s[i] == '\'' ||
                s[i] == '\"') &&
                last_nonwhitespace_was_attr_equals) {
            assert(current_attr_start > 0);
            assert(current_attr_name_end > current_attr_start);
            last_nonwhitespace_was_attr_equals = 0;
            inquote = s[i];
            current_attr_value_start = i;
        } else if (inquote == s[i]) {
            assert(current_attr_start > 0);
            assert(current_attr_name_end > current_attr_start);
            assert(current_attr_value_start > current_attr_name_end);
            if (out_attr_callback != NULL) {
                out_attr_callback(
                    s + current_attr_start,
                    (current_attr_name_end -
                     current_attr_start),
                    s + current_attr_value_start,
                    ((i + 1) - current_attr_value_start),
                    attr_callback_userdata
                );
            }
            last_nonwhitespace_was_attr_equals = 0;
            inquote = '\0';
            current_attr_start = 0;
            current_attr_name_end = 0;
            current_attr_value_start = 0;
        } else if (inquote == '\0' && (s[i] == '>' ||
                s[i] == '/')) {
            if (current_attr_start > 0) {
                if (current_attr_name_end == 0)
                    current_attr_name_end = i;
                if (out_attr_callback != NULL) {
                    if (current_attr_value_start > 0) {
                        out_attr_callback(
                            s + current_attr_start,
                            (current_attr_name_end -
                             current_attr_start),
                            s + current_attr_value_start,
                            ((i + 1) - current_attr_value_start),
                            attr_callback_userdata
                        );
                    } else {
                        out_attr_callback(
                            s + current_attr_start,
                            (current_attr_name_end -
                             current_attr_start),
                            NULL, 0,
                            attr_callback_userdata
                        );
                    }
                }
            }
            last_nonwhitespace_was_attr_equals = 0;
            if (s[i] == '>') {
                if (out_tag_name_start)
                    *out_tag_name_start = tag_name_start;
                if (out_tag_name_len)
                    *out_tag_name_len = tag_name_len;
                if (out_invalid_in_suspicious_ways)
                    *out_invalid_in_suspicious_ways = (
                        invalid_in_suspicious_ways
                    );
                if (out_tag_syntax_type)
                    *out_tag_syntax_type = tagsyntaxtype;
                return i + 1;
            } else {
                assert(s[i] == '/');
                if (tagsyntaxtype == S3DW_HTML_TAG_SYNTAX_OPENINGTAG)
                    tagsyntaxtype = (
                        S3DW_HTML_TAG_SYNTAX_SELFCLOSINGTAG
                    );
            }
            current_attr_start = 0;
            current_attr_name_end = 0;
            current_attr_value_start = 0;
        } else if (inquote == '\0' && s[i] == '=' &&
                current_attr_start > 0) {
            if (current_attr_start > 0) {
                last_nonwhitespace_was_attr_equals = 1;
                if (current_attr_name_end == 0)
                    current_attr_name_end = i;
            } else {
                last_nonwhitespace_was_attr_equals = 0;
                invalid_in_suspicious_ways = 1;
            }
        } else if (inquote == '\0' && (
                s[i] == ' ' || s[i] == '\r' ||
                s[i] == '\n' || s[i] == '\t')) {
            if (current_attr_start > 0) {
                current_attr_name_end = i;
            }
        } else if (inquote == '\0') {
            last_nonwhitespace_was_attr_equals = 0;
            if (inquote == '\0' && (
                    s[i] == '\'' ||
                    s[i] == '\"')) {
                invalid_in_suspicious_ways = 1;
            }
            if (current_attr_start == 0 ||
                    current_attr_name_end > 0) {
                if (current_attr_name_end > 0 &&
                        out_attr_callback != NULL) {
                    out_attr_callback(
                        s + current_attr_start,
                        (current_attr_name_end -
                         current_attr_start),
                        NULL, 0,
                        attr_callback_userdata
                    );
                }
                current_attr_start = i;
                current_attr_name_end = 0;
            }
        }
        i += 1;
    }
    return 0;
}

S3DEXP size_t s3dw_html_GetTagLengthStrEx(
        const char *s,
        int opt_force_keep_invalid_attributes,

        const char **out_tag_name_start,
        size_t *out_tag_name_len,
        int *out_invalid_in_suspicious_ways,
        int *out_tag_syntax_type,
        void (*out_attr_callback)(
            const char *attr_name_start, size_t attr_name_len,
            const char *attr_value_start, size_t attr_value_len,
            void *userdata
        ), void *attr_callback_userdata
        ) {
    return s3dw_html_GetTagLengthByteBufEx(
        s, strlen(s),
        opt_force_keep_invalid_attributes,
        out_tag_name_start, out_tag_name_len,
        out_invalid_in_suspicious_ways,
        out_tag_syntax_type,
        out_attr_callback, attr_callback_userdata
    );
}

S3DEXP size_t s3dw_html_GetTagLengthStr(
        const char *s
        ) {
    return s3dw_html_GetTagLengthStrEx(
        s, 0, NULL, NULL, NULL, NULL, NULL, NULL
    );
}

#endif  // SPEW3DWEB_IMPLEMENTATION

