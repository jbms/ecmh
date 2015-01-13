
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

#ifndef HEADER_GUARD_4ca1d48a672a31d31dd7e170b04fa5b3
#define HEADER_GUARD_4ca1d48a672a31d31dd7e170b04fa5b3

#include "./polynomial_base.hpp"
#include "./limb_ops.hpp"
#include "./GF2.hpp"

namespace jbms {
namespace binary_field {

namespace detail {


inline void reduce_after_multiply(GF2<409,87> const &F, BinaryPolynomial<409> &z, BinaryPolynomial<409 * 2> const &r) {
  limb_t x[3], m[12];

  m[0] = SHR(r.limbs[6], 2);
  m[1] = SHL(r.limbs[6], 62);
  m[2] = SHR(r.limbs[6], 25);
  m[3] = SHL(r.limbs[6], 39);
  m[4] = SHR(r.limbs[5], 2);
  m[5] = SHL(r.limbs[5], 62);
  m[6] = SHR(r.limbs[5], 25);
  m[7] = SHL(r.limbs[5], 39);
  m[8] = SHR(r.limbs[4], 2);
  m[9] = SHL(r.limbs[4], 62);
  m[10] = SHR(r.limbs[4], 25);
  m[11] = SHL(r.limbs[4], 39);

  x[0] = XOR(m[1], m[2]);
  z.limbs[3] = XOR(r.limbs[3], x[0]);
  x[1] = XOR(m[4], m[3]);
  x[2] = ALIGNR<8>(m[0], x[1]);
  z.limbs[3] = XOR(z.limbs[3], x[2]);
  x[0] = XOR(m[5], m[6]);
  z.limbs[2] = XOR(r.limbs[2], x[0]);
  m[7] = XOR(m[7], m[8]);
  x[1] = ALIGNR<8>(x[1], m[7]);
  z.limbs[2] = XOR(z.limbs[2], x[1]);
  x[2] = XOR(m[9], m[10]);
  z.limbs[1] = XOR(r.limbs[1], x[2]);

  /* Clear top */
  x[0] = SET64(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFE000000);
  x[0] = AND(z.limbs[3], x[0]);
  z.limbs[3] = XOR(z.limbs[3], x[0]);

  m[0] = SHR(x[0], 2);
  m[1] = SHL(x[0], 62);
  m[2] = SHR(x[0], 25);
  m[3] = SHL(x[0], 39);

  x[0] = XOR(m[11], m[0]);
  x[1] = ALIGNR<8>(m[7], x[0]);
  z.limbs[1] = XOR(z.limbs[1], x[1]);
  x[2] = XOR(m[1], m[2]);
  z.limbs[0] = XOR(r.limbs[0], x[2]);
  x[0] = ALIGNR<8>(x[0], m[3]);
  z.limbs[0] = XOR(z.limbs[0], x[0]);
}



}


using detail::reduce_after_multiply;


}
}

#endif /* HEADER GUARD */
