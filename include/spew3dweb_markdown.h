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
    SPEW3DVFS_FILE *f, size_t opt_maxchunklen,
    size_t *out_len
);

S3DEXP char *spew3dweb_markdown_GetIChunkFromDiskFile(
    FILE *f, size_t opt_maxchunklen,
    size_t *out_len
);

S3DEXP char *spew3dweb_markdown_CleanByteBuf(
    const char *uncleanbytes, size_t uncleanbyteslen,
    int opt_allowunsafehtml,
    int opt_stripcomments,
    char *(*opt_uritransformcallback)(
        const char *uri, void *userdata
    ),
    void *opt_uritransform_userdata,
    size_t *out_len, size_t *out_alloc
);

S3DEXP char *spew3dweb_markdown_CleanEx(
    const char *uncleanstr,
    int opt_allowunsafehtml,
    int opt_stripcomments,
    char *(*opt_uritransformcallback)(
        const char *uri, void *userdata
    ),
    void *opt_uritransform_userdata,
    size_t *out_len
);

S3DEXP char *spew3dweb_markdown_Clean(const char *uncleanstr);

S3DEXP char *spew3dweb_markdown_MarkdownBytesToAnchor(
    const char *bytebuf, size_t bytebuflen
);

typedef struct s3dw_markdown_tohtmloptions {
    int block_unsafe_html;
    int disable_heading_anchors;
    char *(*uritransform_callback)(
        const char *uri, void *userdata
    );
    void *uritransform_callback_userdata;
    int externallinks_no_target_blank;
    int externallinks_no_rel_noopener;
} s3dw_markdown_tohtmloptions;

S3DEXP char *spew3dweb_markdown_ByteBufToHTML(
    const char *markdownbytes, size_t markdownbyteslen,
    s3dw_markdown_tohtmloptions *options,
    size_t *out_len
);

S3DEXP char *spew3dweb_markdown_ToHTMLEx(
    const char *markdownstr,
    s3dw_markdown_tohtmloptions *options,
    size_t *out_len
);

S3DEXP char *spew3dweb_markdown_ToHTML(
    const char *markdownstr
);

S3DEXP int spew3dweb_markdown_GetBacktickStrLangPrefixLen(
    const char *block, size_t offset
);

S3DEXP int spew3dweb_markdown_GetBacktickByteBufLangPrefixLen(
    const char *block, size_t blocklen,
    size_t offset
);

S3DEXP int spew3dweb_markdown_IsStrUrl(const char *test_str);

S3DEXP int spew3dweb_markdown_IsStrImage(const char *test_str);

S3DHID ssize_t _internal_spew3dweb_markdown_GetInlineEndBracket(
    const char *input, size_t inputlen,
    size_t offset, char closebracket,
    int opt_trim_linebreaks_from_content,
    int *out_spacingstart, int *out_spacingend
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

S3DHID char *_internal_spew3dweb_markdown_CleanByteBufEx(
    const char *input, size_t inputlen,
    int opt_forcenolinebreaklinks,
    int opt_forceescapeunambiguousentities,
    int opt_allowunsafehtml,
    int opt_stripcomments,
    char *(*opt_uritransformcallback)(
        const char *uri, void *userdata
    ),
    void *opt_uritransform_userdata,
    size_t *out_len, size_t *out_alloc
);

S3DHID int _internal_spew3dweb_markdown_GetLinkOrImgLen(
    const char *input, size_t inputlen, size_t offset,
    int trim_linebreaks_from_content,
    int *out_title_start, int *out_title_len,
    int *out_url_start, int *out_url_len,
    int *out_prefix_url_linebreak_to_maintain_formatting,
    int *out_img_width, char *out_img_width_format,
    int *out_img_height, char *out_img_height_format
);

S3DHID ssize_t _internal_spew3dweb_markdown_AddInlineAreaClean(
    const char *input, size_t inputlen, size_t startpos,
    char **resultchunkptr, size_t *resultfillptr,
    size_t *resultallocptr, int origindent, int effectiveindent,
    int currentlineiscode, int opt_allowmultiline,
    int opt_squashmultiline,
    int opt_adjustindentinside,
    int opt_forcelinksoneline,
    int opt_escapeunambiguousentities,
    int opt_allowunsafehtml,
    int opt_stripcomments,
    char *(*opt_uritransformcallback)(
        const char *uri, void *userdata
    ),
    void *opt_uritransform_userdata
);

typedef struct _markdown_lineinfo _markdown_lineinfo;

S3DHID int _internal_s3dw_markdown_LineStartsTable(
    _markdown_lineinfo *lineinfo, size_t linei,
    size_t linefill, int *out_cells
);

S3DHID int _internal_s3dw_markdown_LineContinuesTable(
    _markdown_lineinfo *lineinfo, size_t linei,
    size_t linefill, int cells
);

S3DHID void _internal_s3dw_markdown_GetTableCell(
    _markdown_lineinfo *lineinfo, size_t linei,
    size_t linefill, int cell_no,
    int *out_startoffset, int *out_byteslen
);

S3DHID int _internal_s3dw_markdown_ensurebufsize(
    char **bufptr, size_t *bufalloc, size_t new_size
);

S3DHID int _internal_s3dw_markdown_bufappend(
    char **bufptr, size_t *bufalloc, size_t *buffill,
    const char *appendbuf, size_t appendbuflen, size_t amount
);

S3DHID int _internal_s3dw_markdown_bufappendstr(
    char **bufptr, size_t *bufalloc, size_t *buffill,
    const char *appendstr, size_t amount
);

S3DHID int _internal_s3dw_markdown_bufappendchar(
    char **bufptr, size_t *bufallocptr, size_t *buffillptr,
    char appendc, size_t amount
);

// (Warning, dangerous to increase since used on stack:)
#define _S3D_MD_MAX_FORMAT_NESTING 6

// (Warning, dangerous to increase since used on stack:)
#define _S3D_MD_MAX_LIST_NESTING 12

#endif  // SPEW3DWEB_MARKDOWN_H_

