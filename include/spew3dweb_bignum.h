/* Copyright (c) 2020-2023, ellie/@ell1e & Spew3D Web Team (see AUTHORS.md).

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

#ifndef SPEW3DWEB_BIGINT_H_
#define SPEW3DWEB_BIGINT_H_

#include <stdint.h>

S3DEXP int s3dw_bignum_VerifyStrInt(const char *v);

S3DEXP int s3dw_bignum_VerifyStrIntBuf(
    const char *v, size_t vlen
);

S3DEXP int s3dw_bignum_VerifyStrFloat(const char *v);

S3DEXP int s3dw_bignum_VerifyStrFloatBuf(
    const char *v, size_t vlen
);

S3DEXP int s3dw_bignum_CompareStrInts(
    const char *v1, const char *v2
);

S3DEXP int s3dw_bignum_CompareStrIntsBuf(
    const char *v1, size_t v1len,
    const char *v2, size_t v2len
);

S3DEXP int s3dw_bignum_CompareStrFloats(
    const char *v1, const char *v2
);

S3DEXP int s3dw_bignum_CompareStrFloatsBuf(
    const char *v1, size_t v1len,
    const char *v2, size_t v2len
);

S3DHID char *_internal_s3dw_bignum_AddPosNonfracStrFloatsBuf(
    const char *v1, size_t v1len, size_t v1imaginaryzeroes,
    const char *v2, size_t v2len, size_t v2imaginaryzeroes,
    int allowlinedupdot,
    int with_initial_carryover,
    char *use_buf,
    uint64_t *out_len,
    int *out_endedwithcarryover
);

S3DHID char *_internal_s3dw_bignum_SubPosNonfracStrFloatsBuf(
    const char *v1, size_t v1len, size_t v1imaginaryzeroes,
    const char *v2, size_t v2len, size_t v2imaginaryzeroes,
    int allowlinedupdot,
    int with_initial_carryover,
    char *use_buf,
    uint64_t *out_len
);

S3DEXP void s3dw_bignum_PrintFloatBuf(
    const char *v, size_t vlen
);

S3DEXP char *s3dw_bignum_AddStrFloatBufsEx(
    const char *v1, size_t v1len,
    const char *v2, size_t v2len,
    char *use_buf, int truncate_fractional,
    uint64_t *out_len
);

S3DEXP char *s3dw_bignum_AddStrFloatBufs(
    const char *v1, size_t v1len,
    const char *v2, size_t v2len,
    uint64_t *out_len
);

#endif  // SPEW3DWEB_BIGINT_H_

