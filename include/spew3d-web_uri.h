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
    char *querystring;
} s3d_uriinfo;

/// This function parses a string that is known to somehow refer
/// to a resource, but where it might be both a local file path
/// or a remote URI. You should always use this function if you
/// want to give the user the most flexibility specifying a resource,
/// and accessing local files isn't a security issue.
///
/// **Important assumptions used by this parser:**
///
/// - If there's no vague indications that it's a remote
///   URI and it looks more like a path, the parser assumes
///   it's more likely a file path and not a relative link.
///   So file paths are favored. Whether the file exists on disk
///   or not is irrelevant, the filesystem won't be accessed for
///   parsing.
///
/// - If it looks like a remote URI but there's no
///   protocol, assume the default_remote_protocol given.
///   A good value for default_remote_protocol is `"https"``,
///   unless you know given the source of the URI it's more likely
///   to use a different protocol for remote resources. (Don't set
///   this to `"file"`, it will be applied to URIs that clearly
///   have a port number, etc.)
///
/// - Always guess something, no matter how clueless. This
///   function only returns failure on memory allocation failure.
///
/// **Security warning:** This function is eager to guess a
/// `"file://"` resource if that isn't a somewhat implausible guess,
/// so use this only if local file access is common and reasonable.
///
/// @param uri_or_file_str The URI or file path string to be parsed.
/// @param default_remote_protocol Best set to `"https"`, see above.
/// @returns The parsed result, or NULL on allocation failure.
s3d_uriinfo *s3d_uri_ParseAny(
    const char *uri_or_file_str,
    const char *default_remote_protocol
);

/// This function works like the s3d_uri_ParseURIOrFilePath function,
/// it will always assume the parsed string is a remote or relative
/// URI, if in doubt using the default_remote_protocol if none
/// specified, even if it extremely looks like a path
/// (like a `"C:\"` Windows file path start).
///
/// **Security warning:** This function can still return a file
/// path if the given parsed string literally begins with `"file://"`.
///
/// @param uri_or_file_str The URI string to be parsed.
/// @param default_remote_protocol Best set to `"https"`, see above.
/// @returns The parsed result, or NULL on allocation failure.
s3d_uriinfo *s3d_uri_ParseRemoteOnly(
    const char *uristr,
    const char *default_remote_protocol
);

/// Extended parser version for experts, this function works like
/// s3d_uri_ParseURIOrFilePath but with extended options.
///
/// @param uri_or_file_str The URI or file path to be parsed.
/// @param default_remote_protocol The default protocol
///   that's applied when the string clearly looks remote, but
///   there's no protocol specified.
/// @param default_relative_resource_protocol The default protocol
///   that's applied when the string neither looks remote,
///   nor looks like an absolute file path, but rather like
///   either a relative file path or relative URI.
///   If this is set to `"file"`, such a relative file path will
///   be assumed to not have been URI encoded, otherwise it
///   will be assumed to have been URI encoded.
/// @param never_assume_absolute_file_path Set to 1 to never,
///   ever guess an absolute local file path even if it really
///   badly looks like one. Instead, it will always be assumed
///   to be a relative resource in that case.
/// @param disable_windows_separators Set to 1 if you're fairly
///   certain your users will never, ever supply Windows style
///   paths with `\\` separators, and are less likely to do so
///   than submitting a Unix path that actually has a `/`
///   separator in it. It's not recommended to use this option,
///   since a backslash can still be specified as a `"file://"`
///   with an URL-encoded backslash anyway. However, it might
///   make the life of Unix-only users slightly easier.
s3d_uriinfo *s3d_uri_ParseEx(
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
/// @returns The URI percent encoded string, or NULL if out of memory.
char *s3d_uri_PercentEncodeResource(const char *resource);

/// Convert an `s3d_uriinfo` struct back into an URI string.
///
/// @param uinfo The struct to be converted back to a string.
/// @param ensure_absolute_file_paths If the URI string represents
///     `"file://"` URI but with a relative path, resolve to an absolute
///     path given the working directory.
/// @returns The resulting string, or NULL if out of memory.
char *s3d_uri_ToStrEx(
    s3d_uriinfo *uinfo,
    int ensure_absolute_file_paths);

/// This function works like `s3d_uri_ToStrEx`, but it will leave
/// relative file paths alone.
char *s3d_uri_ToStr(s3d_uriinfo *uri);

#endif  // SPEW3DWEB_URI_H_

