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

#include <assert.h>
#include <check.h>

#define SPEW3D_OPTION_DISABLE_SDL
#define SPEW3D_IMPLEMENTATION
#include "spew3d.h"
#define SPEW3DWEB_IMPLEMENTATION
#include "spew3d-web.h"

#include "testmain.h"

START_TEST (test_markdown_chunks)
{
    char testbuf[256] = {0};
    char *result = NULL;
    size_t resultlen = 0;
    {
        result = _internal_spew3dweb_markdown_GetIChunkExFromStr(
            "```abc def ```hello!",
            testbuf, sizeof(testbuf),
            10, 5,
            &resultlen
        );
        assert(result == testbuf);
        assert(resultlen == 10);
    }
    {
        result = _internal_spew3dweb_markdown_GetIChunkExFromStr(
            "```abc\n\ndef ```hello!",
            testbuf, sizeof(testbuf),
            256, 5,
            &resultlen
        );
        assert(result == testbuf);
        assert(resultlen == 21);
    }
    {
        result = _internal_spew3dweb_markdown_GetIChunkExFromStr(
            "``!abc\n\ndef ``?hello!",
            testbuf, sizeof(testbuf),
            256, 5,
            &resultlen
        );
        assert(result == testbuf);
        assert(resultlen == 6);
    }
    {
        result = _internal_spew3dweb_markdown_GetIChunkExFromStr(
            "``!abc\n\n def ``?hello!",
            testbuf, sizeof(testbuf),
            256, 5,
            &resultlen
        );
        assert(result == testbuf);
        assert(resultlen == 22);
    }
    {
        result = _internal_spew3dweb_markdown_GetIChunkExFromStr(
            "```!abc\n\nd````ef\n\n ```\n\n?hello!",
            testbuf, sizeof(testbuf),
            256, 5,
            &resultlen
        );
        assert(result == testbuf);
        assert(resultlen == 22);
    }
}
END_TEST

START_TEST(test_markdown_clean)
{
    char *result;
    {
        result = spew3dweb_markdown_Clean(
            "\tabc\tdef\n  ", NULL, NULL
        );
        printf("test_markdown_clean result #0: <<%s>>\n", result);
        assert(strcmp(result, "    abc\tdef\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "  abc\n    def\n  ", NULL, NULL
        );
        printf("test_markdown_clean result #1: <<%s>>\n", result);
        assert(strcmp(result, "abc\ndef\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "  abc\n      def\n  ", NULL, NULL
        );
        printf("test_markdown_clean result #2: <<%s>>\n", result);
        assert(strcmp(result, "abc\n    def\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "  abc\n  --\n", NULL, NULL
        );
        printf("test_markdown_clean result #3: <<%s>>\n", result);
        assert(strcmp(result, "abc\n---\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "- abc\n      def\n  ", NULL, NULL
        );
        printf("test_markdown_clean result #4: <<%s>>\n", result);
        assert(strcmp(result, "  - abc\n        def\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc\n  > def\n  ", NULL, NULL
        );
        printf("test_markdown_clean result #5: <<%s>>\n", result);
        assert(strcmp(result, "  > abc\n      > def\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc d\n  --\n  aef\n   efaef\n     \n  > test\n    x", NULL, NULL
        );
        printf("test_markdown_clean result #6: <<%s>>\n", result);
        assert(strcmp(result, "  > abc d\n    -----\n    aef\n"
            "    efaef\n\n      > test\n        x") == 0);
        free(result);
    }
}
END_TEST

TESTS_MAIN(test_markdown_chunks, test_markdown_clean)

