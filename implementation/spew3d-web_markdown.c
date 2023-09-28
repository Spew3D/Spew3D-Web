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

S3DHID char *_internal_spew3dweb_markdown_GetIChunkFromCustomIOEx(
        int (*read_func)(char *buff, size_t amount, void *userdata),
        int (*checkeof_func)(void *userdata),
        int (*seekback_func)(size_t backward_amount, void *userdata),
        void *userdata,
        char *optionalbuf, size_t optionalbufsize,
        size_t opt_maxchunklen, size_t opt_minchunklen,
        size_t *out_len
        ) {
    assert(opt_minchunklen > 0);
    char *readbuf = optionalbuf;
    int readbufheap = 0;
    size_t readbufsize = optionalbufsize;
    size_t readbuffill = 0;
    size_t readsize = 2048;
    if (optionalbufsize <= 1) {
        optionalbuf = NULL;
        readbuf = NULL;
        readbufsize = 0;
    }
    if (opt_maxchunklen < opt_minchunklen)
        opt_minchunklen = opt_maxchunklen;
    if (optionalbufsize >= 20 && optionalbuf != NULL &&
            readsize > optionalbufsize) readsize = optionalbufsize - 1;
    if (readsize > opt_maxchunklen) readsize = opt_maxchunklen;

    // Read bit by bit, and find a good stopping position:
    unsigned int inside_backticks_of_len = 0;
    while (1) {
        if (!readbuf || readbufsize < readbuffill + readsize + 1) {
            // We first need more buffer space to read into
            if (checkeof_func(userdata) && readbuf != NULL) {
                // We reached the end anyway, so just stop.
                readsize = 0;
            } else {
                // Allocate more space:
                size_t new_size = readbufsize * 2;
                if (new_size < readbuffill + readsize * 2 + 1)
                    new_size = readbuffill + readsize * 2 + 1;
                if (new_size < 512) new_size = 512;
                if (new_size > opt_maxchunklen + 1)
                    new_size = opt_maxchunklen + 1;
                char *newreadbuf = realloc(
                    (readbufheap ? readbuf : NULL), new_size
                );
                if (!newreadbuf) {
                    if (readbufheap) free(readbuf);
                    return NULL;
                }
                readbufheap = 1;
                readbuf = newreadbuf;
                readbufsize = new_size;
            }
        }
        int bytes = 0;
        if (readsize > 0)
            bytes = read_func(
                readbuf + readbuffill, readsize, userdata
            );
        assert(bytes <= (int)readsize);
        if (bytes < 0) {
            if (readbufheap) free(readbuf);
            return NULL;
        }
        int waslastread = (bytes <= 0);
        size_t oldfill = readbuffill;
        readbuffill += bytes;
        assert(readbuffill < readbufsize);

        // Always scan to 16 up to the end of what we have,
        // unless we already know we'll have to cut off due
        // to opt_maxchunklen anyway:
        // (Note: this means more than 16 backticks in a row
        // can break if right at the end of what we read.)
        size_t k = oldfill;
        if (k > 16)
            k -= 16;
        else
            k = 0;
        size_t kscanend = readbuffill;
        if (readbuffill < opt_maxchunklen &&
                !waslastread) {  // Won't have to cut off.
            if (kscanend > 16)
                kscanend -= 16;
            else
                kscanend = 0;
        }
        // Do scan and check for back ticks:
        while (k < kscanend) {
            assert(k < readbufsize);
            if (k >= opt_minchunklen &&
                    inside_backticks_of_len == 0) {
                // See if we're at a safe blank line to stop:
                if ((readbuf[k] == '\r' &&
                        k + 1 < readbuffill &&
                        readbuf[k + 1] == '\r') ||
                        (readbuf[k] == '\n' &&
                        k + 1 < readbuffill &&
                        readbuf[k + 1] == '\n') ||
                        (readbuf[k] == '\r' &&
                        k + 3 < readbuffill &&
                        readbuf[k + 1] == '\n' &&
                        readbuf[k + 2] == '\r' &&
                        readbuf[k + 3] == '\n')) {
                    // Check that follow-up line isn't indented:
                    int followup_line_indented = 1;
                    int j = k;
                    while (j < readbufsize) {
                        if (readbuf[j] == '\r' || readbuf[j] == '\n') {
                            j++;
                            continue;
                        }
                        followup_line_indented = (
                            readbuf[j] == ' ' || readbuf[j] == '\t'
                        );
                        break;
                    }

                    if (!followup_line_indented) {
                        // We can stop here.
                        if (!seekback_func(readbuffill - k,
                                userdata)) {
                            if (readbufheap) free(readbuf);
                            return NULL;
                        }
                        readbuf[k] = '\0';
                        *out_len = k;
                        return readbuf;
                    }
                }
            }
            // Track all ``` pairs to not stop inside them:
            if (readbuf[k] == '`' &&
                    k + 2 < readbuffill &&
                    readbuf[k + 1] == '`' &&
                    readbuf[k + 2] == '`') {
                unsigned int tickscount = 3;
                k += 3;
                while (k < readbuffill &&
                        readbuf[k] == '`') {
                    k += 1;
                    tickscount += 1;
                }
                if (tickscount == inside_backticks_of_len) {
                    inside_backticks_of_len = 0;
                } else if (inside_backticks_of_len == 0) {
                    inside_backticks_of_len = tickscount;
                }
                continue;
            }
            k += 1;
        }
        // If we reached the maximum length or end of file, stop:
        if (readbuffill >= opt_maxchunklen || waslastread) {
            assert(readbuffill <= opt_maxchunklen);
            readbuf[readbufsize - 1] = '\0';
            *out_len = readbuffill;
            return readbuf;
        }
        // Read more, but not past our maximum returnable size:
        readsize = 2048;
        if (readsize + readbuffill > opt_maxchunklen)
            readsize = opt_maxchunklen - readbuffill;
    }
}

#define _GETCHUNK_READERMODE_FROMMEM 1
#define _GETCHUNK_READERMODE_FROMVFS 2
#define _GETCHUNK_READERMODE_FROMDISK 3

struct _internal_helper_markdown_getchunk_readerinfo {
    int mode;
    union {
        struct data_frommem {
            size_t size;
            const char *data;
            size_t offset;
        } data_frommem;
        struct data_fromvfs {
            SPEW3DVFS_FILE *f;
        } data_fromvfs;
        struct data_fromdisk {
            FILE *f;
        } data_fromdisk;
    };
};

S3DHID int _internal_helper_markdown_getchunk_checkeof(
        void *userdata
        ) {
    struct _internal_helper_markdown_getchunk_readerinfo *
        info = userdata;
    if (info->mode == _GETCHUNK_READERMODE_FROMMEM) {
        return (info->data_frommem.offset >=
                info->data_frommem.size);
    } else if (info->mode == _GETCHUNK_READERMODE_FROMVFS) {
        return spew3d_vfs_feof(info->data_fromvfs.f);
    } else if (info->mode == _GETCHUNK_READERMODE_FROMDISK) {
        return feof(info->data_fromdisk.f);
    } else {
        assert(0);
    }
    return 0;
}

S3DHID int _internal_helper_markdown_getchunk_seekback(
        size_t amount, void *userdata
        ) {
    assert(amount < INT32_MAX);
    struct _internal_helper_markdown_getchunk_readerinfo *
        info = userdata;
    if (info->mode == _GETCHUNK_READERMODE_FROMMEM) {
        if (amount > info->data_frommem.offset)
            return 0;
        info->data_frommem.offset -= amount;
        return 1;
    } else if (info->mode == _GETCHUNK_READERMODE_FROMVFS) {
        int64_t value = spew3d_vfs_ftell(
            info->data_fromvfs.f
        );
        if (value < 0 || amount > value)
            return 0;
        if (spew3d_vfs_fseek(
                info->data_fromvfs.f, value - amount
                ) < 0) {
            return 0;
        }
        return 1;
    } else if (info->mode == _GETCHUNK_READERMODE_FROMDISK) {
        int64_t value = ftell64(
            info->data_fromdisk.f
        );
        if (value < 0 || amount > value)
            return 0;
        if (fseek64(
                info->data_fromdisk.f, value - amount, SEEK_SET
                ) < 0) {
            return 0;
        }
        return 1;
    } else {
        assert(0);
    }
}

S3DHID int _internal_helper_markdown_getchunk_reader(
        char *write_to, size_t amount, void *userdata
        ) {
    assert(amount <= INT32_MAX);
    struct _internal_helper_markdown_getchunk_readerinfo *
        info = userdata;
    if (info->mode == _GETCHUNK_READERMODE_FROMMEM) {
        assert(info->data_frommem.offset <=
            info->data_frommem.size);
        if (amount + info->data_frommem.offset >
                info->data_frommem.size)
            amount = info->data_frommem.size -
                info->data_frommem.offset;
        if (amount > 0)
            memcpy(write_to, info->data_frommem.data +
                info->data_frommem.offset, amount);
        info->data_frommem.offset += amount;
        return amount;
    } else if (info->mode == _GETCHUNK_READERMODE_FROMVFS) {
        errno = 0;
        size_t amountread = spew3d_vfs_fread(
            write_to, amount, 1, info->data_fromvfs.f
        );
        if (amountread == 0 &&
                spew3d_vfs_ferror(info->data_fromvfs.f))
            return -1;
        return amountread;
    } else if (info->mode == _GETCHUNK_READERMODE_FROMDISK) {
        errno = 0;
        size_t amountread = fread(
            write_to, amount, 1, info->data_fromdisk.f
        );
        if (amountread == 0 && ferror(info->data_fromdisk.f))
            return -1;
        return amountread;
    } else {
        assert(0);
    }
    return -1;
}

/// Note: it makes no sense to use this other than for tests!!
/// (Since if you already have all markdown text in memory,
/// why would you need a memory conserving partial chunk?)
S3DHID char *_internal_spew3dweb_markdown_GetIChunkExFromMem(
        const char *original_buffer, size_t original_buffer_len,
        char *optionalbuf, size_t optionalbufsize,
        size_t opt_maxchunklen, size_t opt_minchunklen,
        size_t *out_len
        ) {
    struct _internal_helper_markdown_getchunk_readerinfo i;
    memset(&i, 0, sizeof(i));
    i.mode = _GETCHUNK_READERMODE_FROMMEM;
    i.data_frommem.data = original_buffer;
    i.data_frommem.size = original_buffer_len;
    char *result = (
        _internal_spew3dweb_markdown_GetIChunkFromCustomIOEx(
            _internal_helper_markdown_getchunk_reader,
            _internal_helper_markdown_getchunk_checkeof,
            _internal_helper_markdown_getchunk_seekback,
            &i, optionalbuf, optionalbufsize,
            opt_maxchunklen, opt_minchunklen,
            out_len
        )
    );
    return result;
}

S3DEXP char *spew3dweb_markdown_GetIChunkFromCustomIO(
        int (*read_func)(char *buff, size_t amount, void *userdata),
        int (*checkeof_func)(void *userdata),
        int (*seekback_func)(size_t backward_amount, void *userdata),
        void *userdata,
        size_t opt_maxchunklen,
        size_t *out_len
        ) {
    return _internal_spew3dweb_markdown_GetIChunkFromCustomIOEx(
        read_func, checkeof_func, seekback_func, userdata,
        NULL, 0,
        opt_maxchunklen, 1024 * 5,
        out_len
    );
}

S3DHID char *_internal_spew3dweb_markdown_GetIChunkFromVFSFileEx(
        SPEW3DVFS_FILE *f,
        char *optionalbuf, size_t optionalbufsize,
        size_t opt_maxchunklen, size_t opt_minchunklen,
        size_t *out_len
        ) {
    struct _internal_helper_markdown_getchunk_readerinfo i;
    memset(&i, 0, sizeof(i));
    i.mode = _GETCHUNK_READERMODE_FROMVFS;
    i.data_fromvfs.f = f;
    char *result = (
        _internal_spew3dweb_markdown_GetIChunkFromCustomIOEx(
            _internal_helper_markdown_getchunk_reader,
            _internal_helper_markdown_getchunk_checkeof,
            _internal_helper_markdown_getchunk_seekback,
            &i, optionalbuf, optionalbufsize,
            opt_maxchunklen, opt_minchunklen,
            out_len
        )
    );
    return result;
}

S3DEXP char *spew3dweb_markdown_GetIChunkFromVFSFile(
        SPEW3DVFS_FILE *f, size_t opt_maxchunklen, size_t *out_len
        ) {
    return _internal_spew3dweb_markdown_GetIChunkFromVFSFileEx(
        f, NULL, 0,
        opt_maxchunklen, 1024 * 5,
        out_len
    );
}

S3DHID char *_internal_spew3dweb_markdown_GetIChunkFromDiskFile(
        FILE *f, size_t opt_maxchunklen, size_t *out_len
        ) {
    struct _internal_helper_markdown_getchunk_readerinfo i;
    memset(&i, 0, sizeof(i));
    i.mode = _GETCHUNK_READERMODE_FROMDISK;
    i.data_fromdisk.f = f;
    char *result = (
        _internal_spew3dweb_markdown_GetIChunkFromCustomIOEx(
            _internal_helper_markdown_getchunk_reader,
            _internal_helper_markdown_getchunk_checkeof,
            _internal_helper_markdown_getchunk_seekback,
            &i, NULL, 0,
            opt_maxchunklen, 1024 * 5,
            out_len
        )
    );
}

static int _ensurebufsize(
        char **bufptr, size_t *bufalloc, size_t new_size
        ) {
    size_t oldalloc = *bufalloc;
    if (!*bufptr || oldalloc < new_size) {
        size_t new_alloc = (oldalloc + (oldalloc / 4) + 256);
        if (new_alloc < new_size)
            new_alloc = new_size;
        char *newbuf = realloc(*bufptr, new_alloc);
        if (!newbuf) {
            if (*bufptr) free(*bufptr);
            return 0;
        }
        *bufptr = newbuf;
        *bufalloc = new_alloc;
    }
    return 1;
}

static int _bufappend(
        char **bufptr, size_t *bufalloc, size_t *buffill,
        const char *appendstr, size_t amount
        ) {
    if (amount == 0)
        return 1;
    const size_t appendlen = strlen(appendstr);
    if (!_ensurebufsize(bufptr, bufalloc,
            (*buffill) + appendlen * amount + 1))
        return 0;
    char *write = (*bufptr) + (*buffill);
    size_t k = 0;
    while (k < amount) {
        memcpy(write, appendstr,
            appendlen * amount);
        write += appendlen;
        k++;
    }
    (*buffill) += appendlen * amount;
    return 1;
}

#define MAX_LIST_NESTING 16

S3DHID void _internal_spew3dweb_markdown_IsListOrCodeIndentEx(
        size_t pos, const char *buf,
        size_t buflen,
        int lastnonemptylineorigindent,
        int lastnonemptylineeffectiveindent,
        int lastnonemptylinewascodeindent,
        int lastlinewasempty,
        int *in_list_with_orig_indent_array,
        int *in_list_logical_nesting_depth,
        int *out_is_in_list_depth,
        int *out_is_code,
        int *out_effective_indent,
        int *out_write_this_many_spaces,
        int *out_content_start,
        int *out_orig_indent,
        int *out_is_list_entry,
        int *out_number_list_entry_num
        ) {
    int startpos = pos;
    int indent_depth = 0;
    while (pos < buflen &&
            (buf[pos] == ' ' ||
            buf[pos] == '\t')) {
        indent_depth += (
            buf[pos] == '\t' ? 4 : 1
        );
        pos += 1;
    }
    *out_is_list_entry = 0;
    *out_number_list_entry_num = -1;
    *out_content_start = (pos - startpos);
    if (indent_depth >= lastnonemptylineorigindent + 4 &&
            !lastnonemptylinewascodeindent) {
        // This starts a code line.
        *out_is_code = 1;
        *out_is_in_list_depth = *in_list_logical_nesting_depth;
        *out_effective_indent = (
            indent_depth + (
                lastnonemptylineeffectiveindent -
                lastnonemptylineorigindent
            )
        );
        *out_write_this_many_spaces = *out_effective_indent;
        *out_orig_indent = indent_depth;
        return;
    }
    if (((*in_list_logical_nesting_depth) == 0 &&
            lastnonemptylineorigindent >= 4) ||
            (*in_list_logical_nesting_depth) > 1 &&
            indent_depth >=
            lastnonemptylineorigindent + 4) {
        // This resumes a code line.
        *out_is_code = 1;
        *out_is_in_list_depth = *in_list_logical_nesting_depth;
        *out_effective_indent = (
            indent_depth + (
                lastnonemptylineeffectiveindent -
                lastnonemptylineorigindent
            )
        );
        *out_write_this_many_spaces = *out_effective_indent;
        *out_orig_indent = indent_depth;
        return;
    }
    // If we reached the end already, bail:
    if (pos >= buflen) {
        *out_is_code = 0;
        *out_is_in_list_depth = 0;
        *out_effective_indent = 0;
        *out_write_this_many_spaces = 0;
        *out_orig_indent = 0;
        return;
    }
    // See if this could be a list item start:
    int couldbelist = 0;
    int iflistthenwithnumber = -1;
    int iflistthenbulletlen = 0;
    int bulletstartpos = pos;
    assert(pos < buflen);
    if (buf[pos] >= '0' && buf[pos] <= '9') {
        char digitbuf[12] = {0};
        digitbuf[0] = buf[pos];
        pos += 1;
        int numberstartpos = pos;
        while (pos < buflen &&
                (buf[pos] >= '0' &&
                buf[pos] <= '9')) {
            if (strlen(digitbuf) < 11)
                digitbuf[strlen(digitbuf)] = buf[pos];
            pos += 1;
        }
        if (pos < buflen && buf[pos] == '.') {
            pos += 1;
            couldbelist = 1;
            iflistthenbulletlen = strlen(digitbuf) + 1;
            iflistthenwithnumber = atoi(digitbuf);
        }
    } else if (buf[pos] == '-' || buf[pos] == '*') {
        pos += 1;
        couldbelist = 1;
        iflistthenbulletlen = 2;
        iflistthenwithnumber = -1;
    }
    if (couldbelist == 1 &&
           (pos >= buflen || (
                pos < buflen &&
                (buf[pos] == ' ' ||
                buf[pos] == '\t')))) {
        *out_is_list_entry = 1;

        // This is a list entry, for sure.
        int effectivechars_bullet_to_text = (
            buf[pos] == '\t' ? (
            (pos - bulletstartpos) + 4) : (
            (pos - bulletstartpos) + 1)
        );
        int contained_text_indent = (
            effectivechars_bullet_to_text + indent_depth
        );
        (*out_content_start) = (
            (*out_content_start) + effectivechars_bullet_to_text
        );
        // Now determine what nesting depth:
        int list_nesting = 0;
        if ((*in_list_logical_nesting_depth) == 0) {
            list_nesting = 1;
        } else {
            int k = 0;
            while (k < (*in_list_logical_nesting_depth)) {
                if (k >= (*in_list_logical_nesting_depth) - 1) {
                    if (contained_text_indent >
                            in_list_with_orig_indent_array[k] + 2)
                        list_nesting = (
                            (*in_list_logical_nesting_depth) + 1
                        );
                    else
                        list_nesting = (
                            (*in_list_logical_nesting_depth)
                        );
                    break;
                } else if (contained_text_indent >=
                            in_list_with_orig_indent_array[k] &&
                        contained_text_indent <
                            in_list_with_orig_indent_array[k + 1]) {
                    list_nesting = k + 1;
                    break;
                }
                k += 1;
            }
        }
        *out_is_code = 0;
        *out_is_in_list_depth = list_nesting;
        *out_effective_indent = (list_nesting * 4);
        *out_orig_indent = contained_text_indent;
        int write_spaces = ((*out_effective_indent) -
            (iflistthenwithnumber >= 0 ? 4 : 2));
        if (write_spaces < 0) write_spaces = 0;
        *out_number_list_entry_num = iflistthenwithnumber; 
        *out_write_this_many_spaces = write_spaces;
        return;
    }
    pos = bulletstartpos;  // Revert, since not a list.

    // This is not a list, and not code. Out-indent of list if non-empty:
    if (pos >= buflen || buf[buflen - 1] == '\n' ||
            buf[buflen - 1] == '\r') {
        // It's empty, so keep current list going.
        *out_is_code = 0;
        *out_is_in_list_depth = (*in_list_logical_nesting_depth);
        *out_orig_indent = (
            (*in_list_logical_nesting_depth) > 0 ?
            in_list_with_orig_indent_array[
                (*in_list_logical_nesting_depth) - 1] : 0);
        *out_effective_indent = (
            (*in_list_logical_nesting_depth) * 4
        );
        *out_write_this_many_spaces = 0;
        return;
    }
    // See what list level we must out indent to:
    int list_nesting = 0;
    if (!lastlinewasempty &&
            ((*in_list_logical_nesting_depth) <= 1 ||
            indent_depth >
            in_list_with_orig_indent_array[
                (*in_list_logical_nesting_depth) - 2
            ])) {
        // Special case, since we're still further indented
        // than one layer "outside" of ours and glue to a
        // previous list entry with no empty line,
        // continue the list entry.
        list_nesting = (*in_list_logical_nesting_depth);
    } else {
        // Find which list entry depth we still fit inside:
        int k = 0;
        while (k < (*in_list_logical_nesting_depth)) {
            if (k >= (*in_list_logical_nesting_depth) - 1) {
                if (indent_depth >
                        in_list_with_orig_indent_array[k] + 2)
                    list_nesting = (
                        (*in_list_logical_nesting_depth) + 1
                    );
                else
                    list_nesting = (*in_list_logical_nesting_depth);
                break;
            } else if (indent_depth >=
                        in_list_with_orig_indent_array[k] &&
                    indent_depth <
                        in_list_with_orig_indent_array[k + 1]) {
                list_nesting = k + 1;
                break;
            }
            k += 1;
        }
    }
    *out_is_code = 0;
    *out_is_in_list_depth = list_nesting;
    *out_orig_indent = (
        (*in_list_logical_nesting_depth) > 0 ?
        in_list_with_orig_indent_array[
            (*in_list_logical_nesting_depth) - 1] :
        indent_depth);
    *out_effective_indent = (
        (*in_list_logical_nesting_depth) * 4
    );
    *out_write_this_many_spaces = (
        (*out_effective_indent)
    );
}

char *spew3dweb_markdown_CleanByteBuf(
        const char *input, size_t inputlen,
        size_t *out_len, size_t *out_alloc
        ) {
    char *resultchunk = NULL;
    size_t resultfill = 0;
    size_t resultalloc = 0;
    if (!_ensurebufsize(&resultchunk, &resultalloc, 1))
        return NULL;

    #define INSC(insertchar) \
        (_ensurebufsize(&resultchunk, &resultalloc,\
        resultfill + 1) ? (\
        (resultchunk[resultfill] = insertchar) * 0 +\
        ((resultfill++) * 0) + 1) :\
        0)
    #define INS(insertstr) \
        (_bufappend(&resultchunk, &resultalloc, &resultfill,\
        insertstr, 1))
    #define INSREP(insertstr, amount) \
        (_bufappend(&resultchunk, &resultalloc, &resultfill,\
        insertstr, amount))

    int in_list_with_orig_indent[MAX_LIST_NESTING] = {0};
    int in_list_logical_nesting_depth = 0;
    int currentlineeffectiveindent = 0;
    int currentlineorigindent = 0;
    int lastnonemptylineeffectiveindent = 0;
    int lastnonemptylineorigindent = 0;
    int lastnoncodelineorigindent = 0;
    int lastnonemptylinewascodeindent = 0;
    int lastlinewasempty = 0;
    size_t i = 0;
    while (i <= inputlen) {
        const char c = (
            i < inputlen ? input[i] : '\0'
        );
        int starts_new_line = (
            (resultfill == 0 && i == 0) || (
            i > 0 && (input[i - 1] == '\n' ||
            input[i - 1] == '\r') &&
            resultchunk[resultfill - 1] == '\n'));
        if (starts_new_line && (
                i + 1 >= inputlen ||
                input[inputlen + 1] != '\r' ||
                input[inputlen + 1] != '\n')) {
            // A non-empty line:
            int out_is_in_list_depth;
            int out_is_code;
            int out_effective_indent;
            int out_write_this_many_spaces;
            int out_content_start;
            int out_orig_indent;
            int out_is_list_entry;
            int out_list_entry_num_value;
            _internal_spew3dweb_markdown_IsListOrCodeIndentEx(
                i, input, inputlen,
                lastnonemptylineorigindent,
                lastnonemptylineeffectiveindent,
                lastnonemptylinewascodeindent,
                lastlinewasempty,
                in_list_with_orig_indent,
                &in_list_logical_nesting_depth,
                &out_is_in_list_depth,
                &out_is_code,
                &out_effective_indent,
                &out_write_this_many_spaces,
                &out_content_start,
                &out_orig_indent,
                &out_is_list_entry,
                &out_list_entry_num_value
            );
            int isonlywhitespace = (!out_is_list_entry && (
                i + out_content_start >= inputlen || (
                input[i + out_content_start] == '\n' ||
                input[i + out_content_start] == '\r')
            ));
            if (isonlywhitespace) {
                // It had only whitespace, that counts as empty.
                assert(out_content_start > 0);
                i += out_content_start;
                lastlinewasempty = 1; 
                continue;
            }
            lastlinewasempty = 0;
            if (!INSREP(" ", out_write_this_many_spaces))
                return NULL;
            if (out_is_list_entry) {
                assert(!out_is_code);
                if (out_list_entry_num_value >= 0) {
                    char buf[5] = {0};
                    snprintf(buf, sizeof(buf) - 1, "%d",
                        out_list_entry_num_value);
                    buf[sizeof(buf) - 1] = '\0';
                    if (!INS(buf) || !INSC('.'))
                        return NULL;
                    assert(strlen(buf) >= 1);
                    if (strlen(buf) == 1)
                        if (!INS("  "))
                            return NULL;
                    else
                        if (!INS(" "))
                            return NULL;
                } else {
                    if (!INS("- "))
                        return NULL;
                }
                assert(out_content_start > 0);
            }
            lastnonemptylinewascodeindent = out_is_code;
            currentlineeffectiveindent = (
                out_effective_indent
            );
            currentlineorigindent = out_orig_indent;
            i += out_content_start;
            lastlinewasempty = 0;
            if (out_content_start > 0)
                // We skipped forward, so restart iteration:
                continue;
        }
        if (c == '\r' || c == '\n' ||
                i >= inputlen) {
            // Remove trailing indent from previous line:
            while (resultfill > 0 && (
                    resultchunk[resultfill - 1] == ' ' ||
                    resultchunk[resultfill - 1] == '\t'))
                resultfill--;
            // If last line wasn't empty, remember its indent:
            if (resultfill > 0 &&
                    resultchunk[resultfill - 1] != '\n') {
                lastnonemptylineeffectiveindent = (
                    currentlineeffectiveindent
                );
                lastnonemptylineorigindent = (
                    currentlineorigindent
                );
                lastlinewasempty = 0;
            } else {
                lastlinewasempty = 1;
            }
            // Fix the line break type:
            if (i < inputlen) {
                if (!INSC('\n'))
                    return NULL;
                if (c == '\r' &&
                        i + 1 < inputlen &&
                        input[i +  1] == '\n')
                    i++;
                i++;
                continue;
            } else {
                break;
            }
        }
        if (c == '\0' && i < inputlen) {
            if (!INSC('?'))
                return NULL;
        } else {
            if (!INSC(c))
                return NULL;
        }
        i++;
    }
    resultchunk[resultfill] = '\0';
    if (out_len) *out_len = resultfill;
    if (out_alloc) *out_alloc = resultalloc;
    return resultchunk;
}

char *spew3dweb_markdown_Clean(
        const char *inputstr,
        size_t *out_len, size_t *out_alloc
        ) {
    return spew3dweb_markdown_CleanByteBuf(
        inputstr, strlen(inputstr),
        out_len, out_alloc
    );
}

#endif  // SPEW3DWEB_IMPLEMENTATION

