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

#include <assert.h>
#include <check.h>

#define SPEW3D_OPTION_DISABLE_SDL
#define SPEW3D_IMPLEMENTATION
#include "spew3d.h"
#define SPEW3DWEB_IMPLEMENTATION
#include "spew3dweb.h"

#include "testmain.h"

START_TEST (test_uri_parse)
{
    s3duri *result = NULL;
    {
        result = s3d_uri_ParseURIOrPath(
            "file://abc?def#c", "https"
        );
        ck_assert(result != NULL);
        ck_assert_str_eq(result->protocol, "file");
        ck_assert(result->resource != NULL);
        ck_assert_str_eq(result->resource, "abc?def");
        s3d_uri_Free(result);
    }
    {
        result = s3d_uri_ParseURIOrPath(
            "http://abc?def#c", "https"
        );
        ck_assert(result->protocol != NULL);
        ck_assert_str_eq(result->protocol, "http");
        ck_assert(result->resource != NULL);
        ck_assert_str_eq(result->resource, "/");
        ck_assert(result->host != NULL);
        ck_assert_str_eq(result->host, "abc");
        s3d_uri_Free(result);
    }
    {
        result = s3d_uri_ParseURIOrPath(
            "test/LICENSE big.md", "https"
        );
        ck_assert_str_eq(result->protocol, "file");
        ck_assert_str_eq(result->resource, "test/LICENSE big.md");
        ck_assert(result->querystring == NULL);
        ck_assert(result->anchor == NULL);
        char *resultback = s3d_uri_ToStr(result);
        ck_assert_str_eq(resultback, "file://test/LICENSE%20big.md");
        free(resultback);
        s3d_uri_Free(result);
    }
    {
        result = s3d_uri_ParseURI(
            "LICENSE.md", "https"
        );
        ck_assert_str_eq(result->protocol, "https");
        ck_assert_str_eq(result->host, "LICENSE.md");
        ck_assert_str_eq(result->resource, "/");
        s3d_uri_Free(result);
    }
    {
        result = s3d_uri_ParseURIOrPath(
            "LICENSE.md", "https"
        );
        ck_assert_str_eq(result->protocol, "file");
        ck_assert_str_eq(result->host, NULL);
        ck_assert_str_eq(result->resource, "LICENSE.md");
        s3d_uri_Free(result);
    }
}
END_TEST

START_TEST (test_uri_fromfilepath)
{
    {
        s3duri *result = s3d_uri_FromFilePath(
            "LICENSE%20.md"
        );
        ck_assert_str_eq(result->protocol, "file");
        ck_assert_str_eq(result->host, NULL);
        ck_assert_str_eq(result->resource, "LICENSE%2520.md");
        s3d_uri_Free(result);
    }
}
END_TEST

TESTS_MAIN(test_uri_parse, test_uri_fromfilepath)

