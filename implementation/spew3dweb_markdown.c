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

S3DHID char *_internal_spew3dweb_markdown_GetIChunkFromCustomIOEx(
        int (*read_func)(char *buff, size_t amount, void *userdata),
        int (*checkeof_func)(void *userdata),
        int (*seekback_func)(size_t backward_amount, void *userdata),
        void *userdata,
        char *optionalbuf, size_t optionalbufsize,
        size_t opt_maxchunklen, size_t opt_minchunklen,
        char *(*opt_uritransformcallback)(
            const char *uri, void *userdata
        ),
        void *opt_uritransform_userdata,
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

    if (checkeof_func(userdata)) {
        return strdup("");
    }

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

        // Read some data:
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
                        if (out_len) *out_len = k;
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
            if (out_len) *out_len = readbuffill;
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
        char *(*opt_uritransformcallback)(
            const char *uri, void *userdata
        ),
        void *opt_uritransform_userdata,
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
            opt_uritransformcallback, opt_uritransform_userdata,
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
        char *(*opt_uritransformcallback)(
            const char *uri, void *userdata
        ),
        void *opt_uritransform_userdata,
        size_t *out_len
        ) {
    return _internal_spew3dweb_markdown_GetIChunkFromCustomIOEx(
        read_func, checkeof_func, seekback_func, userdata,
        NULL, 0,
        opt_maxchunklen, 1024 * 5,
        opt_uritransformcallback, opt_uritransform_userdata,
        out_len
    );
}

S3DHID char *_internal_spew3dweb_markdown_GetIChunkFromVFSFileEx(
        SPEW3DVFS_FILE *f,
        char *optionalbuf, size_t optionalbufsize,
        size_t opt_maxchunklen, size_t opt_minchunklen,
        char *(*opt_uritransformcallback)(
            const char *uri, void *userdata
        ),
        void *opt_uritransform_userdata,
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
            opt_uritransformcallback, opt_uritransform_userdata,
            out_len
        )
    );
    return result;
}

S3DEXP char *spew3dweb_markdown_GetIChunkFromVFSFile(
        SPEW3DVFS_FILE *f, size_t opt_maxchunklen,
        char *(*opt_uritransformcallback)(
            const char *uri, void *userdata
        ),
        void *opt_uritransform_userdata,
        size_t *out_len
        ) {
    return _internal_spew3dweb_markdown_GetIChunkFromVFSFileEx(
        f, NULL, 0,
        opt_maxchunklen, 1024 * 5,
        opt_uritransformcallback, opt_uritransform_userdata,
        out_len
    );
}

S3DEXP char *spew3dweb_markdown_GetIChunkFromDiskFile(
        FILE *f, size_t opt_maxchunklen,
        char *(*opt_uritransformcallback)(
            const char *uri, void *userdata
        ),
        void *opt_uritransform_userdata,
        size_t *out_len
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
            opt_uritransformcallback, opt_uritransform_userdata,
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
        const char *appendbuf, size_t appendbuflen, size_t amount
        ) {
    if (amount == 0)
        return 1;
    if (!_ensurebufsize(bufptr, bufalloc,
            (*buffill) + appendbuflen * amount + 1))
        return 0;
    assert(*bufalloc >= appendbuflen * amount);
    char *write = (*bufptr) + (*buffill);
    size_t k = 0;
    while (k < amount) {
        memcpy(write, appendbuf,
            appendbuflen * amount);
        write += appendbuflen;
        k++;
    }
    (*buffill) += appendbuflen * amount;
    return 1;
}

static int _bufappendstr(
        char **bufptr, size_t *bufalloc, size_t *buffill,
        const char *appendstr, size_t amount
        ) {
    return _bufappend(bufptr, bufalloc, buffill,
        appendstr, strlen(appendstr), amount);
}

#define MAX_FORMAT_NESTING 6
#define MAX_LIST_NESTING 12

S3DHID void _internal_spew3dweb_markdown_IsListOrCodeIndentEx(
        size_t pos, const char *buf,
        size_t buflen,
        int lastnonemptylineorigindent,
        int lastnonemptylineeffectiveindent,
        int lastnonemptylinewascode,
        int lastlinewasemptyorblockinterruptor,
        int *in_list_with_orig_indent_array,
        int *in_list_with_orig_bullet_indent_array,
        int in_list_logical_nesting_depth,
        int *out_is_in_list_depth,
        int *out_is_code,
        int *out_effective_indent,
        int *out_write_this_many_spaces,
        int *out_content_start,
        int *out_orig_indent,
        int *out_orig_bullet_indent,
        int *out_is_list_entry,
        char *out_list_bullet_type,
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

    assert(lastnonemptylineorigindent >= 0);
    if (indent_depth >=
            lastnonemptylineorigindent + 4 &&
            !lastnonemptylinewascode) {
        // This starts a code line.
        int adjustedwriteoutcodeindent = (
            indent_depth - lastnonemptylineorigindent
        ) + lastnonemptylineeffectiveindent;
        if (adjustedwriteoutcodeindent < 4)
            adjustedwriteoutcodeindent = 4;
        *out_is_code = 1;
        *out_is_in_list_depth = in_list_logical_nesting_depth;
        *out_effective_indent = adjustedwriteoutcodeindent;
        *out_write_this_many_spaces = adjustedwriteoutcodeindent;
        *out_orig_indent = indent_depth;
        return;
    }
    if ((lastnonemptylinewascode ||
            (in_list_logical_nesting_depth == 0 &&
            lastnonemptylineorigindent >= 4)) &&
            indent_depth >= 4) {
        // This resumes a code line.
        int adjustedwriteoutcodeindent = (
            indent_depth + (
                lastnonemptylineeffectiveindent -
                lastnonemptylineorigindent
            ));
        if (adjustedwriteoutcodeindent < 4)
            adjustedwriteoutcodeindent = 4;
        *out_is_code = 1;
        *out_is_in_list_depth = in_list_logical_nesting_depth;
        *out_effective_indent = adjustedwriteoutcodeindent;
        *out_write_this_many_spaces = adjustedwriteoutcodeindent;
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
    char iflistthenbullettype = '\0';
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
            if (strlen(digitbuf) > 9)
                iflistthenwithnumber = 999999999;
            else
                iflistthenwithnumber = atoi(digitbuf);
            iflistthenbullettype = '1';
        }
    } else if (buf[pos] == '-' || buf[pos] == '*' ||
            buf[pos] == '>') {
        iflistthenbullettype = buf[pos];
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
        *out_list_bullet_type = iflistthenbullettype;

        // This is a list entry, for sure.
        int effectivechars_bullet_to_text = (
            buf[pos] == '\t' ? (
            (pos - bulletstartpos) + 4) : (
            (pos - bulletstartpos) + 1)
        );
        int contained_text_orig_indent = (
            effectivechars_bullet_to_text + indent_depth
        );
        int bullet_orig_indent = (*out_content_start);
        (*out_content_start) = (
            (*out_content_start) + effectivechars_bullet_to_text
        );
        // Now determine what nesting depth:
        int list_nesting = 0;
        if (in_list_logical_nesting_depth == 0) {
            list_nesting = 1;
        } else {
            int k = 0;
            while (k < in_list_logical_nesting_depth) {
                const int current_depth = (k + 1);
                int nesting_bullet_indent = (
                    in_list_with_orig_bullet_indent_array[k]
                );
                if (bullet_orig_indent < nesting_bullet_indent) {
                    list_nesting = current_depth - 1;
                    if (list_nesting < 0) list_nesting = 0;
                    break;
                } else if (bullet_orig_indent == nesting_bullet_indent) {
                    list_nesting = current_depth;
                    break;
                } else if (k >= in_list_logical_nesting_depth - 1) {
                    list_nesting = (
                        in_list_logical_nesting_depth + 1
                    );
                    break;
                }
                k += 1;
            }
        }
        if (list_nesting < 1) {
            // Since we're a list item by ourselves, always
            // start a new list.
            list_nesting = 1;
        }
        *out_is_code = 0;
        *out_is_in_list_depth = list_nesting;
        *out_effective_indent = (list_nesting * 4);
        *out_orig_indent = contained_text_orig_indent;
        *out_orig_bullet_indent = bullet_orig_indent;
        int write_spaces = ((*out_effective_indent) -
            (iflistthenwithnumber >= 0 ? 4 : 2));
        if (write_spaces < 0) write_spaces = 0;
        *out_number_list_entry_num = iflistthenwithnumber; 
        *out_write_this_many_spaces = write_spaces;
        return;
    }
    pos = bulletstartpos;  // Revert, since not a list.

    // This is not a list, and not code. Out-indent of list if non-empty:
    if (pos >= buflen || buf[pos] == '\n' ||
            buf[pos] == '\r') {
        // It's empty, so keep current list going.
        *out_is_code = 0;
        *out_is_in_list_depth = in_list_logical_nesting_depth;
        *out_orig_indent = (
            in_list_logical_nesting_depth > 0 ?
            in_list_with_orig_indent_array[
                in_list_logical_nesting_depth - 1] : 0);
        *out_effective_indent = (
            in_list_logical_nesting_depth * 4
        );
        *out_write_this_many_spaces = 0;
        return;
    }
    // See what list level we must out indent to:
    int list_nesting = 0;
    if (!lastlinewasemptyorblockinterruptor &&
            in_list_logical_nesting_depth >= 1 &&
            indent_depth >=
            in_list_with_orig_indent_array[
                in_list_logical_nesting_depth - 1
            ]) {
        // Special case, since we're still further indented
        // than one layer "outside" of ours and glue to a
        // previous list entry with no empty line,
        // continue the list entry.
        assert(indent_depth > 0);
        list_nesting = in_list_logical_nesting_depth;
    } else {
        // Find which list entry depth we still fit inside:
        int k = 0;
        while (k < in_list_logical_nesting_depth) {
            const int current_depth = k - 1;
            assert(in_list_with_orig_indent_array[k] > 0);
            if (indent_depth <
                    in_list_with_orig_indent_array[k]) {
                list_nesting = current_depth - 1;
                if (list_nesting < 0) list_nesting = 0;
                break;
            } else if (indent_depth ==
                    in_list_with_orig_indent_array[k] ||
                    indent_depth ==
                    in_list_with_orig_indent_array[k + 1]) {
                list_nesting = in_list_logical_nesting_depth;
                break;
            } else if (k >= in_list_logical_nesting_depth - 1) {
                list_nesting = in_list_logical_nesting_depth + 1;
                break;
            }
            k += 1;
        }
    }
    assert(list_nesting >= 0);
    assert(indent_depth > 0 || list_nesting == 0);
    *out_is_code = 0;
    *out_is_in_list_depth = list_nesting;
    *out_orig_indent = (
        list_nesting > 0 ?
        in_list_with_orig_indent_array[
            list_nesting - 1] :
        indent_depth);
    *out_effective_indent = (
        list_nesting * 4
    );
    *out_write_this_many_spaces = (
        (*out_effective_indent)
    );
}

static int _getlastlinetrulyempty(
        const char *buffer, int buffill
        ) {
    int pos = buffill - 1;
    while (pos >= 0 &&
            buffer[pos] != '\n' &&
            buffer[pos] != '\r')
        pos -= 1;
    if (pos < 0)
        return 1;
    pos -= 1;
    if (buffer[pos + 1] == '\n' &&
            pos >= 0 && buffer[pos] == '\r')
        pos -= 1;
    while (pos >= 0 &&
            (buffer[pos] == ' ' ||
            buffer[pos] == '\t'))
        pos -= 1;
    if (pos < 0 || buffer[pos] == '\n' ||
            buffer[pos] == '\r')
        return 1;
    return 0;
}

static int _getlastoutputlinelen(
        const char *buffer, int buffill,
        int ignore_indent, int ignore_confirmed_bullet
        ) {
    int pos = buffill - 1;
    while (pos >= 0 &&
            buffer[pos] != '\n' &&
            buffer[pos] != '\r')
        pos -= 1;
    if (pos < 0)
        return 0;
    pos -= 1;
    if (buffer[pos + 1] == '\n' &&
            pos >= 0 && buffer[pos] == '\r')
        pos -= 1;
    while (pos >= 0 &&
            (buffer[pos] == ' ' ||
            buffer[pos] == '\t'))
        pos -= 1;
    int len = 0;
    while (pos >= 0 &&
            buffer[pos] != '\n' &&
            buffer[pos] != '\r') {
        pos -= 1;
        len += 1;
    }
    if (ignore_indent) {
        while (len > 0 && (
                buffer[pos + 1] == ' ' ||
                buffer[pos + 2] == '\t')) {
            len -= 1;
            pos += 1;
        }
        if (ignore_confirmed_bullet) {
            assert(len > 1 && ((
                buffer[pos + 1] < 'a' ||
                buffer[pos + 1] > 'z') &&
                buffer[pos + 1] < 'A' ||
                buffer[pos + 1] > 'Z'));
            if (buffer[pos + 1] >= '1' &&
                    buffer[pos + 1] <= '9') {
                len -= 4;
            } else {
                assert(buffer[pos + 2] == ' ');
                len -= 2;
            }
            if (len < 0) len = 0;
        }
    }
    return len;
}

static int _lineonlycontinuouschar(
        const char *input, size_t inputlen,
        size_t linestart, char c
        ) {
    size_t i = linestart;
    while (i < inputlen && (
            input[i] == ' ' || input[i] == '\t'))
        i += 1;
    if (i >= inputlen || input[i] != c)
        return 0;
    while (i < inputlen && input[i] == c)
        i += 1;
    while (i < inputlen && (input[i] == ' ' ||
            input[i] == '\t'))
        i += 1;
    if (i >= inputlen || (input[i] == '\n' ||
            input[i] == '\t'))
        return 1;
    return 0;
}

static int _linestandaloneheadingdepth(
        const char *input, size_t inputlen,
        size_t linestart
        ) {
    size_t i = linestart;
    while (i < inputlen && (
            input[i] == ' ' || input[i] == '\t'))
        i += 1;
    if (i >= inputlen || input[i] != '#')
        return 0;
    int rating = 1;
    i += 1;
    while (i < inputlen && input[i] == '#') {
        rating += 1;
        i += 1;
    }
    if (i >= inputlen || input[i] != ' ')
        return 0;
    i += 1;
    while (i < inputlen && (
            input[i] == ' ' || input[i] == '\t'))
        i += 1;
    if (i >= inputlen || input[i] == '#' ||
            input[i] == '\n' || input[i] == '\r')
        return 0;
    return rating;
}

#define INSC(insertchar) \
    (_ensurebufsize(&resultchunk, &resultalloc,\
    resultfill + 1) ? (\
    (resultchunk[resultfill] = insertchar) * 0 +\
    ((resultfill++) * 0) + 1) :\
    0)
#define INS(insertstr) \
    (_bufappendstr(&resultchunk, &resultalloc, &resultfill,\
    insertstr, 1))
#define INSREP(insertstr, amount) \
    (_bufappendstr(&resultchunk, &resultalloc, &resultfill,\
    insertstr, amount))
#define INSBUF(insertbuf, insertbuflen) \
    (_bufappend(&resultchunk, &resultalloc, &resultfill,\
    insertbuf, insertbuflen, 1))

S3DEXP int spew3dweb_markdown_GetBacktickByteBufLangPrefixLen(
        const char *block, size_t blocklen,
        size_t offset
        ) {
    size_t i = offset;
    if (i >= blocklen || (
            (block[i] < 'a' || block[i] > 'z') &&
            (block[i] < 'A' || block[i] > 'Z') &&
            block[i] <= 127))
        return 0;
    while (i < blocklen) {
        if (block[i] == ' ' || block[i] == '\t' ||
                block[i] == '\r' || block[i] == '\n') {
            break;
        }
        if (block[i] < ' ' || block[i] == '`' ||
                block[i] == '(' || block[i] == '[' ||
                block[i] == ';' ||
                block[i] == '|' ||
                block[i] == '\'' ||
                block[i] == '"' ||
                block[i] == '<' ||
                block[i] == '{')
            return 0;
        i += 1;
    }
    return i - offset;
}

S3DEXP int spew3dweb_markdown_GetBacktickStrLangPrefixLen(
        const char *block, size_t offset
        ) {
    return spew3dweb_markdown_GetBacktickByteBufLangPrefixLen(
        block, strlen(block), offset
    );
}

S3DEXP char *spew3dweb_markdown_CleanByteBuf(
        const char *input, size_t inputlen,
        int opt_allowunsafehtml,
        int opt_stripcomments,
        char *(*opt_uritransformcallback)(
            const char *uri, void *userdata
        ),
        void *opt_uritransform_userdata,
        size_t *out_len, size_t *out_alloc
        ) {
    return _internal_spew3dweb_markdown_CleanByteBufEx(
        input, inputlen, 0, 0, opt_allowunsafehtml,
        opt_stripcomments,
        opt_uritransformcallback, opt_uritransform_userdata,
        out_len, out_alloc
    );
}

S3DEXP int spew3dweb_markdown_IsStrUrl(const char *test_str) {
    size_t test_str_len = strlen(test_str);
    if (test_str_len <= 0 || test_str[0] != '[')
        return 0;
    int len = _internal_spew3dweb_markdown_GetLinkImgLen(
        test_str, test_str_len, 0, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL
    );
    return (len == test_str_len);
}

S3DEXP int spew3dweb_markdown_IsStrImage(const char *test_str) {
    size_t test_str_len = strlen(test_str);
    if (test_str_len <= 0 || test_str[0] != '!')
        return 0;
    int len = _internal_spew3dweb_markdown_GetLinkImgLen(
        test_str, test_str_len, 0, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL
    );
    return (len == test_str_len);
}

static int _internal_spew3dweb_skipmarkdownhtmlcomment(
        const char *input, size_t inputlen, size_t startpos
        ) {
    if (startpos + 3 >= inputlen ||
            memcmp(input + startpos, "<!--", 4) != 0)
        return -1;
    size_t i = startpos + 4;
    while (i + 2 < inputlen &&
            memcmp(input + i, "-->", 3) != 0)
        i += 1;
    if (i + 2 < inputlen)
        i += 3;
    return (int)(i - startpos);
}

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
        ) {
    assert(!opt_adjustindentinside || opt_allowmultiline);
    assert(!opt_squashmultiline || opt_allowmultiline);
    assert(!opt_adjustindentinside || !opt_squashmultiline);
    char *resultchunk = *resultchunkptr;
    size_t resultfill = *resultfillptr;
    size_t resultalloc = *resultallocptr;

    size_t i = startpos;
    while (i < inputlen) {
        // All line break and end of inline handling:
        if (((input[i] == '\n' || input[i] == '\r') &&
                !opt_allowmultiline) ||
                (!currentlineiscode &&
                input[i] == '`' && i + 2 < inputlen &&
                input[i + 1] == '`' &&
                input[i + 2] == '`')) {
            *resultchunkptr = resultchunk;
            *resultfillptr = resultfill;
            *resultallocptr = resultalloc;
            return i;
        } else if ((input[i] == '\r' || input[i] == '\n') &&
                (opt_adjustindentinside ||
                opt_squashmultiline)) {
            if (input[i] == '\r' && i + 1 < inputlen &&
                    input[i + 1] == '\n')
                i += 1;
            i += 1;
            // "Eat up" allow following indent first:
            while (i < inputlen && (input[i] == ' ' ||
                    input[i] == '\t'))
                i += 1;
            if (!opt_squashmultiline) {
                // Output new line with adjusted indent:
                if (!INSC('\n'))
                    goto errorquit;
                if (!INSREP(" ", effectiveindent))
                    goto errorquit;
            } else if (resultfill == 0 ||
                    (resultchunk[resultfill - 1] != '\t' &&
                    resultchunk[resultfill - 1] != ' ')) {
                if (!INSC(' '))
                    goto errorquit;
            }
            continue;
        } else if (input[i] == '\r' || input[i] == '\n') {
            if (input[i] == '\r' &&
                    i + 1 < inputlen && input[i + 1] == '\n')
                i += 1;
            i += 1;
            if (!INSC('\n'))
                goto errorquit;
            continue;
        }

        // All the inline transformations follow here:
        int _commentskip = -1;
        if (input[i] == '\0') {
            if (!INS("�"))
                goto errorquit;
            i += 1;
            continue;
        } else if (input[i] == '<' && !currentlineiscode &&
                (_commentskip =
                    _internal_spew3dweb_skipmarkdownhtmlcomment(
                        input, inputlen, i
                )) > 0) {
            if (opt_stripcomments) {
                i += _commentskip;
                continue;
            } else {
                if (!INSBUF(input + i, _commentskip))
                    goto errorquit;
                i += _commentskip;
                continue;
            }
        } else if (input[i] == '\\' && !currentlineiscode) {
            i += 1;
            if (i < inputlen) {
                if (input[i] == '\r' || input[i] == '\n' ||
                        input[i] == '\0') {
                    // Don't let the backslash character eat this!
                    // Instead, bail out early and process on next loop.
                    if (opt_escapeunambiguousentities) {
                        // Double escape this to be unambiguous.
                        if (!INS("\\")) {
                            errorquit: ;
                            *resultchunkptr = resultchunk;
                            *resultfillptr = resultfill;
                            *resultallocptr = resultalloc;
                            return -1;
                        }
                    }
                    if (!INS("\\"))
                        goto errorquit;
                    // Intentionally no i += 1 here.
                    continue;  // Parse char unescaped in next iteration.
                }
                if (input[i] == '<' &&
                        opt_escapeunambiguousentities) {
                    if (!INS("&lt;"))
                        goto errorquit;
                } else if (input[i] == '>' &&
                        opt_escapeunambiguousentities) {
                    if (!INS("&gt;"))
                        goto errorquit;
                } else if (input[i] == '&' &&
                        opt_escapeunambiguousentities) {
                    if (!INS("&amp;"))
                        goto errorquit;
                } else {
                    if (!INSC('\\'))
                        goto errorquit;
                    if (!INSC(input[i]))
                        goto errorquit;
                }
                i += 1;
            } else {
                if (!INSC('\\'))
                    goto errorquit;
            }
            continue;
        }
        if (input[i] == '`' && !currentlineiscode &&
                (i + 2 >= inputlen || input[i + 1] != '`' ||
                input[i + 2] != '`')) {
            // Possibly inline code!
            size_t i2 = i;
            int ticks = 1;
            if (i2 + 1 < inputlen && input[i2 + 1] == '`') {
                ticks += 1;
                i2 += 1;
            }
            i2 += 1;
            // To verify, find the end of our inline code.
            size_t codestart = i2;
            size_t codeend = 0;
            while (i2 < inputlen) {
                if (input[i2] == '`' && (
                        (ticks == 1 && (i2 + 1 >= inputlen ||
                            input[i2 + 1] != '`')) ||
                        (ticks == 2 && input[i2 + 1] == '`' &&
                            (i2 + 2 >= inputlen ||
                            input[i2 + 2] != '`')))) {
                    codeend = i2;
                    break;
                }
                if (input[i2] == '\r' || input[i2] == '\n') {
                    size_t i3 = i2;
                    if (input[i3] == '\r' && i3 + 1 < inputlen &&
                            input[i3 + 1] == '\n')
                        i3 += 1;
                    i3 += 1;
                    int nextline_indent = 0;
                    while (i3 < inputlen) {
                        if (input[i3] == '\t') {
                            nextline_indent += 4;
                        } else if (input[i3] == ' ') {
                            nextline_indent += 1;
                        } else {
                            break;
                        }
                        i3 += 1;
                    }
                    // If next line continues with an obvious new
                    // block or unfitting indent then abort:
                    if (i3 >= inputlen || input[i3] == '\r' ||
                            input[i3] == '\n' ||
                            input[i3] == '#' || input[i3] == '*' ||
                            input[i3] == '-' || input[i3] == '=' ||
                            input[i3] == '`' ||
                            nextline_indent < origindent ||
                            nextline_indent > origindent + 3)
                        break;
                    // Okay, this line looks non-suspicious. Continue!
                    i2 = i3;
                    continue;
                }
                i2 += 1;
            }
            if (codeend > 0) {  // Valid inline code:
                if (!INSREP("`", ticks))
                    goto errorquit;
                if (codeend - codestart > 0) {
                    ssize_t result = (
                        _internal_spew3dweb_markdown_AddInlineAreaClean(
                            input, codeend, codestart,
                            &resultchunk, &resultfill, &resultalloc,
                            origindent, effectiveindent,
                            1,
                            // We want to always fix indents (unless squashing):
                            !opt_squashmultiline, opt_squashmultiline,
                            1, opt_forcelinksoneline,
                            opt_escapeunambiguousentities,
                            opt_allowunsafehtml,
                            opt_stripcomments,
                            NULL, NULL));
                    assert(result == -1 || result == codeend);
                    if (result < 0)
                        goto errorquit;
                }
                if (!INSREP("`", ticks))
                    goto errorquit;
                i = i2 + ticks;
                continue;
            } else {
                // Must escape invalid.
                if (!INSC('\\'))
                    goto errorquit;
                if (!INSC('`'))
                    goto errorquit;
                i += 1;
                continue;
            }
        }
        if (input[i] == '&' && !currentlineiscode &&
                (opt_escapeunambiguousentities ||
                (i + 1 < inputlen &&
                ((input[i + 1] >= 'a' && input[i + 1] <= 'z') ||
                (input[i + 1] >= 'A' && input[i + 1] <= 'Z') ||
                input[i + 1] == '#')))) {
            size_t i2 = i + 1;
            while (i2 < inputlen && i2 < i + 15 &&
                    ((input[i2] >= 'a' && input[i2] <= 'z') ||
                    (input[i2] >= 'A' && input[i2] <= 'Z')))
                i2++;
            if (i2 >= inputlen || input[i2] != ';') {
                if (!INS("&amp;"))
                    goto errorquit;
            } else {
                if (!INSC('&'))
                    goto errorquit;
            }
            i += 1;
            continue;
        }
        if (input[i] == '>' && !currentlineiscode &&
                (opt_escapeunambiguousentities ||
                i <= 0 ||
                ((input[i - 1] >= 'a' && input[i - 1] <= 'z') ||
                    (input[i - 1] >= 'A' && input[i - 1] <= 'Z')) ||
                (i >= 2 && input[i - 1] == '-' &&
                 input[i - 2] == '-'))) {
            if (!INS("&gt;"))
                goto errorquit;
            i += 1;
            continue;
        }
        if (input[i] == '<' && !currentlineiscode && (
                opt_escapeunambiguousentities ||
                (i + 1 < inputlen &&
                ((input[i + 1] >= 'a' && input[i + 1] <= 'z') ||
                (input[i + 1] >= 'A' && input[i + 1] <= 'Z') ||
                input[i + 1] == '#' || input[i + 1] == '!')))) {
            // Potentially a HTML tag, otherwise needs escaping:
            int endtagidx = -1;
            if (i + 1 < inputlen && opt_allowunsafehtml &&
                    ((input[i + 1] >= 'a' && input[i + 1] <= 'z') ||
                    ((input[i + 1] >= 'A' && input[i + 1] <= 'Z')))) {
                size_t i2 = i + 1;
                if ((input[i2] >= 'a' && input[i2] <= 'z') ||
                        (input[i2] >= 'A' && input[i2] <= 'Z')) {
                    while (i2 < inputlen && i2 < i + 15 && (
                            input[i2] == '-' &&
                            ((input[i2] >= 'a' &&
                                input[i2] <= 'z')) ||
                            ((input[i2] >= 'A' &&
                                input[i2] <= 'Z'))))
                        i2++;
                    if (i2 < inputlen && (input[i] == ' ' ||
                            input[i2] == '>')) {
                        // Likely a tag, find the end:
                        while (i2 < inputlen &&
                                input[i2] != '>' &&
                                input[i2] != '<')
                            i2++;
                        if (i2 < inputlen && input[i2] == '>')
                            endtagidx = i2;
                    }
                }
            }
            if (endtagidx < 0 && !INS("&lt;")) {
                goto errorquit;
            } else if (endtagidx >= 0) {
                if (!INSBUF(input + i,
                        (endtagidx - i) + 1)) {
                    goto errorquit;
                }
                i = endtagidx + 1;
                continue;
            }
            i += 1;
            continue;
        }
        if ((input[i] == '!' || input[i] == '[') &&
                !currentlineiscode) {
            int maybeimage = (input[i] == '!');
            int title_start, title_len;
            int url_start, url_len;
            int prefix_url_linebreak_to_keep_formatting = 0;
            int imgwidth, imgheight;
            int linklen = _internal_spew3dweb_markdown_GetLinkImgLen(
                input, inputlen, i, opt_forcelinksoneline,
                &title_start, &title_len,
                &url_start, &url_len,
                &prefix_url_linebreak_to_keep_formatting,
                &imgwidth, &imgheight
            );
            if (linklen <= 0) {  // oops, not a link/image:
                if (maybeimage) {
                    // Only escape if it truly looks like an image:
                    if (i + 1 < inputlen &&
                            input[i + 1] == '[') {
                        if (!INS("\\!\\["))
                            goto errorquit;
                        i += 2;
                        continue;
                    }
                    if (!INS("!"))
                        goto errorquit;
                    i += 1;
                    continue;
                }
                assert(input[i] == '[');
                if (!INS("\\["))
                    goto errorquit;
                i += 1;
                continue;
            }
            // If we arrive here, it's a link or image!
            if (maybeimage)
                if (!INS("!["))
                    goto errorquit;
            if (!maybeimage)
                if (!INS("["))
                    goto errorquit;
            size_t i3 = title_start;
            assert(title_start > i);
            assert(title_start + title_len <= inputlen);
            if (title_len > 0) {
                ssize_t result = (
                    _internal_spew3dweb_markdown_AddInlineAreaClean(
                        input, title_start + title_len, title_start,
                        &resultchunk, &resultfill, &resultalloc,
                        origindent, effectiveindent,
                        0, 1,
                        // Links must always be squashed or adjusted:
                        (opt_forcelinksoneline ||
                        opt_squashmultiline),
                        (!opt_forcelinksoneline) &&
                        (!opt_squashmultiline),
                        opt_forcelinksoneline,
                        opt_escapeunambiguousentities,
                        opt_allowunsafehtml,
                        opt_stripcomments,
                        NULL, NULL
                    ));
                assert(result == -1 || result == title_start + title_len);
                if (result < 0)
                    goto errorquit;
            }
            if (!INS("]("))
                goto errorquit;
            if (prefix_url_linebreak_to_keep_formatting) {
                if (!INS("\n"))
                    goto errorquit;
                if (!INSREP(" ", effectiveindent))
                    goto errorquit;
            }
            i3 = url_start;
            assert(i3 < inputlen);
            assert(url_start + url_len <= inputlen);
            int url_seen_questionmark = 0;
            char _uribuf_stack[128] = "";
            char *uribuf = _uribuf_stack;
            size_t uribufalloc = sizeof(_uribuf_stack);
            size_t uribuffill = 0;
            int uribufonheap = 0;
            while (i3 < url_start + url_len) {
                if (uribuffill + 5 > uribufalloc) {
                    char *uribufnew = malloc(uribufalloc * 4);
                    if (!uribufnew) {
                        if (uribufonheap) free(uribuf);
                        goto errorquit;
                    }
                    memcpy(uribufnew, uribuf, uribuffill + 1);
                    if (uribufonheap) free(uribuf);
                    uribuf = uribufnew;
                    uribufonheap = 1;
                    uribufalloc *= 4;
                }
                if (input[i3] == '\r' || input[i3] == '\n') {
                    if (input[i3] == '\n' &&
                            i3 + 1 < url_start + url_len &&
                            input[i3] == '\n')
                        i3 += 1;
                    i3 += 1;
                    if (uribuffill <= 0 ||
                            uribuf[uribuffill - 1] != '/') {
                        memcpy(uribuf + uribuffill, "%20", 4);
                        uribuffill += 3;
                    }
                    while (i3 <= url_start + url_len &&
                            (input[i3] == ' ' ||
                            input[i3] == '\t'))
                        i3 += 1;
                    continue;
                } else if (input[i3] == '?') {
                    url_seen_questionmark = 1;
                } else if (input[i3] == '>') {
                    memcpy(uribuf + uribuffill, "%3E", 4);
                    uribuffill += 3;
                    i3 += 1;
                    continue;
                } else if (input[i3] == '<') {
                    memcpy(uribuf + uribuffill, "%3C", 4);
                    uribuffill += 3;
                    i3 += 1;
                    continue;
                } else if (input[i3] == ' ') {
                    if (!url_seen_questionmark) {
                        memcpy(uribuf + uribuffill, "%20", 4);
                        uribuffill += 3;
                    } else {
                        memcpy(uribuf + uribuffill, "+", 2);
                        uribuffill += 1;
                    }
                    i3 += 1;
                    continue;
                } else if (input[i3] == '\t') {
                    memcpy(uribuf + uribuffill, "%09", 4);
                    uribuffill += 3;
                    i3 += 1;
                    continue;
                }
                uribuf[uribuffill] = input[i3];
                uribuf[uribuffill + 1] = '\0';
                uribuffill += 1;
                i3 += 1;
            }
            if (opt_uritransformcallback != NULL) {
                char *transformed_uri = (
                    opt_uritransformcallback(uribuf,    
                        opt_uritransform_userdata));
                if (!transformed_uri) {
                    if (uribufonheap) free(uribuf);
                    goto errorquit;
                }
                if (uribufonheap) free(uribuf);
                uribuf = transformed_uri;
                uribufonheap = 1;
            }
            if (!INS(uribuf)) {
                if (uribufonheap) free(uribuf);
                goto errorquit;
            }
            if (uribufonheap) free(uribuf);
            if (!INS(")"))
                goto errorquit;
            i += linklen;
            continue;
        }
        if (!INSC(input[i]))
            goto errorquit;
        i += 1;
    }
    *resultchunkptr = resultchunk;
    *resultfillptr = resultfill;
    *resultallocptr = resultalloc;
    return inputlen;
}

S3DHID ssize_t _internal_spew3dweb_markdown_GetInlineEndBracket(
        const char *input, size_t inputlen,
        size_t offset, char closebracket,
        int opt_trim_linebreaks_from_content,
        int *out_spacingstart, int *out_spacingend
        ) {
    int isurl = (closebracket == ')');
    int isinlinecode = (closebracket == '`');
    int roundbracketnest = 0;
    int squarebracketnest = 0;
    int curlybracketnest = 0;
    int sawslash = 0;
    size_t i = offset;
    int isfirstline = 1;
    int currentlyemptyline = 1;
    while (i < inputlen && (input[i] != closebracket ||
            roundbracketnest > 0 ||
            squarebracketnest > 0 ||
            curlybracketnest > 0)) {
        int wascurrentlyemptyline = currentlyemptyline;
        if (input[i] == '\r' || input[i] == '\n') {
            if (currentlyemptyline && !isfirstline)
                // Line break on empty line? Looks invalid, bail.
                return -1;
            if (isinlinecode)
                // Not allowed in inline code.
                return -1;
            if (input[i] == '\r' && i + 1 < inputlen &&
                    input[i] == '\n')
                i += 1;
            i += 1;
            isfirstline = 0;
            currentlyemptyline = 1;
            continue;
        }
        if (input[i] != ' ' && input[i] != '\t')
            currentlyemptyline = 0;

        if ((input[i] == '-' || input[i] == '=') &&
                wascurrentlyemptyline && !isinlinecode &&
                _lineonlycontinuouschar(
                    input, inputlen, i, input[i]
                ))
            // Looks like a --- or === header.
            return -1;
        if (input[i] == '#' && i + 1 < inputlen &&
                wascurrentlyemptyline && !isinlinecode &&
                (input[i + 1] == ' ' || input[i + 1] == '\t' ||
                input[i + 1] == '#'))
            // Looks like a # ..text.. header.
            return -1;
        if (input[i] == ' ' && i + 2 < inputlen && isurl &&
                input[i + 1] == ' ' && !currentlyemptyline &&
                (input[i + 2] != ' ' && input[i + 2] != '\t' &&
                input[i + 2] != '\r' && input[i + 2] != '\n'))
            // Odd long spacing in an URL -> probably not an URL.
            return -1;
        if (isurl && (input[i] == '<' || input[i] == '>'))
            return -1;
        if (input[i] == '/')
            sawslash = 1;
        if ((input[i] == '(' || input[i] == '[') &&
                isurl && !sawslash &&
                (i == offset || input[i - 1] == ' ' ||
                input[i - 1] == '\t' ||
                input[i - 1] == '\r' ||
                input[i - 1] == '\n'))
            // Looks more like text than an URL, doesn't it?
            return -1;
        if (isurl && input[i] == '(') roundbracketnest += 1;
        if (isurl && input[i] == '[') squarebracketnest += 1;
        if (isurl && input[i] == '{') curlybracketnest += 1;
        if (isurl && input[i] == ')') roundbracketnest -= 1;
        if (isurl && input[i] == ']') squarebracketnest -= 1;
        if (isurl && input[i] == '}') curlybracketnest -= 1;
        if (roundbracketnest < 0 ||
                squarebracketnest < 0 ||
                curlybracketnest < 0)
            return -1;
        if (input[i] == '`' && i + 2 < inputlen &&
                input[i + 1] == '`' && input[i + 2] == '`')
            // We ran into a code block element, looks invalid, bail.
            return -1;
        if (input[i] == '|' && !isinlinecode && (
                input[i - 1] == ' ' || input[i - 1] == '\r' ||
                input[i - 1] == '\t') && input[i + 1] == '-')
            // Looks like we ran into a markdown table, bail.
            return -1;
        if (!isurl && !isinlinecode && (
                (closebracket == ']' && input[i] == '[')))
            // Title bracket inside a bracket? Probably not valid.
            return -1;
        if (input[i] == '\\' && !isurl && !isinlinecode &&
                i < inputlen &&
                (input[i + 1] == '[' ||
                input[i + 1] == ']' || input[i + 1] == '|')) {
            // Escaped item in title is fine.
            i += 2;
            continue;
        }
        i += 1;
    }
    if (i >= inputlen || input[i] != closebracket)
        return -1;
    size_t endbracketidx = i;

    size_t spacing_start = 0;
    size_t spacing_end = 0;
    i = offset;
    while (i < inputlen && (input[i] == ' ' ||
            input[i] == '\t' || input[i] == '\r' ||
            input[i] == '\n')) {
        spacing_start += 1;
        i += 1;
    }
    i = endbracketidx;
    while (i >= 0 && (input[i] == ' ' ||
            input[i] == '\t' || input[i] == '\r' ||
            input[i] == '\n')) {
        spacing_end += 1;
        i -= 1;
    }
    if (spacing_start + spacing_end > (endbracketidx - offset)) {
        // Empty contents.
        assert(spacing_start >= (endbracketidx - offset));
        spacing_end = 0;
    }
    if (out_spacingstart) *out_spacingstart = spacing_start;
    if (out_spacingend) *out_spacingend = spacing_end;
    return i;
}

S3DHID int _internal_spew3dweb_markdown_GetLinkImgLen(
        const char *input, size_t inputlen, size_t offset,
        int opt_trim_linebreaks_from_content,
        int *out_title_start, int *out_title_len,
        int *out_url_start, int *out_url_len,
        int *out_prefix_url_linebreak_to_keep_formatting,
        int *out_img_width, int *out_img_height
        ) {
    int title_start, title_pastend,
        url_start, url_pastend;

    size_t i = offset;
    if (i >= inputlen || (
            input[i] != '[' && input[i] != '!'))
        return 0;
    int isimg = (input[i] == '!');
    if (isimg)
        i += 1;

    if (i >= inputlen || input[i] != '[')
        return 0;
    i += 1;
    title_start = i;
    int title_spacing_start = 0;
    int title_spacing_end = 0;
    title_pastend = _internal_spew3dweb_markdown_GetInlineEndBracket(
        input, inputlen, i, ']', opt_trim_linebreaks_from_content,
        &title_spacing_start, &title_spacing_end
    );
    if (title_pastend < 0)
        return 0;
    i = title_pastend;
    assert(title_pastend < 0 || (title_pastend >= title_start &&
        input[title_pastend] == ']'));
    title_start += title_spacing_start;
    title_pastend -= title_spacing_end;
    assert(title_pastend >= title_start);
    i += 1;

    int prefix_url_linebreak_to_keep_formatting = 0;
    if (i < inputlen && input[i] != '(') {
        // We only allow this if there follows a line break,
        // and the URL part is the first thing in the next line.
        while (i < inputlen && (
                input[i] == ' ' || input[i] == '\t'))
            i += 1;
        if (i >= inputlen || (input[i] != '\n' &&
                input[i] != '\r'))
            return 0;
        i += 1;
        while (i < inputlen && (
                input[i] == ' ' || input[i] == '\t'))
            i += 1;
        if (!opt_trim_linebreaks_from_content)
            prefix_url_linebreak_to_keep_formatting = 1;
    }

    if (i >= inputlen || input[i] != '(')
        return 0;
    i += 1;
    url_start = i;
    int url_spacing_start = 0;
    int url_spacing_end = 0;
    url_pastend = _internal_spew3dweb_markdown_GetInlineEndBracket(
        input, inputlen, i, ')', opt_trim_linebreaks_from_content,
        &url_spacing_start, &url_spacing_end
    );
    if (url_pastend < 0)
        return 0;
    i = url_pastend;
    assert(url_pastend < 0 || (url_pastend >= url_start &&
        input[url_pastend] == ')'));
    url_start += url_spacing_start;
    url_pastend -= url_spacing_end;
    if (prefix_url_linebreak_to_keep_formatting &&
            (input[url_start] == '\n' ||
            input[url_start] == '\r'))
        prefix_url_linebreak_to_keep_formatting = 0;
    assert(url_pastend >= url_start);
    i += 1;

    if (out_title_start != NULL)
        *out_title_start = title_start;
    if (out_title_len != NULL)
        *out_title_len = title_pastend - title_start;
    if (out_url_start != NULL)
        *out_url_start = url_start;
    if (out_url_len != NULL)
        *out_url_len = url_pastend - url_start;
    if (out_prefix_url_linebreak_to_keep_formatting != NULL)
        *out_prefix_url_linebreak_to_keep_formatting = (
            prefix_url_linebreak_to_keep_formatting
        );
    if (out_img_width != NULL)
        *out_img_width = -1; //img_width;
    if (out_img_height != NULL)
        *out_img_height = -1; //img_height;

    return (i - offset);
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
        ) {
    char *resultchunk = NULL;
    size_t resultfill = 0;
    size_t resultalloc = 0;
    if (!_ensurebufsize(&resultchunk, &resultalloc, 1))
        return NULL;

    int currentlineisblockinterruptor = 0;
    int in_list_with_orig_indent[MAX_LIST_NESTING] = {0};
    int in_list_with_orig_bullet_indent[MAX_LIST_NESTING] = {0};
    int in_list_logical_nesting_depth = 0;
    int currentlinehadlistbullet = 0;
    int currentlineeffectiveindent = 0;
    int currentlineorigindent = 0;
    int currentlinehadnonwhitespace = 0;
    int currentlinehadnonwhitespaceotherthanbullet = 0;
    int currentlineiscode = 0;
    int lastnonemptylineeffectiveindent = 0;
    int lastnonemptylineorigindent = 0;
    int lastnonemptylinewascode = 0;
    int lastlinewasemptyorblockinterruptor = 0;
    int lastlinehadlistbullet = 0;
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
        if (starts_new_line && i < inputlen &&
                c != '\n' && c != '\r' && (
                i + 1 >= inputlen ||
                input[i + 1] != '\r' ||
                input[i + 1] != '\n')) {
            // The start of a non-empty line:
            currentlinehadlistbullet = 0;
            currentlineisblockinterruptor = 0;
            currentlinehadnonwhitespace = 0;
            currentlinehadnonwhitespaceotherthanbullet = 0;
            int out_is_in_list_depth;
            int out_is_code;
            int out_effective_indent;
            int out_write_this_many_spaces;
            int out_content_start;
            int out_orig_indent;
            int out_orig_bullet_indent;
            int out_is_list_entry;
            int out_list_entry_num_value;
            char out_list_bullet_type = '\0';
            _internal_spew3dweb_markdown_IsListOrCodeIndentEx(
                i, input, inputlen,
                lastnonemptylineorigindent,
                lastnonemptylineeffectiveindent,
                lastnonemptylinewascode,
                lastlinewasemptyorblockinterruptor,
                in_list_with_orig_indent,
                in_list_with_orig_bullet_indent,
                in_list_logical_nesting_depth,
                &out_is_in_list_depth,
                &out_is_code,
                &out_effective_indent,
                &out_write_this_many_spaces,
                &out_content_start,
                &out_orig_indent,
                &out_orig_bullet_indent,
                &out_is_list_entry,
                &out_list_bullet_type,
                &out_list_entry_num_value
            );
            assert(!out_is_code ||
                out_effective_indent >= 4);
            assert(out_write_this_many_spaces >= 0);
            assert(out_is_in_list_depth >= 0 &&
                out_is_in_list_depth <=
                in_list_logical_nesting_depth + 1);
            currentlineiscode = out_is_code;
            if (out_is_in_list_depth > MAX_LIST_NESTING)
                out_is_in_list_depth = MAX_LIST_NESTING;
            if (out_is_in_list_depth >
                    in_list_logical_nesting_depth) {
                in_list_logical_nesting_depth = out_is_in_list_depth;
                in_list_with_orig_indent[out_is_in_list_depth - 1] = (
                    out_orig_indent
                );
                in_list_with_orig_bullet_indent[
                    out_is_in_list_depth - 1
                ] = out_orig_bullet_indent;
            }
            int isonlywhitespace = (!out_is_list_entry && (
                i + out_content_start >= inputlen || (
                input[i + out_content_start] == '\n' ||
                input[i + out_content_start] == '\r')
            ));
            assert(out_is_in_list_depth <= 0 || (
                out_effective_indent > 0 &&
                (out_write_this_many_spaces > 0 || isonlywhitespace)));
            if (isonlywhitespace) {
                // It had only whitespace, that counts as empty.
                assert(out_content_start > 0);
                i += out_content_start;
                lastlinewasemptyorblockinterruptor = 1;
                continue;
            }
            assert(i + out_content_start < inputlen);
            if (!lastlinewasemptyorblockinterruptor &&
                    out_is_code && !lastnonemptylinewascode &&
                    resultfill > 0  // <- We had a previous line.
                    ) {
                // Some markdown interpreters hate if a regular text
                // block is followed up by a 4 space code indent.
                // Therefore, make sure to have a separating line.
                if (!INS("\n"))
                    return NULL;
            } else if (!out_is_list_entry &&
                    out_is_in_list_depth == 0 &&
                    _linestandaloneheadingdepth(
                        input, inputlen,
                        i + out_content_start
                    ) == 0 &&
                    !_lineonlycontinuouschar(input, inputlen,
                        i + out_content_start, '-') &&
                    lastlinehadlistbullet) {
                // Some markdown interpreters will treat a significant
                // dedent breaking out of a list item visually still
                // as a continuation, so insert spacing here.
                if (!INS("\n"))
                    return NULL;
            }
            lastlinewasemptyorblockinterruptor = 0;
            if (!INSREP(" ", out_write_this_many_spaces))
                return NULL;
            if (out_is_list_entry) {
                currentlinehadlistbullet = 1;
                currentlinehadnonwhitespace = 1;
                assert(!out_is_code);
                if (out_list_entry_num_value >= 0) {
                    char buf[10] = {0};
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
                    if (!INSC(out_list_bullet_type))
                        return NULL;
                    if (!INS(" "))
                        return NULL;
                }
                assert(out_content_start > 0);
            }
            lastnonemptylinewascode = out_is_code;
            currentlineeffectiveindent = (
                out_effective_indent
            );
            currentlineorigindent = out_orig_indent;
            i += out_content_start;
            lastlinewasemptyorblockinterruptor = 0;
            if (out_content_start > 0)
                // We skipped forward, so restart iteration:
                continue;
        }
        if (c == '`' && i + 2 < inputlen && !currentlineiscode &&
                input[i + 1] == '`' && input[i + 2] == '`') {
            // Parse ``` code block.
            int ticks = 3;
            i += 3;
            while (i < inputlen && input[i] == '`') {
                ticks += 1;
                i += 1;
            }
            if (currentlinehadnonwhitespace) {
                // Any opening ``` must be on separate line.
                // Remember to cut trailing space from previous line:
                while (resultfill > 0 && (
                        resultchunk[resultfill - 1] == ' ' ||
                        resultchunk[resultfill - 1] == '\t'))
                    resultfill--;
                if (!INS("\n"))
                    return NULL;
                if (!INSREP(" ", currentlineeffectiveindent))
                    return NULL;
                lastlinehadlistbullet = currentlinehadlistbullet;
            }
            if (!INSREP("`", ticks))
                return NULL;
            currentlinehadnonwhitespace = 1;
            currentlineisblockinterruptor = 1;
            currentlinehadlistbullet = 0;
            currentlineiscode = 0;
            currentlinehadnonwhitespaceotherthanbullet = 1;
            int insidecontentsstart = i;
            int insidecontentsend = -1;
            int langnamelen = (
                spew3dweb_markdown_GetBacktickByteBufLangPrefixLen(
                    input, inputlen, i
                )
            );
            if (langnamelen > 0) {
                if (!INSBUF(input + i, langnamelen))
                    return NULL;
                i += langnamelen;
                insidecontentsstart += langnamelen;
            }
            while (i < inputlen &&
                    (input[i] == ' ' || input[i] == '\t')) {
                i += 1;
                insidecontentsstart += 1;
            }

            // Find inner content area, and find smallest indent:
            // (We're scanning ahead, not writing results yet)
            int isfirstinnerline = 1;
            int firstinnerlineempty = 1;
            int innerlinestart = i;
            int innerlinecurrentindent = 0;
            int smallestinnerindentseen = -1;
            int nonwhitespaceoninnerline = 0;
            int lastinnerlineisblank = 0;
            while (i < inputlen) {
                if (input[i] == '\r' || input[i] == '\n') {
                    if (input[i] == '\r' &&
                            i + 1 < inputlen &&
                            input[i] == '\n')
                        i += 1;
                    i += 1;
                    isfirstinnerline = 0;
                    innerlinestart = i;
                    nonwhitespaceoninnerline = 0;
                    innerlinecurrentindent = 0;
                    continue;
                } else if (input[i] == ' ' &&
                        !nonwhitespaceoninnerline) {
                    innerlinecurrentindent += 1;
                } else if (input[i] == '\t' &&
                        !nonwhitespaceoninnerline) {
                    innerlinecurrentindent += 4;
                } else {
                    if (input[i] == '`') {
                        size_t i2 = i;
                        while (i2 < inputlen && i2 < i + ticks &&
                                input[i2] == '`')
                            i2 += 1;
                        if (i2 == i + ticks) {
                            lastinnerlineisblank = (
                                !nonwhitespaceoninnerline
                            );
                            insidecontentsend = i;
                            i = i2;
                            break;
                        }
                    }
                    if (!nonwhitespaceoninnerline &&
                            !isfirstinnerline &&
                            (smallestinnerindentseen < 0 ||
                            innerlinecurrentindent <
                            smallestinnerindentseen)) {
                        smallestinnerindentseen = (
                            innerlinecurrentindent
                        );
                    }
                    nonwhitespaceoninnerline = 1;
                    if (isfirstinnerline)
                        firstinnerlineempty = 0;
                }
                i += 1;
            }
            int needlinebreakpastticks = 0;
            while (i < inputlen && (input[i] == ' ' ||
                    input[i] == '\t'))
                i += 1;
            if (i < inputlen && input[i] != '\r' &&
                    input[i] != '\n')
                needlinebreakpastticks = 1;
            // Now actually write out contents:
            int addindentonwrite = 0;
            if (smallestinnerindentseen >= 0 &&
                    smallestinnerindentseen <
                    currentlineorigindent)
                addindentonwrite = (
                    currentlineorigindent -
                    smallestinnerindentseen
                );
            addindentonwrite += (
                currentlineeffectiveindent -
                currentlineorigindent
            );
            size_t i2 = insidecontentsstart;
            if (!firstinnerlineempty) {
                if (!INSC('\n'))
                    return NULL;
                if (!INSREP(" ", currentlineeffectiveindent))
                    return NULL;
            }
            while (i2 < insidecontentsend) {
                if (input[i2] == '\r' || input[i2] == '\n') {
                    if (input[i2] == '\r' &&
                            i2 < insidecontentsend &&
                            input[i2] == '\n')
                        i2 += 1;
                    i2 += 1;
                    if (!INSC('\n'))
                        return NULL;
                    int line_orig_indent = 0;
                    while (i2 < insidecontentsend &&
                                (input[i2] == ' ' || input[i2] == '\t')) {
                        if (input[i2] == ' ') line_orig_indent++;
                        else line_orig_indent += 4;
                        i2 += 1;
                    }
                    if (i2 >= insidecontentsend) {
                        assert(lastinnerlineisblank);
                        break;
                    }
                    int line_want_indent = (
                        line_orig_indent + addindentonwrite
                    );
                    if (line_want_indent < 0)
                        line_want_indent = 0;
                    if (!INSREP(" ", line_want_indent))
                        return NULL;
                    continue;
                } else if (input[i2] == '\0') {
                    if (!INS("�"))
                        return NULL;
                } else {
                    if (!INSC(input[i2]))
                        return NULL;
                }
                i2 += 1;
            }
            if (insidecontentsend < 0) insidecontentsend = inputlen;
            if (!lastinnerlineisblank) {
                if (!INSC('\n'))
                    return NULL;
                if (!INSREP(" ", currentlineeffectiveindent))
                    return NULL;
            }
            i2 = 0;
            while (i2 < ticks) {
                if (!INSC('`'))
                    return NULL;
                i2 += 1;
            }
            if (needlinebreakpastticks) {
                if (!INSC('\n'))
                    return NULL;
                if (!INSREP(" ", currentlineeffectiveindent))
                    return NULL;
            }
            continue;
        }
        if ((c == '-' || c == '=' || c == '#' ||
                c == '*') && !currentlineiscode && (
                !currentlinehadnonwhitespace || (
                currentlinehadlistbullet &&
                !currentlinehadnonwhitespaceotherthanbullet
                ))) {
            int _heading_rating_if_any = 0;
            if (c == '#' &&
                    (_heading_rating_if_any =
                        _linestandaloneheadingdepth(
                            input, inputlen, i
                        )) > 0) {
                // It's a heading using style: # TITLE
                // Skip past the hashtags and spacing.
                int headingnest = 1;
                int i2 = i + 1;
                while (i2 < inputlen && input[i2] == '#') {
                    headingnest += 1;
                    i2 += 1;
                }
                while (i2 < inputlen && (
                        input[i2] == ' ' ||
                        input[i2] == '\t'))
                    i2 += 1;
                int contentstart = i2;
                if (!INSREP("#", headingnest))
                    return NULL;
                if (!INS(" "))
                    return NULL;
                i = i2;
                continue;
            }
            if (c != '*' && c != '#' &&
                    !lastlinewasemptyorblockinterruptor &&
                    currentlineorigindent ==
                    lastnonemptylineorigindent &&
                    !_getlastlinetrulyempty(resultchunk, resultfill) &&
                    ((c == '-' && _lineonlycontinuouschar(
                        input, inputlen, i, '-')) ||
                    (c == '=' && _lineonlycontinuouschar(
                        input, inputlen, i, '=')))) {
                // It's a heading, === or --- style:
                while (i < inputlen &&
                        input[i] != '\n' &&
                        input[i] != '\r')
                    i += 1;
                currentlineisblockinterruptor = 1;
                lastlinewasemptyorblockinterruptor = 1;
                char heading[2] = {0};
                heading[0] = c;
                if (!INSREP(heading, _getlastoutputlinelen(
                        resultchunk, resultfill, 1,
                        lastlinehadlistbullet
                        )))
                    return NULL;
                continue;
            }
            if (c != '#' && i + 2 < inputlen &&
                    input[i + 1] == c &&
                    input[i + 2] == c) {
                // It's possibly a horizontal ruler line.
                int i2 = i + 1;
                while (i2 < inputlen &&
                        input[i2] == c)
                    i2 += 1;
                while (i2 < inputlen &&
                        (input[i2] == ' ' || input[i2] == '\t'))
                    i2 += 1;
                if (i2 >= inputlen ||
                        input[i2] == '\n' ||
                        input[i2] == '\r') {
                    // Confirmed ruler. Output it, but fixed up:
                    i = i2;
                    currentlineisblockinterruptor = 1;
                    lastlinewasemptyorblockinterruptor = 1;
                    if (!INSC(c))
                        return NULL;
                    if (!INSC(c))
                        return NULL;
                    if (!INSC(c))
                        return NULL;
                    continue;
                }
            }
        }
        if (c != '\r' && c != '\n' && i < inputlen) {
            // Handle with advanced inline formatting:
            ssize_t i2 = _internal_spew3dweb_markdown_AddInlineAreaClean(
                input, inputlen, i,
                &resultchunk, &resultfill, &resultalloc,
                currentlineorigindent, currentlineeffectiveindent,
                currentlineiscode, 0, 0, 0,
                opt_forcenolinebreaklinks,
                opt_forceescapeunambiguousentities,
                opt_allowunsafehtml,
                opt_stripcomments,
                opt_uritransformcallback,
                opt_uritransform_userdata
            );
            if (i2 < 0)
                return NULL;
            assert(i2 > i);
            i = i2;
            continue;
        }
        if (c == '\r' || c == '\n' ||
                i >= inputlen) {
            // Remove trailing space from line written so far:
            while (resultfill > 0 && (
                    resultchunk[resultfill - 1] == ' ' ||
                    resultchunk[resultfill - 1] == '\t'))
                resultfill--;
            // If line written so far wasn't empty, remember indent:
            if (resultfill > 0 &&
                    resultchunk[resultfill - 1] != '\n') {
                // This line wasn't empty.
                lastnonemptylineeffectiveindent = (
                    currentlineeffectiveindent
                );
                lastnonemptylineorigindent = (
                    currentlineorigindent
                );
                lastlinehadlistbullet = (
                    currentlinehadlistbullet
                );
                if (!currentlineisblockinterruptor)
                    lastlinewasemptyorblockinterruptor = 0;
            } else {
                lastlinehadlistbullet = 0;
                lastlinewasemptyorblockinterruptor = 1;
            }
            currentlineiscode = 0;
            currentlinehadnonwhitespace = 0;
            currentlinehadnonwhitespaceotherthanbullet = 0;
            currentlinehadlistbullet = 0;
            currentlineisblockinterruptor = 0;
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
        if (c != ' ' && c != '\t' && i < inputlen) {
            currentlinehadnonwhitespace = 1;
            currentlinehadnonwhitespaceotherthanbullet = 1;
        }
        if (c == '\0' && i < inputlen) {
            if (!INS("�"))
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

S3DEXP char *spew3dweb_markdown_CleanEx(
        const char *inputstr, int opt_allowunsafehtml,
        int opt_stripcomments,
        char *(*opt_uritransformcallback)(
            const char *uri, void *userdata
        ),
        void *opt_uritransform_userdata,
        size_t *out_len
        ) {
    return spew3dweb_markdown_CleanByteBuf(
        inputstr, strlen(inputstr),
        opt_allowunsafehtml,
        opt_stripcomments,
        opt_uritransformcallback,
        opt_uritransform_userdata,
        out_len, NULL
    );
}

S3DEXP char *spew3dweb_markdown_Clean(
        const char *inputstr
        ) { 
    return spew3dweb_markdown_CleanByteBuf(
        inputstr, strlen(inputstr),
        1, 0, NULL, NULL, NULL, NULL
    );  
}

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

static int _spew3d_markdown_process_inline_content(
        char **resultchunkptr, size_t *resultfillptr,
        size_t *resultallocptr,
        _markdown_lineinfo *lineinfo, size_t lineinfofill,
        int startline, int endbeforeline,
        int start_at_content_index,
        int as_code, int *out_endlineidx) {
    char *resultchunk = *resultchunkptr;
    size_t resultfill = *resultfillptr;
    size_t resultalloc = *resultallocptr;
    int endline = startline;
    while (endline + 1 < endbeforeline &&
            lineinfo[endline + 1].indentlen ==
            lineinfo[startline].indentlen &&
            lineinfo[endline + 1].indentedcontentlen > 0 &&
            (lineinfo[endline + 1].linestart[0] != '-' || (
                lineinfo[endline + 1].indentedcontentlen > 1 &&
                lineinfo[endline + 1].linestart[1] != '-' &&
                lineinfo[endline + 1].linestart[2] != ' ')) &&
            (lineinfo[endline + 1].linestart[0] != '*' || (
                lineinfo[endline + 1].indentedcontentlen > 1 &&
                lineinfo[endline + 1].linestart[1] != '*' &&
                lineinfo[endline + 1].linestart[2] != ' ')) &&
            (lineinfo[endline + 1].linestart[0] != '>' || (
                lineinfo[endline + 1].indentedcontentlen > 1 &&
                lineinfo[endline + 1].linestart[1] != ' ')) &&
            (lineinfo[endline + 1].linestart[0] != '#' || (
                lineinfo[endline + 1].indentedcontentlen > 1 &&
                lineinfo[endline + 1].linestart[1] != '#' &&
                lineinfo[endline + 1].linestart[2] != ' ')) &&
            (lineinfo[endline + 1].linestart[0] != '`' || (
                lineinfo[endline + 1].indentedcontentlen < 3 && (
                lineinfo[endline + 1].linestart[1] != '`' ||
                lineinfo[endline + 1].linestart[2] != '`'))))
        endline += 1;
    char fnestings[MAX_FORMAT_NESTING];
    int fnestingsdepth = 0;
    size_t iline = startline;
    while (iline <= endline) {
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
        size_t ipastend = lineinfo[iline].indentedcontentlen;
        const char *linebuf = lineinfo[iline].linestart;
        while (i < ipastend) {
            if (!INSC(linebuf[i]))
                goto errorquit;
            i += 1;
        }
        iline += 1;
    }
    *resultchunkptr = resultchunk;
    *resultfillptr = resultfill;
    *resultallocptr = resultalloc;
    *out_endlineidx = iline - 1;
    return 1;
}

S3DEXP char *spew3dweb_markdown_ByteBufToHTML(
        const char *uncleaninput, size_t uncleaninputlen,
        int opt_allowunsafehtml,
        char *(*opt_uritransformcallback)(
            const char *uri, void *userdata
        ),
        void *opt_uritransform_userdata,
        size_t *out_len
        ) {
    // First, clean up the input:
    size_t inputlen = 0;
    char *input = _internal_spew3dweb_markdown_CleanByteBufEx(
        uncleaninput, uncleaninputlen,
        1, 1, opt_allowunsafehtml, 1,
        opt_uritransformcallback,
        opt_uritransform_userdata,
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
                    ((size_t)currentlinecontentstart) -
                        currentlinestart
                );
                lineinfo[lineinfofill].indentedcontentlen -= (
                    lineinfo[lineinfofill].indentlen
                );
            }
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
            continue;
        }
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
    lineinfo[lineinfofill].indentlen = 0;
    lineinfo[lineinfofill].linestart = zerostringbuf;

    // Now process the markdown and spit out HTML:
    char *resultchunk = NULL;
    size_t resultfill = 0;
    size_t resultalloc = 0;
    if (!_ensurebufsize(&resultchunk, &resultalloc, 1)) {
        if (lineinfoheap)
            free(lineinfo);
        free(input);
        return NULL;
    }
    int nestingstypes[MAX_LIST_NESTING] = {0};
    int nestingsdepth = 0;
    int insidecodeindent = -1;
    int lastnonemptynoncodeindent = 0;
    i = 0;
    while (i < lineinfofill) {
        if (lineinfo[i].indentlen <= lastnonemptynoncodeindent - 4 ||
                (insidecodeindent > 0 &&
                lineinfo[i].indentlen <= insidecodeindent - 4)) {
            // We're leaving a higher nesting, either list or code.
            if (insidecodeindent >= 0) {
                if (!INS("</code></pre>\n"))
                    goto errorquit;
            }
            int nestreduce = (lastnonemptynoncodeindent -
                lineinfo[i].indentlen) / 4;
            while (nestreduce > 0) {
                if (nestingstypes[i] == '-' || nestingstypes[i] == '*') {
                    if (!INS("</li>\n")) {
                        errorquit: ;
                        if (lineinfoheap)
                            free(lineinfo);
                        free(input);
                        return NULL;
                    }
                } else if (nestingstypes[i] == '1') {
                    if (!INS("</li>\n"))
                        goto errorquit;
                } else if (nestingstypes[i] == '>') {
                    if (!INS("</blockquote>\n"))
                        goto errorquit;
                } else {
                    assert(0);  // Should never happen.
                }
                nestreduce -= 1;
            }
        }
        if (insidecodeindent < 0) {
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
            } else if (lineinfo[i].indentlen >=
                    lastnonemptynoncodeindent + 2 &&
                    lineinfo[i].indentedcontentlen >= 2 && (
                    lineinfo[i].linestart[lineinfo[i].indentlen] == '-' ||
                    lineinfo[i].linestart[lineinfo[i].indentlen] == '*' ||
                    lineinfo[i].linestart[lineinfo[i].indentlen] == '>') &&
                    lineinfo[i].linestart[lineinfo[i].indentlen + 1] == ' ') {
                // Start of list entry!
                char bullettype = (
                    lineinfo[i].linestart[lineinfo[i].indentlen]
                );
                lastnonemptynoncodeindent = (
                    lineinfo[i].indentlen + 2  // (indent of actual list)
                );
                if (nestingsdepth + 1 < MAX_LIST_NESTING) {
                    nestingsdepth += 1;
                    nestingstypes[nestingsdepth - 1] = bullettype;
                    if (bullettype != '>') {
                        if (!INS("<li>"))
                            goto errorquit;
                    } else {
                        if (!INS("<blockquote>"))
                            goto errorquit;
                    }
                }
                lineinfo[i].indentlen += 2;
                lineinfo[i].indentedcontentlen -= 2;
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
                if (i2 < lineinfo[i].indentedcontentlen && (
                        lineinfo[i].linestart[i2] == ' ' ||
                        lineinfo[i].linestart[i2] == '\t')) {
                    while (i2 < lineinfo[i].indentedcontentlen && (
                            lineinfo[i].linestart[i2] == ' ' ||
                            lineinfo[i].linestart[i2] == '\t')) {
                        i2 += 1;
                    }
                    if (i2 < lineinfo[i].indentedcontentlen &&
                            lineinfo[i].linestart[i2] != '#') {
                        // This is indeed a heading. Process insides:
                        if (!INS("<h"))
                            goto errorquit;
                        if (!INSC('0' + headingtype))
                            goto errorquit;
                        if (!INS(">"))
                            goto errorquit;
                        int endlineidx = -1;
                        if (!_spew3d_markdown_process_inline_content(
                                &resultchunk, &resultfill, &resultalloc,
                                lineinfo, lineinfofill, i, i + 1,
                                i2,
                                0, &endlineidx))
                            goto errorquit;
                        if (!INS("</h"))
                            goto errorquit;
                        if (!INSC('0' + headingtype))
                            goto errorquit;
                        if (!INS(">"))
                            goto errorquit;
                        assert(endlineidx == i);
                        i += 1;
                        continue;
                    }
                }
            }
        }
        if (insidecodeindent >= 0) {
            // First, handle the indent but relative to the code base:
            // FIXME.
            // Copy single line with no unnecessary formatting changes:
            int endlineidx = -1;
            if (!_spew3d_markdown_process_inline_content(
                    &resultchunk, &resultfill, &resultalloc,
                    lineinfo, lineinfofill, i, i + 1, 0,
                    1 /* as code, no formatting */, &endlineidx))
                goto errorquit;
            if (!INS("\n"))
                goto errorquit;
        } else if (insidecodeindent < 0) {
            // Add in regular inline content:
            lastnonemptynoncodeindent = lineinfo[i].indentlen;
            if (!INS("<p>"))
                goto errorquit;
            int endlineidx = -1;
            if (!_spew3d_markdown_process_inline_content(
                    &resultchunk, &resultfill, &resultalloc,
                    lineinfo, lineinfofill, i, lineinfofill, 0,
                    0, &endlineidx))
                goto errorquit;
            assert(endlineidx >= i);
            i = endlineidx;
            if (!INS("</p>\n"))
                goto errorquit;
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
        int opt_allowunsafehtml,
        char *(*opt_uritransformcallback)(
            const char *uri, void *userdata
        ),
        void *opt_uritransform_userdata,
        size_t *out_len
        ) {
    return spew3dweb_markdown_ByteBufToHTML(
        uncleaninput, strlen(uncleaninput),
        opt_allowunsafehtml,
        opt_uritransformcallback,
        opt_uritransform_userdata,
        out_len
    );
}

S3DEXP char *spew3dweb_markdown_ToHTML(
        const char *uncleaninput
        ) {
    return spew3dweb_markdown_ByteBufToHTML(
        uncleaninput, strlen(uncleaninput),
        1, NULL, NULL, NULL
    );
}

#endif  // SPEW3DWEB_IMPLEMENTATION

