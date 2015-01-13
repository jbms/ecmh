#ifndef HEADER_GUARD_2b4ad5108337ae445b7dc80e3c8a9f89
#define HEADER_GUARD_2b4ad5108337ae445b7dc80e3c8a9f89

#include "jbms/benchmark.hpp"
#include "jbms/binary_field/assign_hash.hpp"
#include "jbms/array_view.hpp"

namespace jbms {
namespace multiset_hash {

template <bool C, class Field, class Hash>
__attribute__((noinline)) void test_assign_hash(typename Field::Element &result, jbms::array_view<void const> data) {
  Field F;
  Hash H;
  __asm__ volatile("");
  if (C) assign_hash(F, H, result, data);
}

template <class Field, class Hash>
void benchmark_binary_field_assign_hash() {

  auto do_benchmark = [&](auto N_t) {
    std::array<uint8_t, N_t.value> v;
    typename Field::Element a;
    benchmark_function("hash" + std::to_string(N_t.value),
                       [&](auto C){ test_assign_hash<C(),Field,Hash>(a,v); });
  };

  do_benchmark(std::integral_constant<size_t,10>{});
  do_benchmark(std::integral_constant<size_t,100>{});
  do_benchmark(std::integral_constant<size_t,150>{});
  do_benchmark(std::integral_constant<size_t,200>{});
}

}
}

#endif /* HEADER GUARD */
