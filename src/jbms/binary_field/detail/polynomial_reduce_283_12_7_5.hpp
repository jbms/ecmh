
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

#ifndef HEADER_GUARD_72574726d124a29126f3af06d61571d2
#define HEADER_GUARD_72574726d124a29126f3af06d61571d2

#include "./polynomial_base.hpp"
#include "./limb_ops.hpp"
#include "./GF2.hpp"

namespace jbms {
namespace binary_field {

namespace detail {


#ifdef FAST_CLMUL
inline void reduce_after_multiply(GF2<283,12,7,5> const &F, BinaryPolynomial<283> &z, BinaryPolynomial<283 * 2> const &r) {
  limb_t poly, x[2], m[5];

  poly = SET64(0, 0x0002142000000000);

  m[0] = poly_mul_vec<0, 0>(r.limbs[4], poly);
  m[1] = poly_mul_vec<0, 1>(r.limbs[3], poly);
  m[2] = poly_mul_vec<0, 0>(r.limbs[3], poly);

  x[0] = SHR128<8>(m[0]);
  z.limbs[2] = XOR(r.limbs[2], x[0]);
  z.limbs[1] = XOR(m[1], r.limbs[1]);
  x[0] = ALIGNR<8>(m[0], m[2]);
  z.limbs[1] = XOR(x[0], z.limbs[1]);

  x[0] = SHL128<8>(m[2]);
  z.limbs[0] = XOR(x[0], r.limbs[0]);

  x[0] = SET64(0x0000000000000000, 0x0000000007FFFFFF);
  x[1] = NAND(x[0], z.limbs[2]);
  z.limbs[2] = AND(x[0], z.limbs[2]);

  m[3] = poly_mul_vec<0, 1>(x[1], poly);
  m[4] = poly_mul_vec<0, 0>(x[1], poly);

  x[0] = SHR128<8>(m[4]);
  z.limbs[0] = XOR(x[0], z.limbs[0]);
  z.limbs[0] = XOR(m[3], z.limbs[0]);
}
#else // slow CLMUL
inline void reduce_after_multiply(GF2<283,12,7,5> const &F, BinaryPolynomial<283> &z, BinaryPolynomial<283 * 2> const &r_in) {
  BinaryPolynomial<283 * 2> r = r_in;
  limb_t x[4];

  x[0] = ALIGNR<8>(r.limbs[3], r.limbs[2]);
  x[1] = ALIGNR<8>(r.limbs[4], r.limbs[3]);
  x[3] = r.limbs[2];

  r.limbs[4] = SHR(r.limbs[4], 27);
  r.limbs[3] = SHR(r.limbs[3], 27);
  x[2] = SHL(x[1], 37);
  r.limbs[3] = XOR(r.limbs[3], x[2]);
  r.limbs[2] = SHR(r.limbs[2], 27);
  x[2] = SHL(x[0], 37);
  r.limbs[2] = XOR(r.limbs[2], x[2]);

  x[0] = ALIGNR<15>(r.limbs[4], r.limbs[3]);
  x[2] = SHR(x[0], 1);
  r.limbs[4] = XOR(r.limbs[4], x[2]);

  x[1] = ALIGNR<8>(r.limbs[3], r.limbs[2]);
  x[2] = SHL(r.limbs[3], 7);
  r.limbs[3] = XOR(r.limbs[3], x[2]);
  x[2] = SHR(x[1], 57);
  r.limbs[3] = XOR(r.limbs[3], x[2]);

  x[0] = ALIGNR<8>(r.limbs[2], ZERO());
  x[2] = SHL(r.limbs[2], 7);
  r.limbs[2] = XOR(r.limbs[2], x[2]);
  x[2] = SHR(x[0], 57);
  r.limbs[2] = XOR(r.limbs[2], x[2]);

  x[0] = ALIGNR<15>(r.limbs[4], r.limbs[3]);
  x[1] = SHR(x[0], 3);
  r.limbs[4] = XOR(r.limbs[4], x[1]);

  x[1] = ALIGNR<8>(r.limbs[3], r.limbs[2]);
  x[2] = SHL(r.limbs[3], 5);
  r.limbs[3] = XOR(r.limbs[3], x[2]);
  x[2] = SHR(x[1], 59);
  r.limbs[3] = XOR(r.limbs[3], x[2]);

  x[0] = ALIGNR<8>(r.limbs[2], ZERO());
  x[2] = SHL(r.limbs[2], 5);
  r.limbs[2] = XOR(r.limbs[2], x[2]);
  x[2] = SHR(x[0], 59);
  r.limbs[2] = XOR(r.limbs[2], x[2]);

  z.limbs[0] = XOR(r.limbs[0], r.limbs[2]);
  z.limbs[1] = XOR(r.limbs[1], r.limbs[3]);
  z.limbs[2] = XOR(x[3], r.limbs[4]);

  /* Clear top */
  x[0] = SHR(r.limbs[4], 27);
  x[2] = SHL(x[0], 5);
  x[1] = XOR(x[0], x[2]);
  x[2] = SHL(x[1], 7);
  x[0] = XOR(x[1], x[2]);

  z.limbs[0] = XOR(z.limbs[0], x[0]);
  x[2] = SET64(0x0000000000000000, 0x0000000007FFFFFF);
  z.limbs[2] = AND(z.limbs[2], x[2]);
}
#endif

}

using detail::reduce_after_multiply;

}
}

#endif /* HEADER GUARD */
