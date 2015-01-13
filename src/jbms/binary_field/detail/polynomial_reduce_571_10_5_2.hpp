
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

#ifndef HEADER_GUARD_24d9e12d96402bdc189950f8287687fc
#define HEADER_GUARD_24d9e12d96402bdc189950f8287687fc

#include "./polynomial_base.hpp"
#include "./limb_ops.hpp"
#include "./GF2.hpp"

namespace jbms {
namespace binary_field {

namespace detail {

#ifndef FAST_CLMUL
inline void reduce_after_multiply(GF2<571,10,5,2> const &F, BinaryPolynomial<571> &z, BinaryPolynomial<571 * 2> const &r_in) {
  BinaryPolynomial<571 * 2> r = r_in;
  /* Init */
  const int n = 4;
  int i;
  limb_t x[5];

  x[4] = ZERO();
  for (i = 8; i > n; i--) {
    x[0] = SHL(r.limbs[i], 5);
    x[1] = SHR(r.limbs[i], 59);

    x[2] = SHL(r.limbs[i], 7);
    x[3] = SHR(r.limbs[i], 57);
    x[0] = XOR(x[0], x[2]);
    x[1] = XOR(x[1], x[3]);

    x[2] = SHL(r.limbs[i], 10);
    x[3] = SHR(r.limbs[i], 54);
    x[0] = XOR(x[0], x[2]);
    x[1] = XOR(x[1], x[3]);

    x[2] = SHL(r.limbs[i], 15);
    x[3] = SHR(r.limbs[i], 49);
    x[0] = XOR(x[0], x[2]);
    x[1] = XOR(x[1], x[3]);

    x[2] = ALIGNR<8>(x[4], x[0]);
    r.limbs[i - n] = XOR(r.limbs[i - n], x[2]);
    r.limbs[i - n] = XOR(r.limbs[i - n], x[1]);

    x[4] = x[0];
  }

  x[0] = SHL128<8>(x[4]);
  r.limbs[i - n] = XOR(r.limbs[i - n], x[0]);

  /* Clear top */
  x[4] = SET64(0xFFFFFFFFFFFFFFFF, 0xF800000000000000);
  x[4] = AND(r.limbs[4], x[4]);
  r.limbs[4] = XOR(r.limbs[4], x[4]);

  x[0] = SHR(x[4], 59);
  x[1] = SHL(x[4], 5);

  x[2] = SHR(x[4], 57);
  x[3] = SHL(x[4], 7);
  x[0] = XOR(x[0], x[2]);
  x[1] = XOR(x[1], x[3]);

  x[2] = SHR(x[4], 54);
  x[3] = SHL(x[4], 10);
  x[0] = XOR(x[0], x[2]);
  x[1] = XOR(x[1], x[3]);

  x[2] = SHR(x[4], 49);
  x[3] = SHL(x[4], 15);
  x[0] = XOR(x[0], x[2]);
  x[1] = XOR(x[1], x[3]);

  x[1] = SHR128<8>(x[1]);
  r.limbs[0] = XOR(r.limbs[0], x[1]);

  r.limbs[0] = XOR(r.limbs[0], x[0]);

  for (int i = 0; i < 5; ++i)
    z.limbs[i] = r.limbs[i];
}

#else // slow CLMUL

inline void reduce_after_multiply(GF2<571,10,5,2> const &F, BinaryPolynomial<571> &z, BinaryPolynomial<571 * 2> const &r) {
  limb_t _p, x[10];

  _p = SET64(0, 0x00000000000084A0);

  x[0] = poly_mul_vec<0, 1>(r.limbs[8], _p);
  x[1] = poly_mul_vec<0, 0>(r.limbs[8], _p);
  x[2] = poly_mul_vec<0, 1>(r.limbs[7], _p);
  x[3] = poly_mul_vec<0, 0>(r.limbs[7], _p);
  x[4] = poly_mul_vec<0, 1>(r.limbs[6], _p);
  x[5] = poly_mul_vec<0, 0>(r.limbs[6], _p);
  x[6] = poly_mul_vec<0, 1>(r.limbs[5], _p);
  x[7] = poly_mul_vec<0, 0>(r.limbs[5], _p);

  z.limbs[4] = XOR(x[0], r.limbs[4]);
  z.limbs[3] = XOR(x[2], r.limbs[3]);
  z.limbs[2] = XOR(x[4], r.limbs[2]);
  z.limbs[1] = XOR(x[6], r.limbs[1]);

  x[8] = SHR128<8>(x[1]);
  z.limbs[4] = XOR(x[8], z.limbs[4]);
  x[9] = ALIGNR<8>(x[1], x[3]);
  z.limbs[3] = XOR(x[9], z.limbs[3]);
  x[8] = ALIGNR<8>(x[3], x[5]);
  z.limbs[2] = XOR(x[8], z.limbs[2]);
  x[9] = ALIGNR<8>(x[5], x[7]);
  z.limbs[1] = XOR(x[9], z.limbs[1]);
  x[0] = SHL128<8>(x[7]);
  z.limbs[0] = XOR(x[0], r.limbs[0]);

  x[3] = SET64(0xFFFFFFFFFFFFFFFF, 0xF800000000000000);
  x[3] = AND(z.limbs[4], x[3]);
  z.limbs[4] = XOR(z.limbs[4], x[3]);

  x[1] = poly_mul_vec<0, 1>(x[3], _p);
  x[2] = poly_mul_vec<0, 0>(x[3], _p);

  z.limbs[0] = XOR(x[1], z.limbs[0]);
  x[2] = SHR128<8>(x[2]);
  z.limbs[0] = XOR(x[2], z.limbs[0]);
}
#endif

}

using detail::reduce_after_multiply;

}
}



#endif /* HEADER GUARD */
