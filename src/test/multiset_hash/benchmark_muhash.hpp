#ifndef HEADER_GUARD_cedb8c2b028c2d460788a4749050bd0d
#define HEADER_GUARD_cedb8c2b028c2d460788a4749050bd0d

#include "jbms/multiset_hash/MuHash.hpp"
#include "./benchmark_adhash_or_muhash.hpp"

namespace jbms {
namespace multiset_hash {

template <class Hash>
void benchmark_muhash(int bits, Hash const &H) {

  jbms::openssl::bignum p;

  if (bits < 0) {
    // Just generate an odd number, since generating a large prime takes a long time
    // The timings for add aren't affected by whether it is a prime or not
    pseudo_rand(p, -bits, 0, 1);
  } else {
    // Note: For actual usage, we would want a safe prime
    // However, for testing purposes, it doesn't matter

    jbms::openssl::throw_last_error_if(BN_generate_prime(p.get(),
                                                         bits,
                                                         0 /* not safe */,
                                                         nullptr /* add */,
                                                         nullptr /* rem */,
                                                         nullptr /* callback */,
                                                         nullptr /* cb_arg */) == nullptr);
  }
  MuHash<Hash> mh(p, H);

  benchmark_adhash_or_muhash(mh);
}

}
}


#endif /* HEADER GUARD */
