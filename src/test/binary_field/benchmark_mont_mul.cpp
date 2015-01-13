#include "jbms/openssl/bn.hpp"
#include "jbms/benchmark.hpp"
#include <iostream>
#include <iomanip>

struct MuHashSimulator {
  jbms::openssl::bn_ctx ctx;
  jbms::openssl::bn_mont_ctx mont;
};

template <bool C>
__attribute__((noinline)) void test_mont_mul(jbms::openssl::bignum &result, jbms::openssl::bignum &x, MuHashSimulator &ctx) {
  __asm__ volatile("");
  if (C)
    mod_mul_montgomery(result, result, x, ctx.mont, ctx.ctx);
}

template <bool C>
__attribute__((noinline)) void test_mul(jbms::openssl::bignum &result, jbms::openssl::bignum &x, jbms::openssl::bignum const &p, MuHashSimulator &ctx) {
  __asm__ volatile("");
  if (C)
    mod_mul(result, result, x, p, ctx.ctx);
}



int main(int argc, char **argv) {
  int bits = atoi(argv[1]);
  using namespace jbms::openssl;

  MuHashSimulator ctx;
  jbms::openssl::bignum p;

  {
#if 1
    pseudo_rand(p, bits, 0, 1);
    // Generating a prime takes a while, and mod_mul_montgomery takes the same amount of time anyway
#else
    jbms::openssl::throw_last_error_if(BN_generate_prime(p.get(),
                                                         bits,
                                                         0 /* not safe */,
                                                         nullptr /* add */,
                                                         nullptr /* rem */,
                                                         nullptr /* callback */,
                                                         nullptr /* cb_arg */) == nullptr);
#endif
    ctx.mont.set(p, ctx.ctx);
  }

  bignum current_total;
  current_total.set_one();
  bignum x;
  BN_pseudo_rand_range(x, ctx.mont.N());
  jbms::benchmark_function("mont_mul", [&](auto C) { test_mont_mul<C()>(current_total, x, ctx); });

  bignum current_total2;
  current_total2.set_one();
  jbms::benchmark_function("mul", [&](auto C) { test_mul<C()>(current_total, x, p, ctx); });

  return 0;
}
