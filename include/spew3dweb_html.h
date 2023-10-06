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

#ifndef SPEW3DWEB_HTML_H_
#define SPEW3DWEB_HTML_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>  // for size_t

S3DEXP size_t s3dw_html_GetTagLengthByteBuf(
    const char *s, size_t slen,
    const char **out_tagname_tart,
    size_t *out_tagname_len
);

S3DEXP size_t s3dw_html_GetTagLengthStr(
    const char *s,
    const char **out_tagname_tart,
    size_t *out_tagname_len
);

#endif  // SPEW3DWEB_HTML_H_

