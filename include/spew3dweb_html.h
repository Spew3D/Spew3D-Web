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

#ifndef SPEW3DWEB_HTML_H_
#define SPEW3DWEB_HTML_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>  // for size_t

#define S3DW_TAGSYNTAX_OPENINGTAG 1
#define S3DW_TAGSYNTAX_CLOSINGTAG 2
#define S3DW_TAGSYNTAX_SELFCLOSINGTAG 3

#define S3DW_HTMLEXTRACTTAG_RESULT_SUCCESS 0
#define S3DW_HTMLEXTRACTTAG_RESULT_NOVALIDTAG 1
#define S3DW_HTMLEXTRACTTAG_RESULT_OUTOFMEMORY 2

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
);

S3DEXP int s3dw_html_IsValidTagContinuationByte(char s);

S3DEXP size_t s3dw_html_GetTagLengthByteBuf(
    const char *s, size_t slen
);

S3DEXP size_t s3dw_html_GetTagLengthStr(
    const char *s
);

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
);

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
);

#endif  // SPEW3DWEB_HTML_H_

