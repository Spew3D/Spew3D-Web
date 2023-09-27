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

static int _insdel(
        char **bufptr, size_t *buffill, size_t *bufalloc,
        size_t insert_at, const char *insert, size_t delchars
        ) {
    char *buf = *bufptr;
    size_t oldfill = *buffill;
    size_t insertlen = (insert != NULL ? strlen(insert) : 0);
    if (insertlen == 0 && delchars == 0) {
        // Nothing to do!
        return 1;
    }
    assert(delchars + insert_at <= oldfill);
    size_t newfill = oldfill - delchars + insertlen;
    if (newfill + 1 > *bufalloc) {
        // We'll need more space to not overrun.
        size_t newalloc = newfill + (oldfill / 4) + 512;
        char *newbuf = realloc(buf, newalloc);
        if (!newbuf) {
            free(buf);
            return 0;
        }
        buf = newbuf;
        *bufptr = newbuf;
        *bufalloc = newalloc;
    }
    // Check what shifts around after our insert and deletion point:
    size_t shiftsectionlen = (oldfill - insert_at) - delchars;
    int64_t shiftsectionamount = (
        -((int64_t) delchars) + (int64_t)insertlen
    );
    // Do the shifting:
    if (shiftsectionlen > 0 && shiftsectionamount != 0)
        memmove(buf + insert_at + delchars,
            buf + (int64_t)(((int64_t)insert_at + delchars) +
            shiftsectionamount),
            shiftsectionlen);
    // Write the inserted part:
    memcpy(buf + insert_at, insert, insertlen);
    *buffill = newfill;
    return 1;
}

char *spew3dweb_markdown_CleanChunk(
        char **chunkptr, size_t *chunkfillptr,
        size_t *chunkallocptr,
        int opt_makecstring, int opt_cleanindent
        ) {
    char *chunk = *chunkptr;
    size_t chunkfill = *chunkfillptr;
    size_t chunkalloc = *chunkallocptr;
    if (chunkfill >= chunkalloc && opt_makecstring) {
        // Make room for null terminator
        char *newchunk = realloc(chunk, chunkalloc + 128);
        if (!newchunk) {
            free(chunk);
            return NULL;
        }
        chunkalloc += 128;
        chunk = newchunk;
    }

    size_t i = 0;
    while (i < chunkfill) {

        i++;
    }
}


#endif  // SPEW3DWEB_IMPLEMENTATION

