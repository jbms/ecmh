#ifndef HEADER_GUARD_3894b6a1b1b0bfbfb971255456fe84c4
#define HEADER_GUARD_3894b6a1b1b0bfbfb971255456fe84c4

#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include "jbms/openssl/rand.hpp"
#include "jbms/assign_endian.hpp"
#include <array>
#include <vector>
#include "jbms/benchmark.hpp"

namespace jbms {
namespace multiset_hash {

template <bool C, class MSH, size_t N>
__attribute__((noinline)) void test_add(MSH const &msh, typename MSH::State &state, std::array<uint8_t, N> const &e) {
  __asm__ volatile("");
  if (C)
    add(msh, state, e);
}

template <bool C, class MSH, size_t N>
__attribute__((noinline)) void test_remove(MSH const &msh, typename MSH::State &state, std::array<uint8_t, N> const &e) {
  __asm__ volatile("");
  if (C)
    remove(msh, state, e);
}

template <bool C, class MSH, size_t N>
__attribute__((noinline)) void test_batch_add(MSH const &msh,
                                              typename MSH::State &state,
                                              std::vector<std::array<uint8_t, N>> const &elements) {
  __asm__ volatile("");
  if (C)
    batch_add(msh, state, elements);
}

template <bool C, class MSH, size_t N>
__attribute__((noinline)) void test_batch_remove(MSH const &msh,
                                                 typename MSH::State &state,
                                                 std::vector<std::array<uint8_t, N>> const &elements) {
  __asm__ volatile("");
  if (C)
    batch_remove(msh, state, elements);
}

template <bool C, class MSH>
__attribute__((noinline)) void test_invert(MSH const &msh, typename MSH::State &state, typename MSH::State const &x) {
  __asm__ volatile("");
  if (C)
    invert(msh, state, x);
}

template <bool C, class MSH, class Wrapper>
__attribute__((noinline)) void test_serialize(MSH const &msh, Wrapper data, typename MSH::State &state) {
  __asm__ volatile("");
  if (C)
    assign(msh, data, state);
}

template <bool C, class MSH, class Wrapper>
__attribute__((noinline)) void test_deserialize(MSH const &msh, typename MSH::State &state, Wrapper data) {
  __asm__ volatile("");
  if (C)
    assign(msh, state, data);
}

template <bool C, class MSH>
__attribute__((noinline)) bool test_equal(MSH const &msh, typename MSH::State const &a, typename MSH::State const &b) {
  __asm__ volatile("");
  if (C)
    return equal(msh, a, b);
  else
    return false;
}

template <class MSH>
void benchmark_generic_multiset_hash(MSH const &msh, bool benchmark_batch = true) {
  size_t num_elements = 256;

  using State = typename MSH::State;

  State s = initial_state(msh), s2 = initial_state(msh);

  // generate an arbitrary non-special value
  auto generate_non_special = [&] {
    auto x = initial_state(msh);
    for (size_t i = 0; i < 10; ++i) {
      std::array<uint8_t, 10> e;
      jbms::openssl::rand_pseudo_bytes(e);
      add(msh, x, e);
    }

    for (size_t i = 0; i < 20; ++i) {
      std::array<uint8_t, 10> e;
      jbms::openssl::rand_pseudo_bytes(e);
      remove(msh, x, e);
    }
    return x;
  };

  auto non_special1 = generate_non_special();
  auto non_special2 = generate_non_special();

  auto benchmark_add = [&](auto N_t) {
    std::vector<std::array<uint8_t, N_t.value>> elements(num_elements);
    for (auto &e : elements)
      jbms::openssl::rand_pseudo_bytes(e);

    benchmark_function(("add" + std::to_string(N_t.value)).c_str(),
                       [&](auto C) {
                         for (auto &&e : elements)
                           test_add<C()>(msh, s, e);
                       },
                       num_elements);
    benchmark_function(("remove" + std::to_string(N_t.value)).c_str(),
                       [&](auto C) {
                         for (auto &&e : elements)
                           test_remove<C()>(msh, s, e);
                       },
                       num_elements);
    if (benchmark_batch) {
      benchmark_function(("batch_add" + std::to_string(N_t.value)).c_str(),
                         [&](auto C) { test_batch_add<C()>(msh, s, elements); },
                         num_elements);
      benchmark_function(("batch_remove" + std::to_string(N_t.value)).c_str(),
                         [&](auto C) { test_batch_remove<C()>(msh, s, elements); },
                         num_elements);
    }
  };
  benchmark_add(std::integral_constant<size_t, 10>{}); // 1 64 or 128-byte block
  benchmark_add(std::integral_constant<size_t, 100>{}); // 2 64-byte or 1 128-byte block
  benchmark_add(std::integral_constant<size_t, 150>{}); // 3 64-byte or 2 128-byte blocks
  benchmark_add(std::integral_constant<size_t, 200>{}); // 4 64-byte or 2 128-byte blocks

  benchmark_function("invert", [&](auto C) { test_invert<C()>(msh, s2, non_special1); });
  benchmark_function("equal", [&](auto C) { test_equal<C()>(msh, non_special2, non_special1); });

  std::vector<uint8_t> buf(msh.num_bytes());
  benchmark_function("serialize_le", [&](auto C) { test_serialize<C()>(msh, jbms::little_endian(buf), non_special1); });
  benchmark_function("deserialize_le", [&](auto C) { test_deserialize<C()>(msh, s, jbms::little_endian(buf)); });

  benchmark_function("serialize_be", [&](auto C) { test_serialize<C()>(msh, jbms::big_endian(buf), non_special1); });
  benchmark_function("deserialize_be", [&](auto C) { test_deserialize<C()>(msh, s, jbms::big_endian(buf)); });
}
}
}

#endif /* HEADER GUARD */
