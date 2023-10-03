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

static char *_internal_s3d_uri_ParsePath(
        const char *escaped_path,
        int maxlen,
        int enforce_leading_separator,
        int allow_windows_separator
        ) {
    char *unescaped_path = strdup(escaped_path);
    unsigned int i = 0;
    while (i < strlen(unescaped_path) &&
            (maxlen < 0 || i < maxlen)) {
        if (allow_windows_separator &&
                unescaped_path[i] == '\\') {
            unescaped_path[i] = '/';
            i += 1;
        }
        if (unescaped_path[i] == '%' &&
                ((unescaped_path[i + 1] >= '0' &&
                  unescaped_path[i + 1] <= '9') ||
                 (unescaped_path[i + 1] >= 'a' &&
                  unescaped_path[i + 1] <= 'a') ||
                 (unescaped_path[i + 1] >= 'A' &&
                  unescaped_path[i + 1] <= 'Z')) &&
                ((unescaped_path[i + 2] >= '0' &&
                  unescaped_path[i + 2] <= '9') ||
                 (unescaped_path[i + 2] >= 'a' &&
                  unescaped_path[i + 2] <= 'z') ||
                 (unescaped_path[i + 2] >= 'A' &&
                  unescaped_path[i + 2] <= 'Z'))
                ) {
            char hexvalue[3];
            hexvalue[0] = unescaped_path[i + 1];
            hexvalue[1] = unescaped_path[i + 2];
            hexvalue[2] = '\0';
            int number = (int)strtol(hexvalue, NULL, 16);
            if (number > 255 || number == 0)
                number = '?';
            ((uint8_t*)unescaped_path)[i] = (uint8_t)number;
            memmove(
                &unescaped_path[i + 1], &unescaped_path[i + 3],
                strlen(unescaped_path) - (i + 3) + 1
            );
        }
        i++;
    }
    unescaped_path[i] = '\0';
    if (unescaped_path[0] != '/' && enforce_leading_separator) {
        if (allow_windows_separator &&
                unescaped_path[0] == '\\') {
            unescaped_path[0] = '/';
        } else {
            char *unescaped_path_2 = malloc(
                strlen(unescaped_path) + 2
            );
            unescaped_path_2[0] = '/';
            memcpy(unescaped_path_2 + 1, unescaped_path,
                   strlen(unescaped_path) + 1);
            free(unescaped_path);
            return unescaped_path_2;
        }
    }
    return unescaped_path;
}

static int _path_is_typical_unix_abs_path(const char *path) {
    char *signalstarts[] = {
        "/home/", "/usr/", "/etc/", "/root/", "/proc/",
        "/var/", "/media/", "/run/user/", "/mnt/media/",
        NULL,
    };
    int i = 0;
    while (signalstarts[i] != NULL) {
        if (strlen(path) > strlen(signalstarts[i]) &&
                memcmp(path, signalstarts[i],
                       strlen(signalstarts[i])) == 0)
            return 1;
        i += 1;
    }
    return 0;
}

static int _uri_set_resource_from_uri_encoded_str(
        s3duri *result, const char *known_protocol,
        const char *encodedpath, int allow_windows_separator
        ) {
    int querystringstart = 0;
    while (encodedpath[querystringstart] != '\0') {
        if (encodedpath[querystringstart] == '?' ||
                encodedpath[querystringstart] == '#')
            break;
        querystringstart += 1;
    }
    int anchorstringstart = querystringstart;
    while (encodedpath[anchorstringstart] != '\0') {
        if (encodedpath[anchorstringstart] == '#')
            break;
        anchorstringstart += 1;
    }
    if (known_protocol != NULL &&
            strcasecmp(known_protocol, "file") == 0) {
        // These don't have query strings.
        querystringstart = anchorstringstart;
    }
    result->resource = _internal_s3d_uri_ParsePath(
        encodedpath, querystringstart,
        (!result->protocol ||
         strcasecmp(result->protocol, "file") != 0),
        (allow_windows_separator &&
         strcasecmp(result->protocol, "file") == 0)
    );
    if (!result->resource) return 0;
    if (encodedpath[querystringstart] == '?') {
        result->querystring = _internal_s3d_uri_ParsePath(
            encodedpath + querystringstart,
            anchorstringstart - querystringstart,
            0,  // Leave separators alone since truly path.
            0  // Leave separators.
        );
        if (!result->querystring) {
            return 0;
        }
    }
    if (encodedpath[anchorstringstart] == '#') {
        result->anchor = _internal_s3d_uri_ParsePath(
            encodedpath + anchorstringstart, -1,
            0,  // Leave separators alone since truly path.
            0  // Leave separators.
        );
        if (!result->anchor) {
            return 0;
        }
    }
    return 1;
}

S3DHID s3duri *_internal_s3d_uri_ParseEx(
        const char *uristr,
        const char *default_remote_protocol,
        const char *default_relative_path_protocol,
        int never_assume_absolute_file_path,
        int disable_windows_separator
        ) {
    if (!uristr)
        return NULL;
    if (strcasecmp(default_remote_protocol, "file") == 0)
        return NULL;  // This will otherwise break things.
    int allow_windows_separator = !disable_windows_separator;

    s3duri *result = malloc(sizeof(*result));
    if (!result)
        return NULL;
    memset(result, 0, sizeof(*result));
    result->port = -1;

    int lastdotindex = -1;
    const char *part_start = uristr;
    const char *part = uristr;
    while (*part != ' ' && *part != ';' && *part != ':' &&
            *part != '/' && *part != '\\' && *part != '\0' &&
            *part != '\n' && *part != '\r' && *part != '\t' &&
            *part != '@' && *part != '*' && *part != '&' &&
            *part != '%' && *part != '#' && *part != '$' &&
            *part != '!' && *part != '"' && *part != '\'' &&
            *part != '(' && *part != ')' && *part != '|') {
        if (*part == '.')
            lastdotindex = (part - part_start);
        part++;
    }
    int autoguessedunencodedfile = 0;
    int recognizedfirstblock = 0;
    if (strlen(part) >= strlen("://") &&
            (memcmp(part, "://", strlen("://")) == 0 ||
             memcmp(part, ":\\\\", strlen(":\\\\")) == 0)) {
        // This is an URI with a protocol in front:
        result->protocol = malloc(part - part_start + 1);
        if (!result->protocol) {
            s3d_uri_Free(result);
            return NULL;
        }
        memcpy(result->protocol, part_start, part - part_start);
        result->protocol[part - part_start] = '\0';
        part += 3;
        lastdotindex = -1;
        part_start = part;
        if (strcasecmp(result->protocol, "file") == 0) {
            if (!_uri_set_resource_from_uri_encoded_str(
                    result, "file", part_start,
                    allow_windows_separator
                    )) {
                s3d_uri_Free(result);
                return NULL;
            }
            char *path_cleaned = spew3d_fs_NormalizeEx(
                result->resource, allow_windows_separator, 0, '/'
            );
            if (!path_cleaned) {
                s3d_uri_Free(result);
                return NULL;
            }
            free(result->resource);
            result->resource = path_cleaned;
            return result;
        }
        recognizedfirstblock = 1;
    } else if (*part == ':' && (part - uristr) == 1 &&
            ((uristr[0] >= 'a' && uristr[0] <= 'z') ||
             (uristr[0] >= 'A' && uristr[0] <= 'Z')) &&
            (*(part + 1) == '/' || *(part + 1) == '\\') &&
            !never_assume_absolute_file_path) {
        // Looks like a Windows absolute path:
        allow_windows_separator = 1;
        result->protocol = strdup("file");
        if (!result->protocol) {
            s3d_uri_Free(result);
            return NULL;
        }
        result->resource = spew3d_fs_NormalizeEx(uristr, 1, 0, '/');
        if (!result->resource) {
            s3d_uri_Free(result);
            return NULL;
        }
        return result;
    } else if (*part == '/' && (part - uristr) == 0 &&
            !never_assume_absolute_file_path &&
            _path_is_typical_unix_abs_path(part)) {
        // Looks like a Unix absolute path:
        allow_windows_separator = 0;
        result->protocol = strdup("file");
        if (!result->protocol) {
            s3d_uri_Free(result);
            return NULL;
        }
        result->resource = spew3d_fs_NormalizeEx(uristr, 0, 1, '/');
        if (!result->resource) {
            s3d_uri_Free(result);
            return NULL;
        }
        return result;
    } else {
        recognizedfirstblock = 0;
    }

    if (recognizedfirstblock) {
        while (*part != ' ' && *part != ';' &&
                *part != '/' && *part != '\\' && *part != '\0' &&
                *part != '\n' && *part != '\r' && *part != '\t' &&
                *part != '@' && *part != '*' && *part != '&' &&
                *part != '%' && *part != '#' && *part != '$' &&
                *part != '!' && *part != '"' && *part != '\'' &&
                *part != '(' && *part != ')' && *part != '|' &&
                *part != '?') {
            if (*part == '.')
                lastdotindex = (part - part_start);
            part++;
        }
    }

    if (*part == ':' &&
            (*(part + 1) >= '0' && *(part + 1) <= '9') &&
            lastdotindex > 0) {
        // Looks like we've had the host followed by port:
        if (!result->protocol) {
            if (default_remote_protocol) {
                result->protocol = strdup(
                    default_remote_protocol
                );
                if (!result->protocol) {
                    s3d_uri_Free(result);
                    return NULL;
                }
            } else {
                result->protocol = NULL;
            }
        }
        result->host = malloc(part - part_start + 1);
        if (!result->host) {
            s3d_uri_Free(result);
            return NULL;
        }
        memcpy(result->host, part_start, part - part_start);
        result->host[part - part_start] = '\0';
        part++;
        part_start = part;
        lastdotindex = -1;
        while (*part != '\0' &&
                (*part >= '0' && *part <= '9'))
            part++;
        char *portbuf = malloc(part - part_start + 1);
        if (!portbuf) {
            s3d_uri_Free(result);
            return NULL;
        }
        memcpy(portbuf, part_start, part - part_start);
        portbuf[part - part_start] = '\0';
        result->port = atoi(portbuf);
        free(portbuf);
        part_start = part;
        lastdotindex = -1;
    } else if ((*part == '/' || *part == '\0' ||
            *part == '#' || *part == '?') &&
            result->protocol &&
            strcasecmp(result->protocol, "file") != 0) {
        // We've had a protocol in front, so first slash ends host:
        result->host = malloc(part - part_start + 1);
        if (!result->host) {
            s3d_uri_Free(result);
            return NULL;
        }
        memcpy(result->host, part_start, part - part_start);
        result->host[part - part_start] = '\0';
        part_start = part;
        lastdotindex = -1;
    }

    // If we have still no guess, look out for telltale characters:
    if (!result->protocol && !result->host && result->port < 0) {
        int i = 0;
        while (i < strlen(part_start)) {
            if (part_start[i] == '%' ||
                    part_start[i] == '?' || part_start[i] == '&' ||
                    part_start[i] == '#') {
                result->protocol = strdup(default_remote_protocol);
                if (!result->protocol) {
                    s3d_uri_Free(result);
                    return NULL;
                }
                break;
            }
            i += 1;
        }
    }

    // Case if no guess, a random relative thing. Use default then:
    if (!result->protocol && !result->host && result->port < 0) {
        result->protocol = strdup(default_relative_path_protocol);
        if (!result->protocol) {
            s3d_uri_Free(result);
            return NULL;
        }
        if (strcasecmp(result->protocol, "file") == 0) {
            // This was likely a literal file path, not an URI.
            // Therefore, assume it wasn't URI-encoded.
            autoguessedunencodedfile = 1;
        }
    }

    // If needed, undo all the URL encoding:
    if (!autoguessedunencodedfile) {
        if (!_uri_set_resource_from_uri_encoded_str(
                result, result->protocol, part_start,
                allow_windows_separator
                )) {
            s3d_uri_Free(result);
            return NULL;
        }
    } else {
        result->resource = strdup(part_start);
    }
    if (!result->resource) {
        s3d_uri_Free(result);
        return NULL;
    }
    if (result->protocol && strcasecmp(result->protocol, "file") == 0) {
        char *path_cleaned = (spew3d_fs_NormalizeEx(
            result->resource, allow_windows_separator, 0, '/'
        ));
        free(result->resource);
        result->resource = path_cleaned;
    }
    return result;
}

S3DEXP s3duri *s3d_uri_ParseURIOrPath(
        const char *uri,
        const char *default_remote_protocol
        ) {
    return _internal_s3d_uri_ParseEx(uri, "https", "file", 0, 0);
}

S3DEXP s3duri *s3d_uri_ParseURI(
        const char *uri,
        const char *default_remote_protocol
        ) {
    return _internal_s3d_uri_ParseEx(uri, "https", "https", 1, 0);
}

S3DEXP char *s3d_uri_PercentEncodeResource(const char *path) {
    return s3d_uri_PercentEncodeResourceEx(path, 0);
}

S3DEXP char *s3d_uri_PercentEncodeResourceEx(
        const char *path, int donttouchbefore
        ) {
    char *buf = malloc(strlen(path) * 3 + 1);
    if (!buf)
        return NULL;
    int buffill = 0;
    unsigned int i = 0;
    while (i < strlen(path)) {
        if (path[i] == '%' ||
                path[i] == '\\' ||
                path[i] <= 32 ||
                path[i] == ' ' || path[i] == '\t' ||
                path[i] == '[' || path[i] == ']' ||
                path[i] == ':' || path[i] == '?' ||
                path[i] == '&' || path[i] == '=' ||
                path[i] == '\'' || path[i] == '"' ||
                path[i] == '@' || path[i] == '#') {
            char hexval[6];
            snprintf(hexval, sizeof(hexval) - 1,
                "%o", (int)(((uint8_t*)path)[i]));
            hexval[2] = '\0';
            buf[buffill] = '%'; buffill++;
            unsigned int z = strlen(hexval);
            while (z < 2) {
                buf[buffill] = '0'; buffill++;
                z++;
            }
            z = 0;
            while (z < strlen(hexval)) {
                buf[buffill] = hexval[z]; buffill++;
                z++;
            }
        } else {
            buf[buffill] = path[i]; buffill++;
        }
        i++;
    }
    buf[buffill] = '\0';
    return buf;
}

S3DEXP char *s3d_uri_ToStrEx(
    s3duri *uinfo,
    int ensure_absolute_file_paths
);

S3DEXP char *s3d_uri_Normalize(
        const char *uristr,
        int absolutefilepaths) {
    s3duri *uinfo = s3d_uri_ParseURIOrPath(uristr, "https");
    if (!uinfo) {
        return NULL;
    }
    return s3d_uri_ToStrEx(uinfo, absolutefilepaths);
}

S3DEXP char *s3d_uri_ToStr(s3duri *uinfo) {
    return s3d_uri_ToStrEx(uinfo, 0);
}

S3DEXP char *s3d_uri_ToStrEx(
        s3duri *uinfo,
        int ensure_absolute_file_paths) {
    char portbuf[128] = "";
    if (uinfo->port > 0) {
        snprintf(
            portbuf, sizeof(portbuf) - 1,
            ":%d", uinfo->port
        );
    }
    char *path = strdup((uinfo->resource ? uinfo->resource : ""));
    if (!path)
        return NULL;
    if (uinfo->protocol && strcasecmp(uinfo->protocol, "file") == 0 &&
            !spew3d_fs_IsAbsolutePath(path) &&
            ensure_absolute_file_paths &&
            uinfo->resource) {
        char *newpath = spew3d_fs_ToAbsolutePath(path);
        if (!newpath) {
            free(path);
            return NULL;
        }
        free(path);
        path = newpath;
    }
    char *encodedpath = s3d_uri_PercentEncodeResource(path);
    if (!encodedpath) {
        free(path);
        return NULL;
    }
    free(path);
    path = NULL;
    char *encodedquerystr = s3d_uri_PercentEncodeResourceEx(
        (uinfo->querystring ? uinfo->querystring : ""), 1
    );
    if (!encodedquerystr) {
        free(encodedpath);
        free(path);
        return NULL;
    }
    char *encodedanchorstr = s3d_uri_PercentEncodeResourceEx(
        (uinfo->querystring ? uinfo->querystring : ""), 1
    );
    if (!encodedanchorstr) {
        free(encodedquerystr);
        free(encodedpath);
        free(path);
        return NULL;
    }

    int upperboundlen = (
        strlen((uinfo->protocol ? uinfo->protocol : "")) +
        strlen("://") +
        strlen((uinfo->host ? uinfo->host : "")) +
        strlen(portbuf) + strlen(encodedpath) +
        strlen(encodedquerystr) + 1 +
        strlen(encodedanchorstr) + 1 +
        1
    ) + 10;
    char *buf = malloc(upperboundlen);
    if (!buf) {
        free(encodedpath);
        return NULL;
    }
    printf("OOP OOP %s\n", encodedpath);
    printf("xx\n");
    printf("EEP OOP %s\n", uinfo->querystring);
    printf("ERP ERP %s\n", uinfo->anchor);
    snprintf(
        buf, upperboundlen - 1,
        "%s://%s%s%s%s%s%s%s%s%s",
        uinfo->protocol,
        (uinfo->host ? uinfo->host : ""), portbuf,
        ((strlen((uinfo->host ? uinfo->host : "")) > 0 &&
          strlen(encodedpath) > 0 &&
          encodedpath[0] != '/') ? "/" : ""),
        encodedpath,
        ((encodedquerystr[0] != '?') ? "?" : ""),
        encodedquerystr,
        ((encodedanchorstr[0] != '#') ? "#" : ""),
        encodedanchorstr
    );
    free(encodedpath);
    free(encodedquerystr);
    free(encodedanchorstr);
    char *shrunkbuf = strdup(buf);
    free(buf);
    return shrunkbuf;
}

S3DEXP void s3d_uri_Free(s3duri *uri) {
    if (!uri)
        return;
    free(uri->protocol);
    free(uri->host);
    free(uri->resource);
    free(uri->anchor);
    free(uri->querystring);
    free(uri);
}

S3DEXP int s3d_uri_HasFileExtension(
        s3duri *uri, const char *extension
        ) {
    int i = strlen(uri->resource) - 1;
    while (i >= 0 && uri->resource[i] != '.') {
        if (uri->resource[i] == '/')
            break;
        i--;
    }
    if (i < 0 || uri->resource[i] != '.') {
        return (strlen(extension) == 0);
    }
    if (extension[0] != '.' && strlen(extension) > 0)
        i += 1;
    return (strcasecmp(uri->resource + i, extension) == 0);
}

S3DEXP int s3d_uri_SetFileExtension(
        s3duri *uri, const char *new_extension
        ) {
    int i = strlen(uri->resource) - 1;
    while (i >= 0 && uri->resource[i] != '.') {
        if (uri->resource[i] == '/')
            break;
        i--;
    }
    if (i < 0 || uri->resource[i] != '.') {
        i = strlen(uri->resource);
    }
    assert(i >= 0 && i <= strlen(uri->resource));
    char *new_resource = malloc(
        i + 1 + strlen(new_extension) + 1
    );
    if (!new_resource) {
        return 0;
    }
    memcpy(new_resource, uri->resource, i);
    char *write = new_resource + i;
    if (strlen(new_extension) > 0 &&
            new_extension[0] != '.') {
        *write = '.';
        write++;
    }
    memcpy(write, new_extension, strlen(new_extension) + 1);
    assert(write[strlen(new_extension)] == '\0');
    free(uri->resource);
    uri->resource = new_resource;

    return 1;
}

#endif  // SPEW3DWEB_IMPLEMENTATION

