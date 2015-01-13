#ifndef HEADER_GUARD_900ccc4beef0f0bce1814e5816f66ff1
#define HEADER_GUARD_900ccc4beef0f0bce1814e5816f66ff1

#include "jbms/multiset_hash/MuHash.hpp"
#include "./test_generic_multiset_hash.hpp"

namespace jbms {
namespace multiset_hash {

template <class Hash>
void test_muhash(Hash const &H) {

  // Test some odd sizes as well to make sure things work when number of bits is not equal to the word size
  for (auto bits : {128, 256, 512, 513, 541}) {
    openssl::bignum p;

    // Note: For actual usage, we would want a safe prime
    // However, for testing purposes, it doesn't matter
    jbms::openssl::throw_last_error_if(BN_generate_prime(p.get(),
                                                         bits,
                                                         0 /* not safe */,
                                                         nullptr /* add */,
                                                         nullptr /* rem */,
                                                         nullptr /* callback */,
                                                         nullptr /* cb_arg */) == nullptr);

    MuHash<Hash> mh(p, H);
    test_generic_multiset_hash(mh);
  }
}

}
}

#endif /* HEADER GUARD */
