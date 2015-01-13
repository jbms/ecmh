
/*********************************************************************************************
 *	EC2M XMM Library
 *
 * This library provides arithmetic in GF(2^m) for m € {163, 193, 233, 239, 283, 409, 571}
 * for the use with elliptic curve cryptography, utilizing Intels vector instructions AVX via
 * compiler intrinsics. For more information about the library please see [1]. The masking
 * scheme has been slightly improved to reduce the number of instructions.
 *
 * [1] M. Bluhm, S. Gueron, 'Fast Software Implementation of Binary Elliptic Curve Cryptography',
 *     2013, https://eprint.iacr.org/2013/741
 *********************************************************************************************/


/*********************************************************************************************
 * Copyright (C) 2014, Manuel Bluhm
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************************/

#ifndef HEADER_GUARD_51f05fc5186ef0472ea24a0fda5b5e42
#define HEADER_GUARD_51f05fc5186ef0472ea24a0fda5b5e42

#include "./polynomial_base.hpp"
#include "./limb_ops.hpp"
#include "./GF2.hpp"

namespace jbms {
namespace binary_field {

namespace detail {


/* Reduction of elements in GF(2^239) is provided in 64-bit mode only. */
inline void reduce_after_multiply(GF2<239,158> const &F, BinaryPolynomial<239> &z, BinaryPolynomial<239 * 2> const &r) {
  uint64_t zz, x[8];

  STORE128((limb_t *)(x + 0), r.limbs[0]);
  STORE128((limb_t *)(x + 2), r.limbs[1]);
  STORE128((limb_t *)(x + 4), r.limbs[2]);
  STORE128((limb_t *)(x + 6), r.limbs[3]);

  zz = x[7];
  x[6] ^= (zz >> 17);
  x[5] ^= (zz << 47);
  x[4] ^= (zz >> 47);
  x[3] ^= (zz << 17);

  zz = x[6];
  x[5] ^= (zz >> 17);
  x[4] ^= (zz << 47);
  x[3] ^= (zz >> 47);
  x[2] ^= (zz << 17);

  zz = x[5];
  x[4] ^= (zz >> 17);
  x[3] ^= (zz << 47);
  x[2] ^= (zz >> 47);
  x[1] ^= (zz << 17);

  zz = x[4];
  x[3] ^= (zz >> 17);
  x[2] ^= (zz << 47);
  x[1] ^= (zz >> 47);
  x[0] ^= (zz << 17);

  /* Clear top */
  zz = (x[3] >> 47);
  x[3] &= 0x00007FFFFFFFFFFF;
  x[0] ^= zz;
  x[2] ^= (zz << 30);

  z.limbs[0] = LOAD128((limb_t *)(x));
  z.limbs[1] = LOAD128((limb_t *)(x + 2));
}

}

using detail::reduce_after_multiply;

}
}

#endif /* HEADER GUARD */
