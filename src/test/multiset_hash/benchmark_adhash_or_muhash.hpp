#ifndef HEADER_GUARD_67a16da9fc64fbade8cce440cbebaef5
#define HEADER_GUARD_67a16da9fc64fbade8cce440cbebaef5

#include "jbms/benchmark.hpp"
#include "./benchmark_generic_multiset_hash.hpp"

namespace jbms {
namespace multiset_hash {

template <bool C, class Hash>
__attribute__((noinline)) void test_hash_element(Hash const &H, jbms::openssl::bignum &result, array_view<void const> x) {
  if (C) hash_element(H, result, x);
}

template <bool C, class Hash>
__attribute__((noinline)) void test_hash_expand(Hash const &H, uint8_t *buf, size_t num_digests, array_view<void const> x) {
  if (C) hash_expand(H, buf, num_digests, x);
}

template <class MSH>
void benchmark_adhash_or_muhash(MSH const &msh) {

  using Hash = typename MSH::Hash;

  benchmark_generic_multiset_hash(msh, false /* don't benchmark batch operations */);

  auto benchmark_hash_element = [&](auto N_t) {
    std::array<uint8_t,N_t.value> data;
    openssl::bignum result;
    benchmark_function("hash_element" + std::to_string(N_t.value), [&](auto C) { test_hash_element<C()>(msh, result, data); });
  };

  benchmark_hash_element(std::integral_constant<size_t,10>{});
  benchmark_hash_element(std::integral_constant<size_t,100>{});
  benchmark_hash_element(std::integral_constant<size_t,150>{});
  benchmark_hash_element(std::integral_constant<size_t,200>{});

  auto benchmark_hash = [&](auto N_t) {
    const size_t num_digests = jbms::div_ceil(msh.num_bytes(), Hash::digest_bytes);
    std::array<uint8_t,N_t.value> data;
    uint8_t buf[num_digests * Hash::digest_bytes];
    benchmark_function("hash_expand" + std::to_string(N_t.value),
                       [&, buf = &buf[0] ](auto C) { test_hash_expand<C()>(msh.hash(), buf, num_digests, data); });
  };

  benchmark_hash(std::integral_constant<size_t,10>{});
  benchmark_hash(std::integral_constant<size_t,100>{});
  benchmark_hash(std::integral_constant<size_t,150>{});
  benchmark_hash(std::integral_constant<size_t,200>{});

}



}
}

#endif /* HEADER GUARD */
