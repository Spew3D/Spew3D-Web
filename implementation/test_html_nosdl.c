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

START_TEST(test_html_get_tag_length)
{
    size_t result;
    {
        const char *tagstart;
        size_t taglen;
        result = s3dw_html_GetTagLengthStr(
            "<img src=\"<img >src=.png\"/> bla.",
            &tagstart, &taglen
        );
        ck_assert(result == 27);
        ck_assert(taglen == 3);
        ck_assert_mem_eq(tagstart, "img", 3);
    }
}
END_TEST

TESTS_MAIN(test_html_get_tag_length)

