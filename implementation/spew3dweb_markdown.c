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
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define INSC(insertchar) \
    (_internal_s3dw_markdown_bufappendchar(\
    &resultchunk, &resultalloc, &resultfill,\
    insertchar, 1))
#define INS(insertstr) \
    (_internal_s3dw_markdown_bufappendstr(\
    &resultchunk, &resultalloc, &resultfill,\
    insertstr, 1))
#define INSREP(insertstr, amount) \
    (_internal_s3dw_markdown_bufappendstr(\
    &resultchunk, &resultalloc, &resultfill,\
    insertstr, amount))
#define INSBUF(insertbuf, insertbuflen) \
    (_internal_s3dw_markdown_bufappend(\
    &resultchunk, &resultalloc, &resultfill,\
    insertbuf, insertbuflen, 1))

typedef struct _markdown_lineinfo {
    const char *linestart;
    int indentlen, indentedcontentlen;
} _markdown_lineinfo;

static int _getlinelen(const char *start, size_t max) {
    int linelen = 0;
    while (max > 0) {
        if (*start == '\n' || *start == '\r')
            return linelen;
        max -= 1;
        start += 1;
    }
    return linelen;
}

static int _m2htmlline_line_get_next_line_heading_strength(
        _markdown_lineinfo *lineinfo,
        size_t linefill, size_t lineindex
        ) {
    size_t i = lineindex;
    int indent = lineinfo[i].indentlen;
    if (i + 1 >= linefill ||
            lineinfo[i + 1].indentlen != indent)
        return 0;
    size_t indentclen = lineinfo[i].indentedcontentlen;
    if (indentclen == 0 ||
            (lineinfo[i].linestart[indentclen] == '#' &&
            (indentclen <= 1 ||
            lineinfo[i].linestart[indentclen + 1] == ' ' ||
            lineinfo[i].linestart[indentclen + 1] == '\t')))
        return 0;
    int indent2 = lineinfo[i + 1].indentlen;
    size_t indentclen2 = lineinfo[i + 1].indentedcontentlen;
    if (indentclen2 == 0 || (
            lineinfo[i + 1].linestart[indent2] != '=' &&
            lineinfo[i + 1].linestart[indent2] != '-'))
        return 0;
    char headingchar = lineinfo[i + 1].linestart[indent2];
    if (indentclen > 1 && (indentclen2 <= 1 ||
            lineinfo[i + 1].linestart[indent2 + 1] != headingchar))
        return 0;
    size_t k = indent2;
    while (k < indent2 + indentclen2) {
        if (lineinfo[i + 1].linestart[k] != headingchar)
            return 0;
        k += 1;
    }
    return ((headingchar == '=') ? 1 : 2);
}

static int _m2htmlline_start_list_number_len(
        _markdown_lineinfo *lineinfo, int lineindex,
        int *numval
        ) {
    size_t i = 0;
    size_t ipastend = lineinfo[lineindex].indentedcontentlen;
    const char *p = (lineinfo[lineindex].linestart +
        lineinfo[lineindex].indentlen);
    if (i >= ipastend ||
            (p[i] < '1' || p[i] > '9'))
        return 0;
    char numbuf[16] = "";
    while (i < ipastend && (p[i] >= '1' && p[i] <= '9')) {
        if (strlen(numbuf) < sizeof(numbuf) - 1) {
            numbuf[strlen(numbuf)] = p[i];
        }
        i += 1;
    }
    if (i + 1 >= ipastend || p[i] != '.' ||
            (p[i + 1] != ' ' && p[i + 1] != '\t'))
        return 0;
    i += 2;
    while (i < ipastend && (p[i] == ' ' || p[i] == '\t'))
        i += 1;
    if (numval) *numval = atoi(numbuf);
    return i;
}

#define _FORMAT_TYPE_ASTERISK1 1
#define _FORMAT_TYPE_ASTERISK2 2
#define _FORMAT_TYPE_UNDERLINE2 3
#define _FORMAT_TYPE_TILDE2 4

static int _md2html_fmt_type_len(int type) {
    return (type == _FORMAT_TYPE_ASTERISK1 ? 1 : 2);
}

static char _md2html_get_inline_fmt_type(
        const char *linebuf, size_t ipastend, size_t i,
        int *out_canopen, int *out_canclose
        ) {
    int type = 0;
    int canclose = (i > 0 &&
        linebuf[i - 1] != ' ' &&
        linebuf[i - 1] != '\t');
    int canopen = 1;
    if (linebuf[i] == '~' &&
            i + 1 < ipastend &&
            linebuf[i + 1] == '~') {
        type = _FORMAT_TYPE_TILDE2;
        if (i + 2 >= ipastend ||
                (linebuf[i + 2] == ' ' &&
                linebuf[i + 2] == '\t'))
            canopen = 0;
    }
    if (linebuf[i] == '*') {
        if (i + 1 < ipastend &&
                linebuf[i + 1] == '*') {
            type = _FORMAT_TYPE_ASTERISK2;
            if (i + 2 >= ipastend ||
                    (linebuf[i + 2] == ' ' &&
                    linebuf[i + 2] == '\t'))
                canopen = 0;
        } else {
            type = _FORMAT_TYPE_ASTERISK1;
            if (i + 1 >= ipastend ||
                (linebuf[i + 1] == ' ' &&
                linebuf[i + 1] == '\t'))
            canopen = 0;
        }
    }
    if (linebuf[i] == '_' &&
            i + 1 < ipastend && linebuf[i + 1] == '_') {
        type = _FORMAT_TYPE_UNDERLINE2;
        if (i + 2 >= ipastend ||
                (linebuf[i + 2] == ' ' &&
                linebuf[i + 2] == '\t'))
            canopen = 0;
    }
    if (type != 0) {
        if (out_canopen) *out_canopen = canopen;
        if (out_canclose) *out_canclose = canclose;
        return type;
    }
    if (out_canopen) *out_canopen = 0;
    if (out_canclose) *out_canclose = 0;
    return 0;
}

static int _m2html_check_line_has_link(
        _markdown_lineinfo *lineinfo, size_t linei, size_t i
        ) {
    size_t starti = i;
    size_t len = (lineinfo[linei].indentlen +
        lineinfo[linei].indentedcontentlen);
    while (i < len) {
        if (lineinfo[linei].linestart[i] == '[') {
            size_t linklen = (
                _internal_spew3dweb_markdown_GetLinkOrImgLen(
                    lineinfo[linei].linestart, len, i, 1,
                    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                    NULL, NULL
                ));
            if (linklen > 0)
                return 1;
        } else if (lineinfo[linei].linestart[i] == '\\') {
            i += 2;
            continue;
        }
        i += 1;
    }
    return 0;
}

static int _is_guaranteed_ext_link(
        const char *linkbytes, size_t byteslen
        ) {
    size_t i = 0;
    while (i < byteslen) {
        if (linkbytes[i] == '?' || linkbytes[i] == '#' ||
                (linkbytes[i] == ':' && (
                i + 1 >= byteslen || linkbytes[i + 1] != '/')) ||
                linkbytes[i] == '\t' || linkbytes[i] == '\r' ||
                linkbytes[i] == '\n' ||
                linkbytes[i] == ' ' || linkbytes[i] == '.')
            return 0;
        if (linkbytes[i] == ':') {
            return (i + 2 < byteslen &&
                linkbytes[i + 1] == '/' &&
                linkbytes[i + 2] == '/');
        }
        i += 1;
    }
    return 0;
}

static int _spew3d_markdown_process_inline_content(
        char **resultchunkptr, size_t *resultfillptr,
        size_t *resultallocptr,
        _markdown_lineinfo *lineinfo, size_t lineinfofill,
        int startline, int endbeforeline,
        int start_at_content_index, int end_at_content_index,
        int as_code, int isinheading,
        s3dw_markdown_tohtmloptions *options,
        int *out_endlineidx
        ) {
    char *resultchunk = *resultchunkptr;
    size_t resultfill = *resultfillptr;
    size_t resultalloc = *resultallocptr;

    int headingbylinebelow = 0;
    int endline = startline;
    if (endline + 1 < endbeforeline &&
            isinheading)
        endbeforeline = endline + 1;

    assert(end_at_content_index < 0 ||
        endbeforeline <= startline + 1);
    size_t inside_linktitle_ends_at = 0;
    int inside_linktitle_minimum_fnesting = 0;
    size_t past_link_idx = 0;
    size_t inside_imgtitle_ends_at = 0;
    size_t past_image_idx = 0;

    #define LINEC(l, c) \
        (lineinfo[l].linestart[\
            lineinfo[l].indentlen + c\
        ])

    while (endline + 1 < endbeforeline &&
            !as_code &&
            lineinfo[endline + 1].indentlen ==
            lineinfo[startline].indentlen &&
            lineinfo[endline + 1].indentedcontentlen > 0 &&
            (LINEC(endline + 1, 0) != '-' || (
                lineinfo[endline + 1].indentedcontentlen > 1 &&
                LINEC(endline + 1, 1) != '-' &&
                LINEC(endline + 1, 1) != ' ' &&
                LINEC(endline + 1, 1) != '\t')) &&
            (LINEC(endline + 1, 0) != '*' || (
                lineinfo[endline + 1].indentedcontentlen > 1 &&
                LINEC(endline + 1, 1) != ' ' &&
                LINEC(endline + 1, 1) != '\t' &&
                (LINEC(endline + 1, 1) != '*' ||
                 lineinfo[endline + 1].indentedcontentlen > 2 &&
                 LINEC(endline + 1, 2) != '*' &&
                 LINEC(endline + 1, 2) != ' ' &&
                 LINEC(endline + 1, 2) != '\t'))) &&
            (LINEC(endline + 1, 0) != '>' || (
                lineinfo[endline + 1].indentedcontentlen > 1 &&
                LINEC(endline + 1, 1) != ' ')) &&
            (LINEC(endline + 1, 0) != '#' || (
                lineinfo[endline + 1].indentedcontentlen > 1 &&
                LINEC(endline + 1, 1) != '#' &&
                LINEC(endline + 1, 1) != ' ' &&
                LINEC(endline + 1, 1) != '\t')) &&
            (LINEC(endline + 1, 0) != '`' || (
                lineinfo[endline + 1].indentedcontentlen < 3 ||
                LINEC(endline + 1, 1) != '`' ||
                LINEC(endline + 1, 2) != '`')) &&
            (_m2htmlline_line_get_next_line_heading_strength(
                lineinfo, lineinfofill, endline + 1) >= 0) &&
            (_m2htmlline_start_list_number_len(
                lineinfo, endline + 1, NULL) <= 0))
        endline += 1;
    #undef LINEC
    /*printf("_spew3d_markdown_process_inline_content on "
        "'%s' line range %d to %d\n",
        lineinfo[startline].linestart + lineinfo[startline].indentlen,
        startline, endline);*/
    char fnestings[_S3D_MD_MAX_FORMAT_NESTING];
    int fnestingsdepth = 0;
    size_t iline = startline;
    while (iline <= endline) {
        /*printf("_spew3d_markdown_process_inline_content "
            "processing line %d\n", iline);*/
        if (iline > startline) {
            if (!INSC(' ')) {
                errorquit: ;
                *resultchunkptr = resultchunk;
                *resultfillptr = resultfill;
                *resultallocptr = resultalloc;
                return 0;
            }
        }
        size_t i = lineinfo[iline].indentlen;
        if (start_at_content_index > 0)
            i = (size_t)start_at_content_index;
        size_t ipastend = (
            lineinfo[iline].indentedcontentlen +
            lineinfo[iline].indentlen);
        if (end_at_content_index >= 0)
            ipastend = end_at_content_index;
        const char *linebuf = lineinfo[iline].linestart;
        while (i < ipastend) {
            if (inside_linktitle_ends_at > 0 &&
                    i >= inside_linktitle_ends_at) {
                linkend: ;
                if (!INS("</a>"))  // Close link title
                    goto errorquit;
                i = past_link_idx;
                inside_linktitle_ends_at = 0;
                fnestingsdepth = (
                    inside_linktitle_minimum_fnesting
                );
                continue;
            }
            if (inside_imgtitle_ends_at > 0 &&
                    i >= inside_imgtitle_ends_at) {
                imgend: ;
                if (!INS("'/>"))  // Close 'alt' attribute
                    goto errorquit;
                i = past_image_idx;
                inside_imgtitle_ends_at = 0;
                continue;
            }
            int _fmtcanopen, _fmtcanclose;
            int _fmttype = 0;
            if (fnestingsdepth < _S3D_MD_MAX_FORMAT_NESTING &&
                    !as_code &&
                    inside_imgtitle_ends_at == 0 &&
                    (_fmttype = _md2html_get_inline_fmt_type(
                        linebuf, ipastend, i,
                        &_fmtcanopen, &_fmtcanclose
                    )) > 0 && _fmtcanopen &&
                    (!_fmtcanclose || (
                    fnestingsdepth == 0 ||
                    fnestings[fnestingsdepth - 1] != _fmttype))) {
                // This opens the given formatting, if there's an end:

                size_t scantruncate = 0;
                if (inside_imgtitle_ends_at > 0)
                    scantruncate = inside_imgtitle_ends_at;
                assert(scantruncate == 0 || scantruncate > i);

                // See if we can find the fitting end:
                int previousnesting = fnestingsdepth;
                fnestingsdepth++;
                fnestings[fnestingsdepth - 1] = _fmttype;
                size_t i2 = i + _md2html_fmt_type_len(_fmttype);
                size_t i2pastend = ipastend;
                if (scantruncate > 0 &&
                        i2pastend > scantruncate)
                    i2pastend = scantruncate;
                size_t foundendinline = 0;
                int foundpastidx = -1;
                int incode = 0;
                size_t iline2 = iline;
                const char *linebuf2 = lineinfo[iline2].linestart;
                while (iline2 <= endline &&
                        (scantruncate == 0 ||
                        iline2 == iline)) {
                    while (i2 < i2pastend) {
                        if (linebuf2[i2] == '\\') {
                            i2 += 2;
                            continue;
                        }
                        if (linebuf2[i2] == '`') incode = !incode;
                        // Skip over inline links and images,
                        // so they can't mess our outer formatting:
                        if (!incode && (linebuf2[i2] == '!' ||
                                linebuf2[i2] == '[')) {
                            size_t linklen = (
                                _internal_spew3dweb_markdown_GetLinkOrImgLen(
                                    linebuf2 + i2,
                                    i2pastend - i2, 0, 1,
                                    NULL, NULL, NULL, NULL,
                                    NULL, NULL, NULL, NULL, NULL
                                ));
                            if (linklen > 0) {
                                i2 += linklen;
                                continue;
                            }
                        }

                        int _innerfmt, _innercanopen, _innercanclose;
                        _innerfmt = _md2html_get_inline_fmt_type(
                            linebuf2, i2pastend, i2,
                            &_innercanopen, &_innercanclose
                        );
                        // Special case: forbid closing right after open:
                        if (iline2 == iline &&
                                i2 == i + _md2html_fmt_type_len(_fmttype))
                            _innercanclose = 0;
                        if (_innerfmt > 0 && !incode && _innercanclose &&
                                fnestings[fnestingsdepth - 1] ==
                                    _innerfmt &&
                                (inside_linktitle_ends_at <= 0 ||
                                fnestingsdepth >
                                inside_linktitle_minimum_fnesting)) {
                            fnestingsdepth--;
                            if (fnestingsdepth <= previousnesting) {
                                foundpastidx = i2 + (
                                    _md2html_fmt_type_len(_innerfmt)
                                );
                                foundendinline = iline2;
                                break;
                            }
                            i2 += _md2html_fmt_type_len(_innerfmt);
                            continue;
                        } else if (_innerfmt > 0 && !incode &&
                                _innercanopen &&
                                fnestingsdepth + 1 <
                                _S3D_MD_MAX_FORMAT_NESTING) {
                            fnestingsdepth++;
                            fnestings[fnestingsdepth - 1] = _innerfmt;
                            i2 += _md2html_fmt_type_len(_innerfmt);
                            continue;
                        }
                        i2 += 1;
                    }
                    if (foundpastidx >= 0)
                        break;
                    iline2 += 1;
                    if (iline2 <= endline) {
                        i2 = lineinfo[iline2].indentlen;
                        i2pastend = (lineinfo[iline2].indentlen +
                        lineinfo[iline2].indentedcontentlen);
                        linebuf2 = lineinfo[iline2].linestart;
                    }
                }
                if (foundpastidx < 0) {
                    // No matched end, invalid. Throw away.
                    if (!INSC(linebuf[i]))
                        goto errorquit;
                    fnestingsdepth = previousnesting;
                    i += 1;
                    continue;
                }

                // If we got here, found the end, so this formatting
                // is valid. Insert it:
                fnestingsdepth = previousnesting + 1;
                fnestings[fnestingsdepth - 1] = _fmttype;
                if (_fmttype == _FORMAT_TYPE_ASTERISK1) {
                    if (!INS("<em>"))
                        goto errorquit;
                } else if (_fmttype == _FORMAT_TYPE_ASTERISK2 ||
                        _fmttype == _FORMAT_TYPE_UNDERLINE2) {
                    if (!INS("<strong>"))
                        goto errorquit;
                } else if (_fmttype == _FORMAT_TYPE_TILDE2) {
                    if (!INS("<strike>"))
                        goto errorquit;
                } else {
                    assert(0);
                }
                i += _md2html_fmt_type_len(_fmttype);
                continue;
            } else if (fnestingsdepth > 0 && !as_code &&
                    inside_imgtitle_ends_at == 0 &&
                    (_fmttype = _md2html_get_inline_fmt_type(
                        linebuf, ipastend, i,
                        &_fmtcanopen, &_fmtcanclose
                    )) > 0 && _fmtcanclose &&
                    fnestings[fnestingsdepth - 1] == _fmttype &&
                    (inside_linktitle_ends_at <= 0 ||
                    fnestingsdepth >
                    inside_linktitle_minimum_fnesting)) {
                // Close corresponding formatting again.
                fnestingsdepth -= 1;
                if (_fmttype == _FORMAT_TYPE_ASTERISK1) {
                    if (!INS("</em>"))
                        goto errorquit;
                } else if (_fmttype == _FORMAT_TYPE_ASTERISK2 ||
                        _fmttype == _FORMAT_TYPE_UNDERLINE2) {
                    if (!INS("</strong>"))
                        goto errorquit;
                } else if (_fmttype == _FORMAT_TYPE_TILDE2) {
                    if (!INS("</strike>"))
                        goto errorquit;
                } else {
                    assert(0);
                }
                i += _md2html_fmt_type_len(_fmttype);
                continue;
            }
            if (linebuf[i] == '`' && !as_code &&
                    inside_imgtitle_ends_at == 0) {
                if (!INS("<code>"))
                    goto errorquit;
                i += 1;
                while (iline <= endline) {
                    while (i < ipastend &&
                            linebuf[i] != '`') {
                        if (linebuf[i] == '<') {
                            if (!INS("&lt;"))
                                goto errorquit;
                        } else if (linebuf[i] == '>') {
                            if (!INS("&gt;"))
                                goto errorquit;
                        } else if (linebuf[i] == '&') {
                            if (!INS("&amp;"))
                                goto errorquit;
                        } else {
                            if (!INSC(linebuf[i]))
                                goto errorquit;
                        }
                        i += 1;
                    }
                    if (i < ipastend && linebuf[i] == '`') {
                        if (i < ipastend && linebuf[i] == '`')
                            i += 1;
                        break;
                    }
                    iline += 1;
                    if (iline <= endline) {
                        if (!INS(" "))
                            goto errorquit;
                        ipastend = (
                            lineinfo[iline].indentedcontentlen +
                            lineinfo[iline].indentlen
                        );
                        linebuf = lineinfo[iline].linestart;
                        i = lineinfo[iline].indentlen;
                    }
                }
                if (!INS("</code>"))
                    goto errorquit;
                if (iline > endline)
                    break;
                continue;
            } else if (linebuf[i] == '\\' && !as_code) {
                if (i < ipastend) {
                    i += 1;
                    if (inside_linktitle_ends_at > 0 &&
                            i >= inside_linktitle_ends_at)
                        goto linkend;
                    if (inside_imgtitle_ends_at > 0 &&
                            i >= inside_imgtitle_ends_at)
                        goto imgend;
                    if (linebuf[i] == '<') {
                        if (!INS("&lt;"))
                            goto errorquit;
                    } else if (linebuf[i] == '>') {
                        if (!INS("&gt;"))
                            goto errorquit;
                    } else if (linebuf[i] == '&') {
                        if (!INS("&amp;"))
                            goto errorquit;
                    } else {
                        if (linebuf[i] != '[' && linebuf[i] != '(' &&
                                linebuf[i] != '#' && linebuf[i] != '|')
                            if (!INSC('\\'))
                                goto errorquit;
                        if (!INSC(linebuf[i]))
                            goto errorquit;
                    }
                }
            } else if ((linebuf[i] == '!' && !as_code &&
                    inside_imgtitle_ends_at == 0 &&
                    i + 1 < ipastend &&
                    linebuf[i + 1] == '[') || (
                    linebuf[i] == '[' &&
                    inside_imgtitle_ends_at == 0 &&
                    inside_linktitle_ends_at == 0)) {
                int linkstarti = i;
                int isimage = (linebuf[i] == '!');
                int title_start, title_len;
                int url_start, url_len;
                int prefix_url_linebreak_to_keep_formatting = 0;
                int imgwidth, imgheight;
                char imgwidthformat, imgheightformat;
                size_t linklen = (
                    _internal_spew3dweb_markdown_GetLinkOrImgLen(
                        linebuf, ipastend, i, 1,
                        &title_start, &title_len,
                        &url_start, &url_len,
                        &prefix_url_linebreak_to_keep_formatting,
                        &imgwidth, &imgwidthformat,
                        &imgheight, &imgheightformat
                    ));
                if (linklen <= 0 || (!isimage &&
                        title_len == 0)) {
                    if (!INSC(linebuf[i]))
                        goto errorquit;
                    if (linebuf[i] == '!') {
                        i += 1;
                        if (!INSC(linebuf[i]))
                            goto errorquit;
                    }
                    i += 1;
                    continue;
                } else {
                    if (isimage) {
                        if (!INS("<img"))
                            goto errorquit;
                        char numbuf[16];
                        if (imgwidthformat != '\0') {
                            if (!INS(" width='"))
                                goto errorquit;
                            snprintf(numbuf, sizeof(numbuf) - 1,
                                "%d", imgwidth);
                            if (!INS(numbuf))
                                goto errorquit;
                            if (imgwidthformat == '%') {
                                if (!INSC('%'))
                                    goto errorquit;
                            } else if (imgwidthformat == 'p') {
                                if (!INS("px"))
                                    goto errorquit;
                            } else {
                                assert(0);  // Shouldn't be possible.
                            }
                            if (!INS("'"))
                                goto errorquit;
                        }
                        if (imgheightformat != '\0') {
                            if (!INS(" height='"))
                                goto errorquit;
                            snprintf(numbuf, sizeof(numbuf) - 1,
                                "%d", imgheight);
                            if (!INS(numbuf))
                                goto errorquit;
                            if (imgheightformat == '%') {
                                if (!INSC('%'))
                                    goto errorquit;
                            } else if (imgheightformat == 'p') {
                                if (!INS("px"))
                                    goto errorquit;
                            } else {
                                assert(0);  // Shouldn't be possible.
                            }
                            if (!INS("'"))
                                goto errorquit;
                        }
                        if (!INS(" src='"))
                            goto errorquit;
                    }
                    if (!isimage)
                        if (!INS("<a href='"))
                            goto errorquit;
                    int add_rel_noopener = 0;
                    int add_target_blank = 0;
                    if (_is_guaranteed_ext_link(
                            linebuf + url_start, url_len
                            )) {
                        if (!options->externallinks_no_target_blank)
                            add_target_blank = 1;
                        if (!options->externallinks_no_rel_noopener)
                            add_rel_noopener = 1;
                    }
                    if (!INSBUF(linebuf + url_start,
                            url_len))
                        goto errorquit;
                    if (isimage && title_len == 0) {
                        // Done already.
                        if (!INS("'/>"))
                            goto errorquit;
                        i += linklen;
                        continue;
                    } else {
                        if (isimage) {
                            if (!INS("' alt='"))
                                goto errorquit;
                            i = title_start;
                            inside_imgtitle_ends_at = (
                                title_start + title_len
                            );
                            past_image_idx = linkstarti + linklen;
                        } else {
                            inside_linktitle_ends_at = (
                                title_start + title_len
                            );
                            inside_linktitle_minimum_fnesting = (
                                fnestingsdepth
                            );
                            i = title_start;
                            past_link_idx = linkstarti + linklen;
                            if (!INS("'"))
                                goto errorquit;
                            if (add_rel_noopener)
                                if (!INS(" rel=noopener"))
                                    goto errorquit;
                            if (add_target_blank)
                                if (!INS(" target=_blank"))
                                    goto errorquit;
                            if (!INS(">"))
                                goto errorquit;
                        }
                        continue;
                    }
                }
            } else if (linebuf[i] == '<' && as_code) {
                if (!INS("&lt;"))
                    goto errorquit;
                i += 1;
                continue;
            } else if (linebuf[i] == '>' && as_code) {
                if (!INS("&gt;"))
                    goto errorquit;
                i += 1;
                continue;
            } else if (linebuf[i] == '&' && as_code) {
                if (!INS("&amp;"))
                    goto errorquit;
                i += 1;
                continue;
            }
            if (!INSC(linebuf[i]))
                goto errorquit;
            i += 1;
        }
        iline += 1;
    }
    *resultchunkptr = resultchunk;
    *resultfillptr = resultfill;
    *resultallocptr = resultalloc;
    if (out_endlineidx)
        *out_endlineidx = iline - 1;
    return 1;
}

#undef _FORMAT_TYPE_ASTERISK1
#undef _FORMAT_TYPE_ASTERISK2
#undef _FORMAT_TYPE_UNDERLINE2
#undef _FORMAT_TYPE_TILDE2

S3DEXP char *spew3dweb_markdown_MarkdownBytesToAnchor(
        const char *bytebuf, size_t bytebuflen
        ) {
    int resultalloc = 64;
    int resultfill = 0;
    char *result = malloc(resultalloc);
    if (!result)
        return NULL;
    size_t i = 0;
    while (i < bytebuflen) {
        if (resultfill + 2 >= resultalloc) {
            char *newresult = realloc(result, resultalloc * 2);
            if (!newresult) {
                free(result);
                return NULL;
            }
            resultalloc *= 2;
        }
        if (bytebuf[i] == ' ' || bytebuf[i] < 32 ||
                bytebuf[i] == '>' || bytebuf[i] == '<' ||
                bytebuf[i] == '(' || bytebuf[i] == ')' ||
                bytebuf[i] == '[' || bytebuf[i] == ']' ||
                bytebuf[i] == ',' || bytebuf[i] == ';' ||
                bytebuf[i] == ':' || bytebuf[i] == '.' ||
                bytebuf[i] == '\r' || bytebuf[i] == '/' || (
                bytebuf[i] == '\\' && i + 1 < bytebuflen &&
                bytebuf[i + 1] == '\\')) {
            if (resultfill > 0 && result[resultfill - 1] != '-') {
                result[resultfill] = '-';
                resultfill += 1;
            }
            i += 1;
            continue;
        } else if (bytebuf[i] == '\\' || (bytebuf[i] < 127 &&
                (bytebuf[i] < 'a' || bytebuf[i] > 'z') &&
                (bytebuf[i] < 'A' || bytebuf[i] > 'Z') &&
                (bytebuf[i] < '0' || bytebuf[i] > '9'))) {
            i += 1;
            continue;
        }
        result[resultfill] = bytebuf[i];
        resultfill += 1;
        i += 1;
    }
    while (resultfill > 0 &&
            result[resultfill - 1] == '-')
        resultfill--;
    result[resultfill] = '\0';

    char *lowercaseresult = malloc(
        resultfill * UTF8_CP_MAX_BYTES + 1
    );
    int iorig = 0;
    int ilower = 0;
    while (iorig < resultfill) {
        int out_origlen = 0;
        int out_lowerlen = 0;
        utf8_char_to_lowercase(
            result + iorig, (resultfill - iorig),
            &out_origlen,
            &out_lowerlen, lowercaseresult + ilower
        );
        iorig += out_origlen;
        ilower += out_lowerlen;
    }
    lowercaseresult[ilower] = '\0';
    free(result);
    return lowercaseresult;
}

static int _line_start_like_list_entry(
        _markdown_lineinfo *lineinfo, int lineindex,
        int *out_numentrylen, int *out_number
        ) {
    int _numentryvalue = 0;
    int _numentrylen = 0;
    int i = lineindex;
    if (lineinfo[i].indentedcontentlen >= 2 &&
            (((
            lineinfo[i].linestart[lineinfo[i].indentlen] == '-' ||
            lineinfo[i].linestart[lineinfo[i].indentlen] == '>' ||
            lineinfo[i].linestart[lineinfo[i].indentlen] == '*') &&
            lineinfo[i].linestart[lineinfo[i].indentlen + 1] == ' ') ||
            (_numentrylen = _m2htmlline_start_list_number_len(
                lineinfo, i, &_numentryvalue
            )) > 0
            )) {
        if (out_number) *out_number = _numentryvalue;
        if (out_numentrylen) *out_numentrylen = _numentrylen;
        return 1;
    }
    return 0;
}

S3DEXP char *spew3dweb_markdown_ByteBufToHTML(
        const char *uncleaninput, size_t uncleaninputlen,
        s3dw_markdown_tohtmloptions *options,
        size_t *out_len
        ) {
    // First, clean up the input:
    size_t inputlen = 0;
    char *input = _internal_spew3dweb_markdown_CleanByteBufEx(
        uncleaninput, uncleaninputlen,
        1, 1, !options->block_unsafe_html, 1,
        options->uritransform_callback,
        options->uritransform_callback_userdata,
        &inputlen, NULL
    );
    if (!input)
        return NULL;

    // First, extract info about each line in the markdown:
    size_t lineinfofill = 0;
    size_t lineinfoalloc = 12;
    int lineinfoheap = 0;
    _markdown_lineinfo _lineinfo_staticbuf[12];
    _markdown_lineinfo *lineinfo = (void *)_lineinfo_staticbuf;
    size_t currentlinestart = 0;
    ssize_t currentlinecontentstart = -1;
    size_t currentindent = 0;
    size_t i = 0;
    while (i <= inputlen) {
        if (i >= inputlen || input[i] == '\n' || input[i] == '\r') {
            // Line end. Register this line's info:
            if (lineinfofill + 2 > lineinfoalloc) {
                size_t newalloc = lineinfofill * 2 + 1;
                if (newalloc < 512)
                    newalloc = 512;
                _markdown_lineinfo *newlineinfo = NULL;
                if (lineinfoheap) {
                    newlineinfo = realloc(
                        lineinfo, sizeof(*lineinfo) * newalloc
                    );
                } else {
                    newlineinfo = malloc(
                        sizeof(*lineinfo) * newalloc
                    );
                    if (newlineinfo)
                        memcpy(newlineinfo, lineinfo,
                            sizeof(*lineinfo) * lineinfoalloc);
                }
                if (!newlineinfo) {
                    if (lineinfoheap)
                        free(lineinfo);
                    free(input);
                    return NULL;
                }
                lineinfo = newlineinfo;
                lineinfoheap = 1;
                lineinfoalloc = newalloc;
            }
            lineinfo[lineinfofill].linestart = (
                input + currentlinestart
            );
            lineinfo[lineinfofill].indentedcontentlen = (
                i - currentlinestart
            );
            if (currentlinecontentstart < 0) {
                lineinfo[lineinfofill].indentlen = 0;
            } else {
                lineinfo[lineinfofill].indentlen = (
                    currentindent
                );
                lineinfo[lineinfofill].indentedcontentlen -= (
                    lineinfo[lineinfofill].indentlen
                );
            }
            /*{
                int len = lineinfo[lineinfofill].indentlen +
                    lineinfo[lineinfofill].indentedcontentlen;
                if (len > 127) len = 127;
                char lineb[128];
                memcpy(lineb, lineinfo[lineinfofill].linestart, len);
                lineb[len] = '\0';
                printf("spew3d_markdown.h: debug: "
                    "spew3dweb_markdown_ByteBufToHTML() "
                    "precomputed line (indent %d): %s\n",
                    lineinfo[lineinfofill].indentlen, lineb);
            }*/
            lineinfofill += 1;

            // Start next line, if any:
            if (i >= inputlen)
                break;
            if (input[i] == '\r' && i + 1 < inputlen &&
                    input[i] == '\n')
                i += 1;
            i += 1;
            currentlinestart = i;
            currentlinecontentstart = -1;
            currentindent = 0;
            continue;
        }
        if (currentlinecontentstart < 0 && (
                input[i] == ' ' || input[i] == '\t'))
            currentindent += 1;
        if (currentlinecontentstart < 0 &&
                input[i] != '\t' && input[i] != ' ') {
            // Remember where actual contents of this line started:
            currentlinecontentstart = i;
            i += 1;
            continue;
        }
        i += 1;
    }
    assert(lineinfofill < lineinfoalloc);
    char zerostringbuf[1] = "";
    lineinfo[lineinfofill].indentedcontentlen = 0;
    lineinfo[lineinfofill].indentlen = 0;
    lineinfo[lineinfofill].linestart = zerostringbuf;

    // Now process the markdown and spit out HTML:
    char *resultchunk = NULL;
    size_t resultfill = 0;
    size_t resultalloc = 0;
    if (!_internal_s3dw_markdown_ensurebufsize(
            &resultchunk, &resultalloc, 1
            )) {
        if (lineinfoheap)
            free(lineinfo);
        free(input);
        return NULL;
    }
    int nestingstypes[_S3D_MD_MAX_LIST_NESTING] = {0};
    int nestingsdepth = 0;
    int insidecodeindent = -1;
    int lastnonemptynoncodeindent = 0;
    int enteredlistinlineidx = -1;
    i = 0;
    while (i <= lineinfofill) {
        /*{
            char lineb[2048];
            size_t copylen = (lineinfo[i].indentlen +
                lineinfo[i].indentedcontentlen);
            if (copylen >= sizeof(lineb)) copylen = sizeof(lineb);
            memcpy(lineb, lineinfo[i].linestart, copylen);
            lineb[copylen] = '\0';
            printf("spew3d_markdown.h: debug: "
                "spew3dweb_markdown_ByteBufToHTML() at line %d ("
                "indent %d): '%s'\n",
                i, lineinfo[i].indentlen, lineb);
        }*/
        char currentlinefoundbullet = '\0';
        int currentlineindentafterbullet = 0;
        int enteredlistinthisline = (
            i == enteredlistinlineidx
        );

        // First, figure out some basics on our indent:
        int currentlookslikelistno = 0;
        int currentlookslikelistnolen = 0;
        int currentlookslikelist = 0;
        int currentlookslikeinnerindent = (
            lineinfo[i].indentlen
        );
        assert(currentlookslikeinnerindent >= 0);
        if (!enteredlistinthisline) {
            currentlookslikelist = _line_start_like_list_entry(
                lineinfo, i, &currentlookslikelistnolen,
                &currentlookslikelistno
            );
            if (insidecodeindent > 0 &&
                    lineinfo[i].indentlen >= insidecodeindent) {
                currentlookslikelist = 0;
                currentlookslikelistno = 0;
                currentlookslikelistnolen = 0;
            }
            currentlookslikeinnerindent += (
                (!currentlookslikelist) ? 0 : (
                    (currentlookslikelistnolen > 0) ? 4 : 2)
            );
            assert(currentlookslikeinnerindent >= 0);
        } else {
            currentlookslikelist = 1;
        }

        if ((currentlookslikeinnerindent <= lastnonemptynoncodeindent - 3 ||
                (insidecodeindent > 0 &&
                lineinfo[i].indentlen <= insidecodeindent - 4)) &&
                (lineinfo[i].indentedcontentlen > 0 ||
                i == lineinfofill)) {
            // We're leaving a higher nesting, either list or code.
            int referenceindent = lineinfo[i].indentlen;
            if (insidecodeindent >= 0) {
                if (!INS("</code></pre>\n"))
                    goto errorquit;
                referenceindent = insidecodeindent - 4;
                insidecodeindent = -1;
            }
            int nestreduce = (lastnonemptynoncodeindent -
                referenceindent) / 4;
            while (nestreduce > 0) {
                assert(nestingsdepth >= nestreduce);
                const int di = nestingsdepth - 1;
                if (nestingstypes[di] != '>' &&
                        nestingstypes[di] != '1') {
                    if (!INS("</li></ul>\n")) {
                        errorquit: ;
                        if (lineinfoheap)
                            free(lineinfo);
                        free(input);
                        return NULL;
                    }
                } else if (nestingstypes[di] == '1') {
                    if (!INS("</li></ol>\n"))
                        goto errorquit;
                } else if (nestingstypes[di] == '>') {
                    if (!INS("</blockquote>\n"))
                        goto errorquit;
                } else {
                    assert(0);  // Should never happen.
                }
                nestreduce -= 1;
                nestingsdepth -= 1;
            }
        }
        int potentialtablecells = 0;
        if (insidecodeindent < 0 && i < lineinfofill) {
            // Check for everything allowed outside of a code block:
            if (lineinfo[i].indentlen >=
                        lastnonemptynoncodeindent + 4 &&
                    insidecodeindent < 0) {
                // Start of 4 space code block!
                insidecodeindent = lastnonemptynoncodeindent + 4;
                if (!INS("<pre><code>"))
                    goto errorquit;
                if (!INSBUF(
                        lineinfo[i].linestart + lineinfo[i].indentlen,
                        lineinfo[i].indentedcontentlen))
                    goto errorquit;
                if (!INS("\n"))
                    goto errorquit;
                i += 1;
                continue;
            } else if (lineinfo[i].indentedcontentlen >= 3 &&
                    lineinfo[i].linestart[
                    lineinfo[i].indentlen] == '`' &&
                    lineinfo[i].linestart[
                    lineinfo[i].indentlen + 1] == '`' &&
                    lineinfo[i].linestart[
                    lineinfo[i].indentlen + 2] == '`') {
                // This is a special fenced code block:
                int baseindent = lineinfo[i].indentlen;
                int j = lineinfo[i].indentlen + 3;
                int ticks = 3;
                while (j < lineinfo[i].indentedcontentlen &&
                        lineinfo[i].linestart[j] == '`') {
                    ticks += 1;
                    j += 1;
                }
                int langnamelen = (
                    spew3dweb_markdown_GetBacktickByteBufLangPrefixLen(
                        lineinfo[i].linestart,
                        lineinfo[i].indentlen +
                        lineinfo[i].indentedcontentlen, j
                    ));
                if (!INS("<pre><code"))
                    goto errorquit;
                if (langnamelen > 0) {
                    if (!INS(" lang='"))
                        goto errorquit;
                    int jend = j + langnamelen;
                    while (j < jend) {
                        if (lineinfo[i].linestart[j] == '&') {
                            if (!INS("&amp;"))
                                goto errorquit;
                        } else if (lineinfo[i].linestart[j] != '\'') {
                            if (!INSC(lineinfo[i].linestart[j]))
                                goto errorquit;
                        }
                        j += 1;
                    }
                    if (!INS("'>"))
                        goto errorquit;
                } else {
                    if (!INS(">"))
                        goto errorquit;
                }
                i += 1;
                while (i < lineinfofill) {
                    j = lineinfo[i].indentlen;
                    int _foundticks = 0;
                    while (j < lineinfo[i].indentlen +
                            lineinfo[i].indentedcontentlen &&
                            lineinfo[i].linestart[j] == '`') {
                        _foundticks += 1;
                        j += 1;
                    }
                    if (_foundticks >= ticks) {
                        i += 1;
                        break;
                    } else {
                        // Add indent of this line:
                        int incodeindent = (
                            lineinfo[i].indentlen -
                            baseindent);
                        if (incodeindent < 0)
                            incodeindent = 0;
                        if (!INSREP(" ", incodeindent))
                            goto errorquit;
                        // Add in the contents of this line:
                        int endlineidx = -1;
                        if (!_spew3d_markdown_process_inline_content(
                                &resultchunk, &resultfill, &resultalloc,
                                lineinfo, lineinfofill, i, i, 0, -1,
                                1, 0, options,
                                &endlineidx))
                            goto errorquit;
                        assert(endlineidx == i);
                        if (!INS("\n"))
                            goto errorquit;
                    }
                    i += 1;
                }
                if (!INS("</code></pre>"))
                    goto errorquit;
                continue;
            } else if (!enteredlistinthisline && currentlookslikelist) {
                // Start of a list entry!
                char bullettype = (
                    (currentlookslikelistnolen > 0 ? '1' :
                    lineinfo[i].linestart[lineinfo[i].indentlen])
                );
                currentlinefoundbullet = bullettype;
                int listbasenesting = (
                    (bullettype == '1' ? (
                        (lineinfo[i].indentlen / 4) + 1
                    ) : ((lineinfo[i].indentlen - 2) / 4) + 1)
                );
                currentlineindentafterbullet = (
                    lineinfo[i].indentlen + 2 + (
                        bullettype == '1' ? 2 : 0));
                // Previous code should have descended out of nested lists:
                assert(nestingsdepth <= listbasenesting);
                lastnonemptynoncodeindent = (
                    currentlineindentafterbullet  // (indent of actual list)
                );
                if (listbasenesting > nestingsdepth &&
                        nestingsdepth + 1 < _S3D_MD_MAX_LIST_NESTING) {
                    nestingsdepth += 1;
                    assert(nestingsdepth >= listbasenesting);
                    enteredlistinthisline = 1;
                    enteredlistinlineidx = i;
                    assert(bullettype != 0);
                    nestingstypes[nestingsdepth - 1] = bullettype;
                    if (bullettype == '>') {
                        if (!INS("<blockquote>"))
                            goto errorquit;
                    } else if (bullettype == '1') {
                        if (!INS("<ol start="))
                            goto errorquit;
                        char startval[16];
                        snprintf(startval, sizeof(startval) - 1,
                            "%d", currentlookslikelistno);
                        if (!INS(startval))
                            goto errorquit;
                        if (!INS(">"))
                            goto errorquit;
                    } else {
                        if (!INS("<ul>"))
                            goto errorquit;
                    }
                }
                if (bullettype != '>') {
                    if (!enteredlistinthisline)
                        if (!INS("</li>"))
                            goto errorquit;
                    if (!INS("<li>"))
                        goto errorquit;
                }
                int oldindentlen = lineinfo[i].indentlen;
                assert(oldindentlen <= currentlineindentafterbullet);
                lineinfo[i].indentlen = currentlineindentafterbullet;
                lineinfo[i].indentedcontentlen -= (
                    currentlineindentafterbullet - oldindentlen
                );
                // No i += 1 increase here! We want to process
                // the line again for list item content:
                continue;
            } else if (lineinfo[i].indentedcontentlen >= 3 &&
                    lineinfo[i].linestart[
                        lineinfo[i].indentlen] == '#' && (
                    lineinfo[i].linestart[
                        lineinfo[i].indentlen + 1] == ' ' ||
                    lineinfo[i].linestart[
                        lineinfo[i].indentlen + 2] == '#')) {
                // Check if this is a heading:
                int headingtype = 1;
                size_t i2 = lineinfo[i].indentlen + 1;
                while (i2 < lineinfo[i].indentedcontentlen &&
                        lineinfo[i].linestart[i2] == '#') {
                    headingtype += 1;
                    i2 += 1;
                }
                if (headingtype > 6) headingtype = 6;
                if (i2 < lineinfo[i].indentlen +
                        lineinfo[i].indentedcontentlen && (
                        lineinfo[i].linestart[i2] == ' ' ||
                        lineinfo[i].linestart[i2] == '\t')) {
                    while (i2 < lineinfo[i].indentlen +
                            lineinfo[i].indentedcontentlen && (
                            lineinfo[i].linestart[i2] == ' ' ||
                            lineinfo[i].linestart[i2] == '\t')) {
                        i2 += 1;
                    }
                    if (i2 < lineinfo[i].indentlen +
                            lineinfo[i].indentedcontentlen &&
                            lineinfo[i].linestart[i2] != '#') {
                        // This is indeed a heading. Process insides:
                        int doanchor = 0;
                        if (!options->disable_heading_anchors) {
                            doanchor = !_m2html_check_line_has_link(
                                lineinfo, i, 0
                            );
                        }
                        if (!INS("<h"))
                            goto errorquit;
                        if (!INSC('0' + headingtype))
                            goto errorquit;
                        if (!INS(">"))
                            goto errorquit;
                        if (doanchor) {
                            char *name = (
                                spew3dweb_markdown_MarkdownBytesToAnchor(
                                    lineinfo[i].linestart + i2,
                                    (lineinfo[i].indentlen +
                                    lineinfo[i].indentedcontentlen) - i2
                                ));
                            if (!name)
                                goto errorquit;
                            if (!INS("<a name='")) {
                                free(name);
                                goto errorquit;
                            }
                            if (!INS(name)) {
                                free(name);
                                goto errorquit;
                            }
                            if (!INS("' href='#")) {
                                free(name);
                                goto errorquit;
                            }
                            if (!INS(name)) {
                                free(name);
                                goto errorquit;
                            }
                            free(name);
                            if (!INS("'>"))
                                goto errorquit;
                        }
                        int endlineidx = -1;
                        if (!_spew3d_markdown_process_inline_content(
                                &resultchunk, &resultfill, &resultalloc,
                                lineinfo, lineinfofill, i, i + 1,
                                i2, -1, 0, 1, options, &endlineidx))
                            goto errorquit;
                        if (doanchor) {
                            if (!INS("</a>"))
                                goto errorquit;
                        }
                        if (!INS("</h"))
                            goto errorquit;
                        if (!INSC('0' + headingtype))
                            goto errorquit;
                        if (!INS(">\n"))
                            goto errorquit;
                        assert(endlineidx == i);
                        i += 1;
                        continue;
                    }
                }
            } else if (_internal_s3dw_markdown_LineStartsTable(
                    lineinfo, i, lineinfofill,
                    &potentialtablecells
                    )) {
                const int cellcount = potentialtablecells;
                assert(i + 1 < lineinfofill);
                assert(cellcount > 0);
                if (!INS("<table>"))
                    goto errorquit;
                int firstrowidx = i;
                int mustskipidx = i + 1;
                while (i < lineinfofill && (
                        i <= mustskipidx ||
                        _internal_s3dw_markdown_LineContinuesTable(
                        lineinfo, i, lineinfofill, cellcount))) {
                    if (i == mustskipidx) {
                        i += 1;
                        continue;
                    }
                    if (!INS("\n<tr>"))
                        goto errorquit;
                    int cellidx = 0;
                    while (cellidx < cellcount) {
                        int cell_start, cell_len;
                        _internal_s3dw_markdown_GetTableCell(
                            lineinfo, i, lineinfofill, cellidx + 1,
                            &cell_start, &cell_len
                        );
                        if (i == firstrowidx) {
                            if (!INS("<th>"))
                                goto errorquit;
                        } else {
                            if (!INS("<td>"))
                                goto errorquit;
                        }
                        if (cell_len == 0) {
                            cellidx += 1;
                            continue;
                        }
                        int endlineidx;
                        if (!_spew3d_markdown_process_inline_content(
                                &resultchunk, &resultfill, &resultalloc,
                                lineinfo, lineinfofill, i, i,
                                cell_start, cell_start + cell_len,
                                0, 1, options, &endlineidx))
                            goto errorquit;
                        assert(endlineidx == i);
                        if (i == firstrowidx) {
                            if (!INS("</th>"))
                                goto errorquit;
                        } else {
                            if (!INS("</td>"))
                                goto errorquit;
                        }
                        cellidx += 1;
                    }
                    if (!INS("</tr>"))
                        goto errorquit;
                    i += 1;
                }
                if (!INS("\n</table>\n"))
                    goto errorquit;
                continue;
            }
        }
        if (insidecodeindent >= 0) {
            assert(i < lineinfofill);
            // First, handle the indent but relative to the code base:
            int actualindent = (lineinfo[i].indentlen -
                insidecodeindent);
            if (actualindent < 0)
                actualindent = 0;
            if (!INSREP(" ", actualindent))
                goto errorquit;
            // Copy single line with no unnecessary formatting changes:
            int endlineidx = -1;
            if (!_spew3d_markdown_process_inline_content(
                    &resultchunk, &resultfill, &resultalloc,
                    lineinfo, lineinfofill, i, i + 1, 0, -1,
                    1 /* as code, no formatting */,
                    0, options, &endlineidx))
                goto errorquit;
            if (!INS("\n"))
                goto errorquit;
        } else if (insidecodeindent < 0 && i < lineinfofill &&
                lineinfo[i].indentedcontentlen > 0) {
            // Add in regular inline content:
            lastnonemptynoncodeindent = lineinfo[i].indentlen;
            int headingtype = (
                _m2htmlline_line_get_next_line_heading_strength(
                    lineinfo, lineinfofill, i
                ));
            int doanchor = 0;
            if (headingtype > 0) {
                if (!options->disable_heading_anchors)
                    doanchor = !_m2html_check_line_has_link(
                        lineinfo, i, 0
                    );
                if (!INS("<h"))
                    goto errorquit;
                if (!INSC('0' + headingtype))
                    goto errorquit;
                if (!INS(">"))
                    goto errorquit;
                if (doanchor) {
                    char *name = (
                        spew3dweb_markdown_MarkdownBytesToAnchor(
                            lineinfo[i].linestart,
                            (lineinfo[i].indentlen +
                            lineinfo[i].indentedcontentlen)
                        ));
                    if (!name)
                        goto errorquit;
                    if (!INS("<a name='")) {
                        free(name);
                        goto errorquit;
                    }
                    if (!INS(name)) {
                        free(name);
                        goto errorquit;
                    }
                    if (!INS("' href='#")) {
                        free(name);
                        goto errorquit;
                    }
                    if (!INS(name)) {
                        free(name);
                        goto errorquit;
                    }
                    free(name);
                    if (!INS("'>"))
                        goto errorquit;
                }
            } else {
                if (!INS("<p>"))
                    goto errorquit;
            }
            int endlineidx = -1;
            if (!_spew3d_markdown_process_inline_content(
                    &resultchunk, &resultfill, &resultalloc,
                    lineinfo, lineinfofill, i, lineinfofill,
                    0, -1,
                    0, (headingtype != 0), options,
                    &endlineidx))
                goto errorquit;
            if (headingtype != 0) {
                assert(endlineidx == i);
                if (doanchor) {
                    if (!INS("</a>"))
                        goto errorquit;
                }
                if (!INS("</h"))
                    goto errorquit;
                if (!INSC('0' + headingtype))
                    goto errorquit;
                if (!INS(">\n"))
                    goto errorquit;
                i = endlineidx + 2;  // Skip past underline
                continue;
            } else {
                i = endlineidx;
                if (!INS("</p>\n"))
                    goto errorquit;
            }
        }
        i += 1;
    }
    if (lineinfoheap)
        free(lineinfo);
    free(input);
    resultchunk[resultfill] = '\0';
    if (out_len) *out_len = resultfill;
    return resultchunk;
}

#undef INSC
#undef INS
#undef INSREP
#undef INSBUF

S3DEXP char *spew3dweb_markdown_ToHTMLEx(
        const char *uncleaninput,
        s3dw_markdown_tohtmloptions *options,
        size_t *out_len
        ) {
    return spew3dweb_markdown_ByteBufToHTML(
        uncleaninput, strlen(uncleaninput),
        options,
        out_len
    );
}

S3DEXP char *spew3dweb_markdown_ToHTML(
        const char *uncleaninput
        ) {
    s3dw_markdown_tohtmloptions options = {0};
    return spew3dweb_markdown_ByteBufToHTML(
        uncleaninput, strlen(uncleaninput),
        &options, NULL
    );
}

#endif  // SPEW3DWEB_IMPLEMENTATION

