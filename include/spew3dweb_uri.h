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

#ifndef SPEW3DWEB_URI_H_
#define SPEW3DWEB_URI_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>  // for size_t

/// A struct representing the parsed elements of an URI resource.
/// Please that the port can be -1 if not specified, and the
/// querystring can be NULL if not specified.
typedef struct s3d_uriinfo {
    char *protocol;
    char *host;
    int port;
    char *resource;
    char *querystring, *anchor;
} s3d_uriinfo;

/// This function parses a string that is known to somehow refer
/// to a resource, but where it might be both a local disk file path
/// or a remote URI, and users will commonly want to use both.
/// However, full URIs with proper `://` protocol header will
/// always be parsed as such.
///
/// **Warning:** This function is eager to guess the
/// argument to be a file path if there's no opposing evidence. So
/// a website's relative URIs without protocol, like
/// `subfolder/test.html`, may be wrongly guessed as a disk path.
/// Nevertheless, on Linux it's possible to name files to contain
/// a literal `://` URI in which case it's still not guaranteed to
/// be picked up as a file path.
///
/// @param uri_or_file_str The URI or file path string to be parsed.
/// @param default_remote_protocol Best set to `"https"`,
///   you know from context the user will likely assume another default.
/// @returns The parsed result, or NULL on allocation failure.
///   The parser always guesses something, so there's no parse failure
///   other than out of memory.
s3d_uriinfo *s3d_uri_ParseURIOrPath(
    const char *uri_or_file_str,
    const char *default_remote_protocol
);

/// This function is like the s3d_uri_ParseURIOrPath function, but
/// it will always assume the parsed string is a remote or relative
/// URI, even if it extremely looks like a disk file path
/// (like a `"C:\"` Windows file path start).
///
/// **Security warning:** This function can still return a file
/// path if the given parsed string literally begins with `"file://"`.
///
/// @param uri_or_file_str The URI string to be parsed.
/// @param default_remote_protocol Best set to `"https"`, unless
///   you know from context the user will likely assume another default.
/// @returns The parsed result, or NULL on allocation failure.
///   The parser always guesses something, so there's no parse failure
///   other than out of memory.
s3d_uriinfo *s3d_uri_ParseURI(
    const char *uristr,
    const char *default_remote_protocol
);

S3DHID s3d_uriinfo *_internal_s3d_uri_ParseEx(
    const char *uristr,
    const char *default_remote_protocol,
    const char *default_relative_path_protocol,
    int never_assume_absolute_file_path,
    int disable_windows_separators
);

/// Normalize a given remote URI string that was already URI encoded.
///
/// @param ensure_absolute_file_paths If the URI string represents
///     `"file://"` URI but with a relative path, resolve to an absolute
///     path given the working directory.
/// @returns The normalized URI string, or NULL if out of memory.
char *s3d_uri_Normalize(const char *uristr, int absolutefilepaths);

/// Destroy the given `s3d_uriinfo` struct, freeing memory of all
/// members.
void s3d_uri_Free(s3d_uriinfo *uri);

/// Return an URI percent encoded string from the given unencoded
/// resource string.
///
/// @param resource The resource string to be URL percent encoded.
/// @returns The URI percent encoded string, or NULL if out of memory.
S3DEXP char *s3d_uri_PercentEncodeResource(const char *resource);

/// Like @{s3d_uri_PercentEncodeResource}, but more options.
///
/// @param resource The resource string to be URL percent encoded.
/// @param donttouchbefore A byte offset before which the string
///   should be left untouched, even if containing something that
///   would normally use URL percent encoding.
/// @returns the URI percent encoded string, or NULL if out of memory.
S3DEXP char *s3d_uri_PercentEncodeResourceEx(
    const char *path, int donttouchbefore
);

/// Convert an `s3d_uriinfo` struct back into an URI string.
///
/// @param uinfo The struct to be converted back to a string.
/// @param ensure_absolute_file_paths If the URI string represents
///     `"file://"` URI but with a relative path, resolve to an absolute
///     path given the working directory.
/// @returns The resulting string, or NULL if out of memory.
S3DEXP char *s3d_uri_ToStrEx(
    s3d_uriinfo *uinfo,
    int ensure_absolute_file_paths
);

/// This function works like `s3d_uri_ToStrEx`, but it will leave
/// relative file paths alone.
S3DEXP char *s3d_uri_ToStr(s3d_uriinfo *uri);

/// Check if the URI's resource has the given file extension or not.
/// @returns 1 if the extension matches, 0 if not.
S3DEXP int s3d_uri_HasFileExtension(
    s3d_uriinfo *uri, const char *extension
);

/// Set the URI's resource to have the given file extension.
/// If it had a previous file extension that one will be truncated
/// first.
/// @returns 0 on out of memory which will leave the URI unchanged,
///   otherwise 1.
S3DEXP int s3d_uri_SetFileExtension(
    s3d_uriinfo *uri, const char *new_extension
);

#endif  // SPEW3DWEB_URI_H_

