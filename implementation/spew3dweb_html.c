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

struct _extractnexttag_result {
    size_t attribute_count, attribute_alloc;
    char **attribute_name, **attribute_value;
    size_t *attribute_name_len,
           *attribute_value_len;
    int outofmemory;
};

void _extractnexttag_result_freecontents(
        struct _extractnexttag_result *result
        ) {
    size_t i = 0;
    while (i < result->attribute_count) {
        free(result->attribute_name[i]);
        free(result->attribute_value[i]);
        i += 1;
    }
    free(result->attribute_name);
    free(result->attribute_name_len);
    free(result->attribute_value);
    free(result->attribute_value_len);
}

static int _extractnexttag_collect_attribute_cb(
        const char *attr_name_start, size_t attr_name_len,
        const char *attr_value_start, size_t attr_value_len,
        void *userdata
        ) {
    struct _extractnexttag_result *result = userdata;
    if (result->attribute_count + 1 >
            result->attribute_alloc) {
        size_t new_alloc = result->attribute_count + 5;
        char **new_attribute_name = realloc(
            result->attribute_name,
            sizeof(*result->attribute_name) * new_alloc
        );
        if (!new_attribute_name) {
            result->outofmemory = 1;
            return 0;
        }
        result->attribute_name = new_attribute_name;
        size_t *new_attribute_name_len = realloc(
            result->attribute_name_len,
            sizeof(*result->attribute_name_len) * new_alloc
        );
        if (!new_attribute_name_len) {
            result->outofmemory = 1;
            return 0;
        }
        result->attribute_name_len = (
            new_attribute_name_len
        );
        char **new_attribute_value = realloc(
            result->attribute_value,
            sizeof(*result->attribute_value) * new_alloc
        );
        if (!new_attribute_value) {
            result->outofmemory = 1;
            return 0;
        }
        result->attribute_value = new_attribute_value;
        size_t *new_attribute_value_len = realloc(
            result->attribute_value_len,
            sizeof(*result->attribute_value_len) * new_alloc
        );
        if (!new_attribute_value_len) {
            result->outofmemory = 1;
            return 0;
        }
        result->attribute_value_len = (
            new_attribute_value_len
        );
        result->attribute_alloc = new_alloc;
    }
    char *attr_name = malloc(attr_name_len + 1);
    if (!attr_name) {
        result->outofmemory = 1;
        return 0;
    }
    memcpy(attr_name, attr_name_start, attr_name_len);
    attr_name[attr_name_len] = '\0';
    char *attr_value = malloc(attr_value_len + 1);
    if (!attr_value) {
        result->outofmemory = 1;
        return 0;
    }
    memcpy(attr_value,
           attr_value_start, attr_value_len);
    attr_value[attr_value_len] = '\0';
    result->attribute_name[result->attribute_count] = attr_name;
    result->attribute_name_len[result->attribute_count] = (
        attr_name_len
    );
    result->attribute_value[result->attribute_count] = attr_value;
    result->attribute_value_len[result->attribute_count] = (
        attr_value_len
    );
    result->attribute_count += 1;
    return 1;
}

S3DEXP int s3dw_html_ExtractNextTag(
        const char *s, size_t slen,
        int opt_force_keep_invalid_attributes,

        size_t *out_tag_byteslen,
        char **out_tag_name,
        int *out_tag_syntax_type,
        size_t *out_attribute_count,
        char ***out_attribute_name,
        size_t **out_attribute_name_len,
        char ***out_attribute_value,
        size_t **out_attribute_value_len
        ) {
    struct _extractnexttag_result result = {0};

    const char *_tag_name_start;
    size_t _tag_name_len;
    int _invalid_in_suspicious_ways;
    int _tag_syntax_type;

    size_t len = s3dw_html_GetTagLengthByteBufEx(
        s, slen, opt_force_keep_invalid_attributes,

        &_tag_name_start, &_tag_name_len,
        &_invalid_in_suspicious_ways,
        &_tag_syntax_type,
        &_extractnexttag_collect_attribute_cb,
        &result
    );
    if (len == 0) {
        _extractnexttag_result_freecontents(&result);
        if (result.outofmemory)
            return S3DW_HTMLEXTRACTTAG_RESULT_OUTOFMEMORY;
        return S3DW_HTMLEXTRACTTAG_RESULT_NOVALIDTAG;
    }
    assert(!result.outofmemory);

    char *tag_name = malloc(_tag_name_len + 1);
    if (!tag_name) {
        _extractnexttag_result_freecontents(&result);
        return S3DW_HTMLEXTRACTTAG_RESULT_OUTOFMEMORY;
    }
    memcpy(tag_name, _tag_name_start, _tag_name_len);
    tag_name[_tag_name_len] = '\0';

    if (out_tag_name)
        *out_tag_name = tag_name;
    else
        free(tag_name);
    if (out_tag_byteslen)
        *out_tag_byteslen = len;
    if (out_tag_syntax_type)
        *out_tag_syntax_type = _tag_syntax_type;
    if (out_attribute_count)
        *out_attribute_count = result.attribute_count;
    if (out_attribute_name) {
        *out_attribute_name = result.attribute_name;
    } else {
        size_t i = 0;
        while (i < result.attribute_alloc) {
            free(result.attribute_name[i]);
            i += 1;
        }
        free(result.attribute_name);
    }
    if (out_attribute_name_len)
        *out_attribute_name_len = result.attribute_name_len;
    else
        free(result.attribute_name_len);

    return S3DW_HTMLEXTRACTTAG_RESULT_SUCCESS;
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
        int (*out_attr_callback)(
            const char *attr_name_start, size_t attr_name_len,
            const char *attr_value_start, size_t attr_value_len,
            void *userdata
        ), void *attr_callback_userdata
        ) {
    int tagsyntaxtype = S3DW_TAGSYNTAX_OPENINGTAG;

    // Browse opening part:
    if (slen <= 0 || *s != '<')
        return 0;
    char inquote = '\0';
    size_t i = 1;
    if (i < slen && (s[i] == ' ' ||
            s[i] == '\t'))
        return 0;
    if (i < slen && s[i] == '/') {
        tagsyntaxtype = S3DW_TAGSYNTAX_CLOSINGTAG;
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
        } else if (inquote != '\0' && inquote != 'a' &&
                inquote == s[i]) {
            assert(current_attr_start > 0);
            assert(current_attr_name_end > current_attr_start);
            assert(current_attr_value_start > current_attr_name_end);
            if (out_attr_callback != NULL) {
                if (!out_attr_callback(
                        s + current_attr_start,
                        (current_attr_name_end -
                         current_attr_start),
                        s + current_attr_value_start,
                        ((i + 1) - current_attr_value_start),
                        attr_callback_userdata
                        ))
                    return 0;
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
                        if (!out_attr_callback(
                                s + current_attr_start,
                                (current_attr_name_end -
                                 current_attr_start),
                                s + current_attr_value_start,
                                ((i + 1) - current_attr_value_start),
                                attr_callback_userdata
                                ))
                            return 0;
                    } else {
                        if (!out_attr_callback(
                                s + current_attr_start,
                                (current_attr_name_end -
                                 current_attr_start),
                                NULL, 0,
                                attr_callback_userdata
                                ))
                            return 0;
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
                if (tagsyntaxtype == S3DW_TAGSYNTAX_OPENINGTAG)
                    tagsyntaxtype = (
                        S3DW_TAGSYNTAX_SELFCLOSINGTAG
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
                    if (!out_attr_callback(
                            s + current_attr_start,
                            (current_attr_name_end -
                             current_attr_start),
                            NULL, 0,
                            attr_callback_userdata
                            ))
                        return 0;
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
        int (*out_attr_callback)(
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

