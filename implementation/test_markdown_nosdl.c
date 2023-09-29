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
            "\tabc\tdef\n  ", NULL
        );
        printf("test_markdown_clean result #0: <<%s>>\n", result);
        assert(strcmp(result, "    abc\tdef\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "  abc\n    def\n  ", NULL
        );
        printf("test_markdown_clean result #1: <<%s>>\n", result);
        assert(strcmp(result, "abc\ndef\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "  abc\n      def\n  ", NULL
        );  // The code block 'def' must be separated, or some
            // weird parsers won't see it as a code block.
        printf("test_markdown_clean result #2: <<%s>>\n", result);
        assert(strcmp(result, "abc\n\n    def\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "- abc\ndef\n", NULL
        );  // The 'def' must be separated so some weird parsers
            // don't see it as part of the bullet point.
        printf("test_markdown_clean result #3: <<%s>>\n", result);
        assert(strcmp(result, "  - abc\n\ndef\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "- abc\n# def\n", NULL
        );  // The 'def' heading here should be left alone.
        printf("test_markdown_clean result #4: <<%s>>\n", result);
        assert(strcmp(result, "  - abc\n# def\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "  abc\n  --\n", NULL
        );
        printf("test_markdown_clean result #5: <<%s>>\n", result);
        assert(strcmp(result, "abc\n---\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "- abc\n\n      def\n  ", NULL
        );
        printf("test_markdown_clean result #6: <<%s>>\n", result);
        assert(strcmp(result, "  - abc\n\n        def\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc\n===\n  ", NULL
        );
        printf("test_markdown_clean result #7: <<%s>>\n", result);
        assert(strcmp(result, "  > abc\n\n===\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc\n    =\n  ", NULL
        );
        printf("test_markdown_clean result #8: <<%s>>\n", result);
        assert(strcmp(result, "  > abc\n    ===\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc\n  > def\n  ", NULL
        );
        printf("test_markdown_clean result #9: <<%s>>\n", result);
        assert(strcmp(result, "  > abc\n      > def\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc d\n  --\n  aef\n   efaef\n     \n  > test\n    x", NULL
        );
        printf("test_markdown_clean result #10: <<%s>>\n", result);
        assert(strcmp(result, "  > abc d\n    -----\n    aef\n"
            "    efaef\n\n      > test\n        x") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc d\n  -\n", NULL
        );
        printf("test_markdown_clean result #11: <<%s>>\n", result);
        assert(strcmp(result, "  > abc d\n    -----\n") == 0);
        free(result);
    }
}
END_TEST

static int _s3dw_check_html_same(const char *p1, const char *p2) {
    int previous_p1_was_tagclose = 0;
    int previous_p2_was_tagclose = 0;
    while (1) {
        while ((*p1 == ' ' || *p1 == '\t' ||
                *p1 == '\r' || *p1 == '\n') &&
                (*(p1 + 1) == ' ' || *(p1 + 1) == '\t' ||
                *(p1 + 1) == '\r' || *(p1 + 1) == '\n' ||
                *(p1 + 1) == '<' || previous_p1_was_tagclose))
            p1++;
        while ((*p2 == ' ' || *p2 == '\t' ||
                *p2 == '\r' || *p2 == '\n') &&
                (*(p2 + 1) == ' ' || *(p2 + 1) == '\t' ||
                *(p2 + 1) == '\r' || *(p2 + 1) == '\n' ||
                *(p2 + 1) == '<' || previous_p2_was_tagclose))
            p2++;
        /*printf("_s3dw_check_html_same: p1='%c' p2='%c'\n",
            (*p1 != '\0' ? *p1 : '?'),
            (*p2 != '\0' ? *p2 : '?'));*/
        while (*p1 == '\0' &&
                (*p2 == '\r' || *p2 == '\n' ||
                *p2 == ' ' || *p2 == '\t'))
            p2++;
        while (*p2 == '\0' && (*p1 == '\r' || *p2 == '\n' ||
                *p1 == ' ' || *p1 == '\t'))
            p1++;
        if ((*p1) != (*p2)) {
            if ((*p1 != ' ' && *p1 != '\t' &&
                    *p1 != '\r' && *p1 != '\n') ||
                    (*p2 != ' ' && *p2 != '\t' &&
                    *p2 != '\r' && *p2 != '\n'))
                return 0;
        }
        if (*p1 == '\0') {
            assert(*p2 == '\0');
            return 1;
        }
        previous_p1_was_tagclose = ((*p1) == '>');
        previous_p2_was_tagclose = ((*p2) == '>');
        p1++;
        p2++;
    }
}

START_TEST(test_markdown_tohtml)
{
    assert(_s3dw_check_html_same("  <span>test\nbla",
        "<span> test  bla "));
    assert(!_s3dw_check_html_same("  <span>test\nbla",
        "<span> testbla "));
    char *result;
    {
        result = spew3dweb_markdown_ToHTML(
            "# abc\ndef", 1, NULL
        );
        printf("test_markdown_tohtml result #0: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<h1>abc</h1>\n<p>def</p>"));
        free(result);
    }
}
END_TEST

TESTS_MAIN(test_markdown_chunks, test_markdown_clean,
    test_markdown_tohtml)

