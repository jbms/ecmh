#ifndef HEADER_GUARD_dca13d97c384a3f95f04d64e63b600eb
#define HEADER_GUARD_dca13d97c384a3f95f04d64e63b600eb

#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <random>
#include "jbms/binary_field/detail/apply_linear_table_transform.hpp"
#include "jbms/binary_field/batch_invert.hpp"
#include "jbms/binary_field/invert_blinded.hpp"
#include "jbms/binary_field/solve_quadratic_blinded.hpp"
#include "jbms/benchmark.hpp"
#include <boost/range/functions.hpp>
#include "jbms/static_repeat.hpp"
#include "jbms/hugepages.hpp"

#include <sys/mman.h>
#include <system_error>
#include <unistd.h>
#include <errno.h>

namespace jbms {
namespace binary_field {

template <bool C, class Field, class T>
__attribute__((noinline)) void test_add(T &result, T const &a, T const &b) {
  __asm__ volatile("");
  Field F;
  if (C)
    add(F, result, a, b);
}

template <bool C, class Field, class T>
__attribute__((noinline)) void test_multiply(T &result, T const &a, T const &b) {
  __asm__ volatile("");
  Field F;
  if (C) multiply(F, result, a, b);
}

template <bool C, class Field, class T>
__attribute__((noinline)) void test_square(T &result, T const &a) {
  __asm__ volatile("");
  Field F;
  if (C) square(F, result, a);
}

template <bool C, class Field, class T>
__attribute__((noinline)) void test_invert(T &result, T const &a) {
  __asm__ volatile("");
  Field F;
  if (C) invert(F, result, a);
}

template <bool C, class Field, class T>
__attribute__((noinline)) void test_invert_blinded(T &result, T const &a) {
  __asm__ volatile("");
  Field F;
  if (C) invert_blinded(F, result, a);
}

void invert_inline(...);

template <bool C, class Field, class T>
__attribute__((noinline)) void test_invert_inline(T &result, T const &a) {
  __asm__ volatile("");
  Field F;
  if (C) invert_inline(F, result, a);
}

template <size_t PrefixLen>
void invert_prefix(...);

template <bool C, size_t PrefixLen, class Field, class T>
__attribute__((noinline)) void test_invert_prefix(T &result, T const &a) {
  __asm__ volatile("");
  Field F;
  if (C) invert_prefix<PrefixLen>(F, result, a);
}


template <bool C, class Field, class Output, class Input>
__attribute__((noinline)) void test_batch_invert(Output &&output, Input const &input) {
  __asm__ volatile("");
  Field F;
  if (C) batch_invert(F, boost::begin(output), input);
}

template <bool C, class Field, class Output, class Input>
__attribute__((noinline)) void test_batch_invert_blinded(Output &&output, Input const &input) {
  __asm__ volatile("");
  Field F;
  if (C) batch_invert_blinded(F, boost::begin(output), input);
}

template <bool C, class Field, class T>
__attribute__((noinline)) bool test_trace(T const &a) {
  __asm__ volatile("");
  Field F;
  if (C) return trace(F, a);
  else return false;
}

template <bool C, class Field, class T>
__attribute__((noinline)) void test_solve_quadratic(T &result, T const &a) {
  __asm__ volatile("");
  Field F;
  if (C) solve_quadratic(F, result, a);
}

template <bool C, class Field, class T>
__attribute__((noinline)) void test_solve_quadratic_blinded(T &result, T const &a) {
  __asm__ volatile("");
  Field F;
  if (C) solve_quadratic_blinded(F, result, a);
}

template <class Field>
std::vector<typename Field::Element> generate_table_examples(size_t num_examples = 256) {
  Field F;
  constexpr size_t num_bytes = F.num_bytes();
  using ByteArray = std::array<uint8_t,num_bytes>;

  std::vector<ByteArray> byte_arrs(num_examples);
  std::default_random_engine generator; // use deterministic seed

  for (size_t j = 0; j < num_bytes; ++j) {
    std::vector<uint8_t> temp_arr(num_examples);
    std::iota(temp_arr.begin(), temp_arr.end(), 0);
    // Shuffle values so that table lookups using the examples in order access non-contiguous elements
    std::shuffle(temp_arr.begin(), temp_arr.end(), generator);
    for (size_t i = 0; i < num_examples; ++i) {
      auto &x = byte_arrs[i];
      // x[j] = i;
      x[j] = temp_arr[i];
    }
  }

  std::vector<typename Field::Element> fe_arr(num_examples);
  for (size_t i = 0; i < num_examples; ++i)
    assign(F, fe_arr[i], jbms::little_endian(byte_arrs[i]));

  return fe_arr;
}

template <bool C, class Field, size_t BlockBits, class T, class Table>
__attribute__((noinline)) void test_linear_table_transform(T &result, T const &a, Table const &table) {
  __asm__ volatile("");
  if (C) apply_linear_table_transform<BlockBits>(result, a, table);
}

#if 0
template <bool C, class Field, size_t BlockBits, class T, class... Table>
__attribute__((noinline)) void test_linear_table_transform_seq(T &result, T const &a, Table const &... table) {
  __asm__ volatile("");
  if (C) {
    T cur = a;
    auto l = { 0, (apply_linear_table_transform<BlockBits>(cur, cur, table),0)... };
    (void)l;
    result = cur;
  }
}
#endif

template <bool C, class Field, size_t BlockBits, class T, class... Table>
void test_linear_table_transform_seq(T &result, T const &a, Table const &... table) {
  auto l = { 0, (test_linear_table_transform<C, Field, BlockBits>(result, a, table), 0)... };
  (void)l;
}

template <bool C, class Field, size_t BlockBits, class T, class Table>
__attribute__((noinline)) void test_linear_table_transform_and_multiply(T &result, T const &a, Table const &table) {
  __asm__ volatile("");
  if (C) {
    apply_linear_table_transform<BlockBits>(result, a, table);
    Field F;
    multiply(F, result, result, a);
  }
}

template <bool C, class Field, size_t MultiSquareCount, size_t BlockBits, class Table, size_t NumTables>
__attribute__((noinline)) void test_linear_table_transform_and_multiply_and_multi_square(typename Field::Element &result, typename Field::Element const &a, std::array<Table,NumTables> const &tables) {
  __asm__ volatile("");
  if (C) {
    using FE = typename Field::Element;
    FE res = a;
    static_repeat<NumTables>([&](auto table_i) {
      FE temp;
      apply_linear_table_transform<BlockBits>(temp, a, tables[table_i]);
      Field F;
      multiply(F, res, res, temp);
      //static_repeat<MultiSquareCount>([&](auto sz) { square(F, res, res); });
      direct_multi_square<MultiSquareCount>(F, res, res);
      multiply(F, res, res, temp);
    });

    result = res;
  }
}

template <bool C, class Field, size_t RepeatCount, class T>
__attribute__((noinline)) void test_repeated_square_and_multiply(T &result, T const &a) {
  __asm__ volatile("");
  if (C) {
    Field F;
    T res;
    direct_multi_square<RepeatCount>(F, res, a);
    multiply(F, result, res, a);
  }
}

template <bool C, size_t ChainLength, class Field, size_t RepeatCount, class T>
__attribute__((noinline)) void test_repeated_square_and_multiply(T &result, T const &a) {
  __asm__ volatile("");
  if (C) {
    Field F;

    T x = a;
    static_repeat<ChainLength>([&](auto sz) {
      T res;
      direct_multi_square<RepeatCount>(F, res, x);
      multiply(F, x, res, x);
    });

    result = x;
  }
}

template <class Field, size_t BlockBits>
using linear_table_t = BinaryPolynomial<Field::degree()>[(Field::degree()+BlockBits-1)/BlockBits][1<<BlockBits];

namespace benchmark_detail {
template <size_t N>
using size_ = std::integral_constant<size_t, N>;
}

template <class Field>
void benchmark_linear_transform(bool measure_small = true, bool measure_huge = true, bool check_pagesize = true, bool check_with_multiply = false, bool check_without_multiply = false, bool check_with_multiply_and_multi_square = true) {
  using namespace benchmark_detail;

  auto do_benchmark_for_size = [&](auto BlockBits_t) {

    constexpr auto BlockBits = BlockBits_t.value;

    constexpr size_t num_examples = (BlockBits > 8) ? (size_t(1) << BlockBits) : size_t(256);
    auto examples = generate_table_examples<Field>(num_examples);
    (void)examples;


    auto prefix = "linear_transform" + std::to_string(BlockBits);

    constexpr size_t num_tables = 6;
    using Table = linear_table_t<Field, BlockBits>;

#if 0
    auto pseudo_randomize = [](jbms::array_view<void *> a) {
      std::default_random_engine generator; // use deterministic seed
      std::uniform_int_distribution<uint8_t> dist;
      for (auto &x : a)
        x = dist(generator);
    };
#endif

    auto allocate_tables = [&](bool use_huge_pages) {
      size_t bytes_used = sizeof(Table) * num_tables;
      size_t bytes_req = bytes_used;
      constexpr size_t huge_page_size = 2 * 1024 * 1024; // 2 MiB
      size_t page_size = use_huge_pages ? huge_page_size : ::sysconf(_SC_PAGESIZE);
      auto rem = bytes_req % page_size;
      if (rem > 0)
        bytes_req += (page_size - rem);
      size_t bytes_allocated = bytes_req;
      if (use_huge_pages) {
        // mmap might not give us memory aligned at the huge_page_size boundary, so we need some extra room to adjust it
        bytes_allocated += huge_page_size;
      }

      void *ptr = ::mmap(nullptr /* no address specified */,
                        bytes_allocated,
                        PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_PRIVATE /*| (use_huge_pages? MAP_HUGETLB : 0)*/,
                        -1 /* no fd */,
                        0 /* no file offset */);
      if (ptr == MAP_FAILED) {
        throw std::system_error(std::error_code(errno, std::system_category()), "mmap");
      }

      uint8_t *aligned_ptr = (uint8_t *)ptr;
      if (((std::uintptr_t)aligned_ptr % page_size) != 0) {
        aligned_ptr += (page_size - ((std::uintptr_t)aligned_ptr % page_size));
      }

      auto deleter = [ptr, bytes_allocated](void *addr) { ::munmap(ptr, bytes_allocated); };
      std::unique_ptr<Table[], decltype(deleter)> arr((Table *)aligned_ptr, deleter);

      if (::madvise(aligned_ptr, bytes_req, use_huge_pages ? MADV_HUGEPAGE : MADV_NOHUGEPAGE) != 0)
        throw std::system_error(std::error_code(errno, std::system_category()), "mmap");

      // zero-fill the memory region
      memset(aligned_ptr, 0, bytes_req);

      // Verify huge page allocation
      if (check_pagesize) {
        auto alloc = check_huge_page_allocation(aligned_ptr, bytes_req);
        // if we want huge pages, check minimum; otherwise check maximum
        size_t huge_page_amount = use_huge_pages ? alloc.first : alloc.second;
        size_t expected_huge_page_amount = use_huge_pages ? bytes_req : 0;
        if (huge_page_amount != expected_huge_page_amount) {
          throw std::runtime_error("Huge page allocation is " + std::to_string(huge_page_amount) + ", expected " + std::to_string(expected_huge_page_amount));
        }
      }
      return arr;
    };

    auto tables_smallpages = allocate_tables(false);
    auto tables_hugepages = allocate_tables(true);

    //auto tables_regular = allocate_hugepage_array<Table>(num_tables);//allocate_aligned();
    //memset(tables_regular.get(), 0, sizeof(Table) * num_tables);

    Field F;
    using FE = typename Field::Element;
    FE result;
    set_zero(F, result);

    static_repeat<num_tables>([&](auto n_tables_minus_1) {
      constexpr auto NumTables = n_tables_minus_1() + 1;

      auto const &tables_smallpages_part = *(std::array<Table,NumTables> const *)tables_smallpages.get();
      auto const &tables_hugepages_part = *(std::array<Table,NumTables> const *)tables_hugepages.get();

      auto do_for_both_page_sizes = [&](auto callback) {
        if (measure_small)
          callback(std::string("smallpage"), tables_smallpages_part);
        if (measure_huge)
          callback(std::string("hugepage"), tables_hugepages_part);
      };

      auto do_for_examples = [&](auto &&callback) {
        if (num_examples == 256) {
          auto loop_over_examples = [&](auto &&inner_callback) {
            for (auto &&e : examples)
              inner_callback(e);
          };
          callback(loop_over_examples);
        } else {
          size_t example_i = 0;
          auto loop_over_examples = [&](auto &&inner_callback) {
            example_i %= examples.size();
            for (size_t example_end = example_i + 256; example_i < example_end; ++example_i) {
              inner_callback(examples[example_i]);
            }
          };
          callback(loop_over_examples);
        }
      };

      if (check_without_multiply) {
        do_for_both_page_sizes([&](auto &&page_str, auto &&tables) {
          do_for_examples([&](auto &&loop_over_examples) {
            benchmark_function(prefix + "_" + std::to_string(NumTables) + "_" + page_str,
                               [&](auto C) {
                                 loop_over_examples([&](auto &&e) {
                                   for (auto &&table : tables)
                                     test_linear_table_transform<C(), Field, BlockBits>(result, e, table);
                                 });
                               },
                               256 * tables.size());
          });
        });
      }

      if (check_with_multiply) {

        do_for_both_page_sizes([&](auto &&page_str, auto &&tables) {
          do_for_examples([&](auto &&loop_over_examples) {
            benchmark_function(prefix + "_and_multiply_" + std::to_string(NumTables) + "_" + page_str,
                               [&](auto C) {
                                 loop_over_examples([&](auto &&e) {
                                   for (auto &&table : tables)
                                     test_linear_table_transform_and_multiply<C(), Field, BlockBits>(result, e, table);
                                 });
                               },
                               256 * tables.size());
          });
        });
      }

      if (check_with_multiply_and_multi_square) {
        do_for_both_page_sizes([&](auto &&page_str, auto &&tables) {
          do_for_examples([&](auto &&loop_over_examples) {
            benchmark_function(prefix + "_and_multiply_" + std::to_string(NumTables) + "_" + page_str + "_with_multi_square2",
                               [&](auto C) {
                                 loop_over_examples([&](auto &&e) {
                                   test_linear_table_transform_and_multiply_and_multi_square<C(), Field, 2, BlockBits>(
                                     result, e, tables);
                                 });
                               },
                               256 * tables.size());
          });
        });
      }
    });
  };

  static_repeat(do_benchmark_for_size, std::index_sequence<4,6,8,10,12,16>{});
}

template <class Field>
void run_core_benchmarks() {
  using namespace benchmark_detail;
  using FE = typename Field::Element;
  FE a, b, c;
  Field F;
  set_zero(F, a);
  set_zero(F, b);
  set_zero(F, c);

  benchmark_function("multiply", [&](auto C) { test_multiply<C(),Field>(a,b,c); });
  benchmark_function("add", [&](auto C) { test_add<C(),Field>(a,b,c); });
  benchmark_function("square", [&](auto C) { test_square<C(),Field>(a,b); });

#if 1
  static_repeat<20>([&](auto i_minus_1) {
    constexpr size_t n = i_minus_1() + 1;

    benchmark_function("multi_square" + std::to_string(n) + "_and_multiply",
                       [&](auto C) { test_repeated_square_and_multiply<C(), Field, n>(a, b); });

#if 1
    static_repeat([&](auto repeat_count) {
        benchmark_function("multi_square" + std::to_string(n) + "_and_multiply_" + std::to_string(repeat_count),
                           [&](auto C) { test_repeated_square_and_multiply<C(), repeat_count, Field, n>(a, b); }, repeat_count);
      }, std::index_sequence<1,4,8,16>{});
#endif
  });
#endif

}

template <class Field>
void run_benchmarks() {

  run_core_benchmarks<Field>();

  using FE = typename Field::Element;
  FE a, b, c;
  Field F;
  set_zero(F, a);
  set_zero(F, b);
  set_zero(F, c);

  auto examples = generate_table_examples<Field>();

#if 0
  benchmark_function("invert_inline", [&](auto C) { for (auto &&e : examples) test_invert_inline<C(),Field>(a, e); }, examples.size());
#endif

  benchmark_function_foreach_simple("invert", [&](auto C, auto &&e) { test_invert<C(),Field>(a, e); }, examples);
  benchmark_function_foreach_simple("invert_blinded", [&](auto C, auto &&e) { test_invert_blinded<C(),Field>(a, e); }, examples);

#if 0
  static_repeat<20>([&](auto i_minus_1) {
    constexpr size_t i = i_minus_1() + 1;
    benchmark_function("invert_prefix_" + std::to_string(i),
                       [&](auto C) {
                         for (auto &&e : examples)
                           test_invert_prefix<C(), i, Field>(a, e);
                       },
                       examples.size());
  });
#endif

  benchmark_function("trace", [&](auto C) { test_trace<C(),Field>(a); });
  benchmark_function_foreach_simple("solve_quadratic", [&](auto C, auto &&e) { test_solve_quadratic<C(),Field>(a, e); }, examples);
  benchmark_function_foreach_simple("solve_quadratic_blinded", [&](auto C, auto &&e) { test_solve_quadratic_blinded<C(),Field>(a, e); }, examples);

  static_repeat<9>([&](auto i) {
    constexpr size_t n = size_t(1) << i();
    std::vector<FE> input(n, one(F)), output(n);
    benchmark_function("batch_invert" + std::to_string(n), [&](auto C) { test_batch_invert<C(), Field>(output, input); }, n);
    benchmark_function("batch_invert_blinded" + std::to_string(n), [&](auto C) { test_batch_invert_blinded<C(), Field>(output, input); }, n);
  });

}
}
}

#endif /* HEADER GUARD */
