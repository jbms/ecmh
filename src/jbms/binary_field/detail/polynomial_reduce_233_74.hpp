
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

#ifndef HEADER_GUARD_1fd1eecb78c93aab3789d18b3ccf7427
#define HEADER_GUARD_1fd1eecb78c93aab3789d18b3ccf7427

#include "./polynomial_base.hpp"
#include "./limb_ops.hpp"
#include "./GF2.hpp"

namespace jbms {
namespace binary_field {

namespace detail {


inline void reduce_after_multiply(GF2<233,74> const &F, BinaryPolynomial<233> &z, BinaryPolynomial<233 * 2> const &r) {
  limb_t x[6];

  x[0] = SHL(r.limbs[3], 33);
  x[1] = SHR(r.limbs[3], 31);
  x[2] = SHL(r.limbs[3], 23);
  x[3] = SHR(r.limbs[3], 41);

  x[4] = XOR(x[0], x[3]);
  x[3] = SHR128<8>(x[4]);
  z.limbs[1] = XOR(r.limbs[1], x[2]);
  limb_t r2 = XOR(r.limbs[2], x[1]);
  r2 = XOR(r2, x[3]);

  x[0] = SHL(r2, 33);
  x[1] = SHR(r2, 31);
  x[2] = SHL(r2, 23);
  x[3] = SHR(r2, 41);

  x[5] = XOR(x[0], x[3]);
  x[3] = ALIGNR<8>(x[4], x[5]);
  z.limbs[0] = XOR(r.limbs[0], x[2]);
  z.limbs[1] = XOR(z.limbs[1], x[1]);
  z.limbs[1] = XOR(z.limbs[1], x[3]);

  /* Clear top */
  x[2] = SET64(0x000001FFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
  x[0] = NAND(x[2], z.limbs[1]);
  x[0] = SHR(x[0], 41);
  x[1] = ALIGNR<8>(x[5], x[0]);
  z.limbs[0] = XOR(z.limbs[0], x[1]);
  x[1] = SHL(x[0], 10);
  z.limbs[0] = XOR(z.limbs[0], x[1]);
  z.limbs[1] = AND(z.limbs[1], x[2]);
}


}


using detail::reduce_after_multiply;

}
}

#endif /* HEADER GUARD */
