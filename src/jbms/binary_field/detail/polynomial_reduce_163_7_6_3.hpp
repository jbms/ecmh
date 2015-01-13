
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

#ifndef HEADER_GUARD_61b902e179d7583e086d600e558c11c6
#define HEADER_GUARD_61b902e179d7583e086d600e558c11c6

#include "./polynomial_base.hpp"
#include "./limb_ops.hpp"
#include "./GF2.hpp"

namespace jbms {
namespace binary_field {

namespace detail {


#ifndef FAST_CLMUL
inline void reduce_after_multiply(GF2<163,7,6,3> const &F, BinaryPolynomial<163> &z, BinaryPolynomial<163 * 2> const &r) {
  limb_t x[5];

  x[0] = SHR(r.limbs[2], 35);
  x[1] = SHL(r.limbs[2], 29);

  x[3] = SHL128<4>(r.limbs[2]);
  x[1] = XOR(x[1], x[3]);

  x[2] = SHR(r.limbs[2], 29);
  x[3] = SHL(r.limbs[2], 35);
  x[0] = XOR(x[0], x[2]);
  x[1] = XOR(x[1], x[3]);

  x[2] = SHR(r.limbs[2], 28);
  x[3] = SHL(r.limbs[2], 36);
  x[0] = XOR(x[0], x[2]);
  x[1] = XOR(x[1], x[3]);

  x[2] = SHL128<8>(x[1]);
  x[1] = SHR128<8>(x[1]);
  x[0] = XOR(x[0], x[1]);

  z.limbs[0] = XOR(r.limbs[0], x[2]);
  z.limbs[1] = XOR(r.limbs[1], x[0]);

  /* Clear top */
  x[1] = SET64(0xFFFFFFFFFFFFFFFF, 0xFFFFFFF800000000);
  x[4] = AND(x[1], z.limbs[1]);
  z.limbs[1] = NAND(x[1], z.limbs[1]);

  x[0] = SHR(x[4], 35);
  x[1] = SHL(x[4], 29);

  x[2] = SHR128<4>(x[4]);
  x[0] = XOR(x[0], x[2]);

  x[2] = SHR(x[4], 29);
  x[3] = SHL(x[4], 35);
  x[0] = XOR(x[0], x[2]);
  x[1] = XOR(x[1], x[3]);

  x[2] = SHR(x[4], 28);
  x[3] = SHL(x[4], 36);
  x[0] = XOR(x[0], x[2]);
  x[1] = XOR(x[1], x[3]);

  x[1] = SHR128<8>(x[1]);
  x[0] = XOR(x[0], x[1]);
  z.limbs[0] = XOR(z.limbs[0], x[0]);
}
#else
inline void reduce_after_multiply(GF2<163,7,6,3> const &F, BinaryPolynomial<163> &z, BinaryPolynomial<163 * 2> const &r) {
  limb_t _p, x[2], m[2];

  _p = SET64(0, 0x000001920000000);

  m[0] = poly_mul_vec<0, 1>(r.limbs[2], _p);
  m[1] = poly_mul_vec<0, 0>(r.limbs[2], _p);

  z.limbs[1] = XOR(r.limbs[1], m[0]);
  x[0] = SHL128<8>(m[1]);
  x[1] = SHR128<8>(m[1]);
  z.limbs[0] = XOR(r.limbs[0], x[0]);
  z.limbs[1] = XOR(z.limbs[1], x[1]);

  x[0] = SET64(0xFFFFFFFFFFFFFFFF, 0xFFFFFFF800000000);
  x[1] = AND(x[0], z.limbs[1]);
  z.limbs[1] = NAND(x[0], z.limbs[1]);

  m[0] = poly_mul_vec<0, 1>(x[1], _p);
  m[1] = poly_mul_vec<0, 0>(x[1], _p);

  z.limbs[0] = XOR(z.limbs[0], m[0]);
  x[0] = SHR128<8>(m[1]);
  z.limbs[0] = XOR(z.limbs[0], x[0]);
}
#endif


}


using detail::reduce_after_multiply;


}
}

#endif /* HEADER GUARD */
