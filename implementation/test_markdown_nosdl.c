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

START_TEST (test_markdown_chunks)
{
    char testbuf[256] = {0};
    char *result = NULL;
    size_t resultlen = 0;
    {
        result = _internal_spew3dweb_markdown_GetIChunkExFromStr(
            "```abc def ```hello!",
            testbuf, sizeof(testbuf),
            10, 5, &resultlen
        );
        assert(result == testbuf);
        assert(resultlen == 10);
    }
    {
        result = _internal_spew3dweb_markdown_GetIChunkExFromStr(
            "```abc\n\ndef ```hello!",
            testbuf, sizeof(testbuf),
            256, 5, &resultlen
        );
        assert(result == testbuf);
        assert(resultlen == 21);
    }
    {
        result = _internal_spew3dweb_markdown_GetIChunkExFromStr(
            "``!abc\n\ndef ``?hello!",
            testbuf, sizeof(testbuf),
            256, 5, &resultlen
        );
        assert(result == testbuf);
        assert(resultlen == 6);
    }
    {
        result = _internal_spew3dweb_markdown_GetIChunkExFromStr(
            "``!abc\n\n def ``?hello!",
            testbuf, sizeof(testbuf),
            256, 5, &resultlen
        );
        assert(result == testbuf);
        assert(resultlen == 22);
    }
    {
        result = _internal_spew3dweb_markdown_GetIChunkExFromStr(
            "```!abc\n\nd````ef\n\n ```\n\n?hello!",
            testbuf, sizeof(testbuf),
            256, 5, &resultlen
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
            "- test\n  - test 2\n- test 3\n\n  bla"
        );
        printf("test_markdown_clean result #1: <<%s>>\n", result);
        assert(strcmp(result,
            "  - test\n      - test 2\n  - test 3\n\n    bla") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "\tab &amp <!-- `&amp` \\n \\ ["
            " c\tdef\n  \n&amp <!-- ` test --> `<!-- &amp` --> [ \\n \\ "
        );
        printf("test_markdown_clean result #2: <<%s>>\n", result);
        assert(strcmp(result,
            "    ab &amp <!-- `&amp` \\n \\ [ c\tdef\n\n"
            "&amp;amp <!-- ` test --> `<!-- &amp` --&gt; \\[ \\n \\") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "basic list\n- item1\n- item 2\n- item 3 oop `fancy`\ndone!"
        );
        printf("test_markdown_clean result #3: <<%s>>\n", result);
        assert(strcmp(result,
            "basic list\n  - item1\n  - item 2\n  - item 3 oop `fancy`\n\ndone!") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "test ![alt image\n]\n(\nmy link)"
        );
        printf("test_markdown_clean result #4: <<%s>>\n", result);
        assert(strcmp(result, "test ![alt image\n](\nmy%20link)") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "test ![alt image\n]\n(my link)"
        );
        printf("test_markdown_clean result #5: <<%s>>\n", result);
        assert(strcmp(result, "test ![alt image\n](\nmy%20link)") == 0);
        free(result);
    }
    {
        const char teststr[] = "test <!-- test ![alt--> "
            "![alt <\nimage]\n(\nmy\n link)";
        result = _internal_spew3dweb_markdown_CleanByteBufEx(
            teststr, strlen(teststr), 1, 1, 1, 1, NULL, NULL, NULL, NULL
        );
        printf("test_markdown_clean result #6: <<%s>>\n", result);
        assert(strcmp(result, "test  ![alt &lt; image](my%20link)") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "  abc\n    def\n ![alt image\n    ](my/\n           link)"
        );
        printf("test_markdown_clean result #7: <<%s>>\n", result);
        assert(strcmp(result, "abc\ndef\n![alt image\n](my/link)") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "abc\n      def\n    ug"
        );
        printf("test_markdown_clean result #8: <<%s>>\n", result);
        assert(strcmp(result, "abc\n\n      def\n    ug") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "- abc\n        def\n      ug"
        );
        printf("test_markdown_clean result #9: <<%s>>\n", result);
        assert(strcmp(result, "  - abc\n\n          def\n        ug") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            " abc:\n\n      def"
        );
        printf("test_markdown_clean result #10: <<%s>>\n", result);
        assert(strcmp(result, "abc:\n\n     def") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "abc:\n\n  ```c\n  abc\n   def\n  ```"
        );
        printf("test_markdown_clean result #11: <<%s>>\n", result);
        assert(strcmp(result, "abc:\n\n```c\nabc\n def\n```") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            " - abc: ```c printf(\"Hello\");```"
        );
        printf("test_markdown_clean result #12: <<%s>>\n", result);
        assert(strcmp(result,
            "  - abc:\n    ```c\n    printf(\"Hello\");\n    ```") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "  abc\n      def\n  "
        );  // The code block 'def' must be separated, or some
            // weird parsers won't see it as a code block.
        printf("test_markdown_clean result #13: <<%s>>\n", result);
        assert(strcmp(result, "abc\n\n    def\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "- abc\ndef\n"
        );  // The 'def' must be separated so some weird parsers
            // don't see it as part of the bullet point.
        printf("test_markdown_clean result #14: <<%s>>\n", result);
        assert(strcmp(result, "  - abc\n\ndef\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "- abc\n# def\n"
        );  // The 'def' heading here should be left alone.
        printf("test_markdown_clean result #15: <<%s>>\n", result);
        assert(strcmp(result, "  - abc\n# def\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "  abc\n  --\n"
        );
        printf("test_markdown_clean result #16: <<%s>>\n", result);
        assert(strcmp(result, "abc\n---\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "- abc\n\n      def\n  "
        );
        printf("test_markdown_clean result #17: <<%s>>\n", result);
        assert(strcmp(result, "  - abc\n\n        def\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc\n===\n  "
        );
        printf("test_markdown_clean result #18: <<%s>>\n", result);
        assert(strcmp(result, "  > abc\n\n===\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc\n    =\n  "
        );
        printf("test_markdown_clean result #19: <<%s>>\n", result);
        assert(strcmp(result, "  > abc\n    ===\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc\n  > def\n  "
        );
        printf("test_markdown_clean result #20: <<%s>>\n", result);
        assert(strcmp(result, "  > abc\n      > def\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc d\n  --\n  aef\n   efaef\n     \n  > test\n    x"
        );
        printf("test_markdown_clean result #21: <<%s>>\n", result);
        assert(strcmp(result, "  > abc d\n    -----\n    aef\n"
            "    efaef\n\n      > test\n        x") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc d\n  -\n"
        );
        printf("test_markdown_clean result #22: <<%s>>\n", result);
        assert(strcmp(result, "  > abc d\n    -----\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc d```a b\n abc\n  def```  bla\n"
        );
        printf("test_markdown_clean result #23: <<%s>>\n", result);
        assert(strcmp(result, "  > abc d\n    ```a\n    b\n"
            "    abc\n     def\n    ```\n    bla\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "> abc d```a b\n   abc\n   def```  bla\n"
        );
        printf("test_markdown_clean result #24: <<%s>>\n", result);
        assert(strcmp(result, "  > abc d\n    ```a\n    b\n     abc\n"
            "     def\n    ```\n    bla\n") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "abc```\ndef```\n"
        );
        printf("test_markdown_clean result #25: <<%s>>\n", result);
        assert(strcmp(result, "abc\n```\ndef\n```\n"
                      ) == 0);
        free(result);
    }
    {
        const char input[] = "[a](\nb\n)";
        result = _internal_spew3dweb_markdown_CleanByteBufEx(
            input, strlen(input),
            1, 1, 0, 1, NULL, NULL,
            NULL, NULL
        );
        printf("test_markdown_clean result #26: <<%s>>\n", result);
        assert(strcmp(result, "[a](b)") == 0);
        free(result);
    }
    {
        const char input[] = "[a](\nb\n)";
        result = _internal_spew3dweb_markdown_CleanByteBufEx(
            input, strlen(input),
            0, 1, 0, 1, NULL, NULL,
            NULL, NULL
        );
        printf("test_markdown_clean result #27: <<%s>>\n", result);
        assert(strcmp(result, "[a](\nb\n)") == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "1. test\n\n2. test\n   bla\n   1. henlo"
        );
        printf("test_markdown_clean result #28: <<%s>>\n", result);
        assert(strcmp(result, "1.  test\n\n2.  test\n    bla\n    1.  henlo"
                      ) == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "1. test\n2.  test\n    4. testo"
        );
        printf("test_markdown_clean result #29: <<%s>>\n", result);
        assert(strcmp(result, "1.  test\n2.  test\n    4.  testo"
                      ) == 0);
        free(result);
    }
    {
        result = spew3dweb_markdown_Clean(
            "1. abc\n\n     ```H\n     def\n     ```"
        );
        printf("test_markdown_clean result #30: <<%s>>\n", result);
        assert(strcmp(result, "1.  abc\n\n    ```H\n    def\n    ```"
                      ) == 0);
        free(result);
    }
}
END_TEST

START_TEST(test_is_url_and_is_image)
{
    assert(spew3dweb_markdown_IsStrUrl(
        "[abc](abc def)"
    ));
    assert(spew3dweb_markdown_IsStrImage(
        "![abc](abc def)"
    ));
    assert(spew3dweb_markdown_IsStrImage(
        "![abc](abc def){width=30% height=20px}"
    ));
    assert(spew3dweb_markdown_IsStrUrl(
        "[abc](\nabc def)"
    ));
    assert(!spew3dweb_markdown_IsStrUrl(
        "[abc](\n\nabc def)"
    ));
    assert(spew3dweb_markdown_IsStrUrl(
        "[ abc ] \n  ( abc def )"
    ));
    assert(!spew3dweb_markdown_IsStrUrl(
        "[abc](abc  def)"
    ));
    assert(spew3dweb_markdown_IsStrUrl(
        "[abc](a/bc( d)ef)"
    ));
    assert(!spew3dweb_markdown_IsStrUrl(
        "[abc](abc ( d)ef)"
    ));
    assert(!spew3dweb_markdown_IsStrUrl(
        "[abc](abc(def)"
    ));
    assert(spew3dweb_markdown_IsStrUrl(
        "[abc](abc[d]ef)"
    ));
    assert(spew3dweb_markdown_IsStrUrl(
        "[ **formatting test** <i>abc</i>](abc def)"
    ));
    assert(!spew3dweb_markdown_IsStrUrl(
        "[abc\n---\n](abc def)"
    ));
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
            "# abc def\ndef"
        );
        printf("test_markdown_tohtml result #1: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<h1><a name='abc-def' href='#abc-def'>"
            "abc def</a></h1>\n<p>def</p>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "- test\n  - test2\n- test 3"
        );
        printf("test_markdown_tohtml result #2: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<ul><li><p>test</p><ul><li><p>test2</p></li></ul></li>"
            "<li><p>test 3</p></li></ul>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "- test\n  - test2\n- test 3\n\n  bla\n- oof"
        );
        printf("test_markdown_tohtml result #3: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<ul><li><p>test</p><ul><li><p>test2</p></li></ul></li>"
            "<li><p>test 3</p><p>bla</p></li>"
            "<li><p>oof</p></li></ul>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "- abc\n  ===\n  bla\n\n  bla 2"
        );
        printf("test_markdown_tohtml result #4: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<ul><li><h1><a name='abc' href='#abc'>"
            "abc</a></h1><p>bla</p>"
            "<p>bla 2</p></li></ul>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "- **test:** bla\n"
        );
        printf("test_markdown_tohtml result #5: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<ul><li><p><strong>test:</strong> bla</p></li></ul>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "a*b*c\n=====\n"
        );
        printf("test_markdown_tohtml result #6: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<h1><a name='abc' href='#abc'>a<em>b</em>c</a></h1>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "a*b*c [abc *def*](/some'target.md)\n=====\n"
        );
        printf("test_markdown_tohtml result #7: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<h1>a<em>b</em>c <a href='/some%27target.md'>"
            "abc <em>def</em></a></h1>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "a*b*c [abc ![my image](/oops.png)*def*](/some'target.md)\n=====\n"
        );
        printf("test_markdown_tohtml result #8: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<h1>a<em>b</em>c <a href='/some%27target.md'>"
            "abc <img src='/oops.png' alt='my image'/><em>def</em></a></h1>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "# [abc](https://example.com/)\ntoast\n"
        );
        printf("test_markdown_tohtml result #9: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<h1><a href='https://example.com/' "
            "rel=noopener target=_blank>abc</a></h1><p>toast</p>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "*(ab,\n[abc](abc))*"
        );
        printf("test_markdown_tohtml result #10: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<p><em>(ab, <a href='abc'>abc</a>)</em></p>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "*a[b*](c)*d"
        );
        printf("test_markdown_tohtml result #11: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<p><em>a<a href='c'>b*</a></em>d</p>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "abc```\ndef```"
        );
        printf("test_markdown_tohtml result #12: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<p>abc</p><pre><code>def\n</code></pre>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "|a|b|\n|--|----|\n|**thick**|![](lol.png)|"
        );
        printf("test_markdown_tohtml result #13: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<table><tr><th>a</th><th>b</th></tr>"
            "<tr><td><strong>thick</strong></td><td>"
            "<img src='lol.png'/></td></tr></table>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "![](test.png){width=5% height=3}"
        );
        printf("test_markdown_tohtml result #14: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<p><img width='5%' height='3px' src='test.png'/></p>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "- *This is a multiline test `(inner\n  code)`,* complex!"
        );
        printf("test_markdown_tohtml result #15: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<ul><li><p><em>This is a multiline test <code>"
            "(inner code)</code>,</em> complex!</p></li></ul>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "[a](\nb\n)"
        );
        printf("test_markdown_tohtml result #16: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<p><a href='b'>a</a></p>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "1. test\n2.  test\n    4. testo"
        );
        printf("test_markdown_tohtml result #17: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<ol start=1><li><p>test</p></li>"
            "<li><p>test</p>"
            "<ol start=4><li><p>testo</p></li></ol></li></ol>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "1. abc\n\n     ```H\n     def\n     ```\n\n   ghi\n\n2. j"
        );
        printf("test_markdown_tohtml result #18: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<ol start=1><li><p>abc</p><pre>"
            "<code lang='H'>def</code></pre><p>ghi</p></li>"
            "<li><p>j</p></li></ol>"));
        free(result);
    }
    {
        result = spew3dweb_markdown_ToHTML(
            "1. abc\n     ```H\n     def\n     ```\n\n   ghi\n\n2. j"
        );
        printf("test_markdown_tohtml result #19: <<%s>>\n", result);
        assert(_s3dw_check_html_same(result,
            "<ol start=1><li><p>abc</p><pre>"
            "<code lang='H'>def</code></pre><p>ghi</p></li>"
            "<li><p>j</p></li></ol>"));
        free(result);
    }
}
END_TEST

TESTS_MAIN(test_markdown_chunks, test_markdown_clean,
    test_markdown_tohtml, test_is_url_and_is_image)

