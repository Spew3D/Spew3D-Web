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

S3DHID int _internal_s3dw_markdown_LineStartsTable(
        _markdown_lineinfo *lineinfo, size_t linei,
        size_t linefill, int *out_cells
        ) {
    size_t i = lineinfo[linei].indentlen;
    size_t len = (lineinfo[linei].indentlen +
        lineinfo[linei].indentedcontentlen);
    int startswithpipe = (
        i < len && lineinfo[linei].linestart[i] == '|'
    );
    char lastnonwhitespacechar = '\0';
    int foundnotpipe = 0;
    int pipecount = 0;
    while (i < len) {
        if (lineinfo[linei].linestart[i] == '|') {
            pipecount += 1;
            lastnonwhitespacechar = '|';
        } else {
            foundnotpipe = 1;
            if (lineinfo[linei].linestart[i] != ' ' &&
                    lineinfo[linei].linestart[i] != '\t')
                lastnonwhitespacechar = (
                    lineinfo[linei].linestart[i]
                );
        }
        i += 1;
    }
    if (!foundnotpipe || pipecount < 2 || !startswithpipe ||
            i < len || lastnonwhitespacechar != '|')
        return 0;
    int cells = (pipecount - 1);
    linei += 1;
    if (linei >= linefill)
        return 0;

    i = lineinfo[linei].indentlen;
    len = (lineinfo[linei].indentlen +
        lineinfo[linei].indentedcontentlen);
    pipecount = 0;
    int dashcount = 0;
    while (i < len) {
        if (lineinfo[linei].linestart[i] == '|') {
            pipecount += 1;
        } else if (lineinfo[linei].linestart[i] == '-') {
            dashcount += 1;
        } else {
            break;
        }
        i += 1;
    }
    while (i < len && (
            lineinfo[linei].linestart[i] == ' ' ||
            lineinfo[linei].linestart[i] == '\t'))
        i += 1;
    if (dashcount < 1 || pipecount < 2 || i < len ||
            lineinfo[linei].indentlen !=
            lineinfo[linei - 1].indentlen)
        return 0;
    if (out_cells) *out_cells = cells;
    return 1;
}

S3DHID int _internal_s3dw_markdown_LineContinuesTable(
        _markdown_lineinfo *lineinfo, size_t linei,
        size_t linefill, int cells
        ) {
    size_t i = lineinfo[linei].indentlen;
    size_t len = (lineinfo[linei].indentlen +
        lineinfo[linei].indentedcontentlen);
    char lastnonwhitespacechar = '\0';
    int startswithpipe = (
        i < len && lineinfo[linei].linestart[i] == '|'
    );
    int foundnotpipe = 0;
    int pipecount = 0;
    while (i < len) {
        if (lineinfo[linei].linestart[i] == '|') {
            pipecount += 1;
            lastnonwhitespacechar = '|';
        } else {
            foundnotpipe = 1;
            if (lineinfo[linei].linestart[i] != '\t' &&
                    lineinfo[linei].linestart[i] != ' ')
                lastnonwhitespacechar = (
                    lineinfo[linei].linestart[i]
                );
        }
        i += 1;
    }
    if (!startswithpipe || foundnotpipe <= 0 ||
            pipecount != cells + 1 ||
            lastnonwhitespacechar != '|')
        return 0;
    return 1;
}

S3DHID void _internal_s3dw_markdown_GetTableCell(
        _markdown_lineinfo *lineinfo, size_t linei,
        size_t linefill, int cell_no,
        int *out_startoffset, int *out_byteslen
        ) {
    size_t i = lineinfo[linei].indentlen;
    size_t len = (lineinfo[linei].indentlen +
        lineinfo[linei].indentedcontentlen);
    int startswithpipe = (
        i < len && lineinfo[linei].linestart[i] == '|'
    );
    int foundnotpipe = 0;
    int pipecount = 0;
    while (i < len) {
        if (lineinfo[linei].linestart[i] == '|') {
            pipecount += 1;
            if (pipecount == cell_no) {
                i += 1;
                while (i < len &&
                        (lineinfo[linei].linestart[i] == ' ' ||
                        lineinfo[linei].linestart[i] == '\t'))
                    i += 1;
                int starti = i;
                while (i < len &&
                        lineinfo[linei].linestart[i] != '|')
                    i += 1;
                if (i < len) {
                    while (i > starti &&
                            (lineinfo[linei].linestart[i - 1] == ' ' ||
                            lineinfo[linei].linestart[i - 1] == '\t'))
                        i -= 1;
                    if (out_startoffset)
                        *out_startoffset = starti;
                    if (out_byteslen)
                        *out_byteslen = i - starti;
                    return;
                }
            }
        } else {
            foundnotpipe = 1;
        }
        i += 1;
    }
    if (out_startoffset) *out_startoffset = 0;
    if (out_byteslen) *out_byteslen = 0;
}

#endif  // SPEW3DWEB_IMPLEMENTATION

