#ifndef HEADER_GUARD_dca13d97c384a3f95f04d64e63b600ed
#define HEADER_GUARD_dca13d97c384a3f95f04d64e63b600ed

#include "jbms/binary_field/GF2m.hpp"
#include <boost/test/unit_test.hpp>
#include "./test_field_match.hpp"
#include "./test_generic_field.hpp"

namespace jbms {
namespace binary_field {

template <class Field>
void test_base_field_endian_convert(Field const &F) {
  using FE = typename Field::Element;

  int num_iters = 100;
  for (int iter = 0; iter < num_iters; ++iter) {
    FE a = pseudo_rand_element(F);

    // test endian conversion
    {
      std::vector<uint8_t> a_le, a_be;
      assign(F, little_endian(a_le), a);
      assign(F, big_endian(a_be), a);

      std::reverse(a_be.begin(), a_be.end());
      BOOST_REQUIRE_EQUAL_COLLECTIONS(a_le.begin(), a_le.end(), a_be.begin(), a_be.end());
    }
  }
}


template <class Field>
void test_base_field_io(Field const &F) {
  BOOST_REQUIRE_EQUAL("0", to_hex(F, zero(F)));
  BOOST_REQUIRE_EQUAL("0", to_hex(F, convert(F,false)));
  BOOST_REQUIRE_EQUAL("1", to_hex(F, one(F)));
  BOOST_REQUIRE_EQUAL("1", to_hex(F, convert(F,true)));

}

template <class Field, class Field1>
void test_reduce(Field const &F, Field1 const &F1) {
  using DoubleFE = typename Field::DoubleElement;
  using DoubleFE1 = typename Field1::DoubleElement;

  int num_iters = 100;

  for (int iter = 0; iter < num_iters; ++iter) {
    DoubleFE1 dbl1 = pseudo_rand_double_element(F1);
    DoubleFE dbl;
    assign_from_hex(F, dbl, to_hex(F1, dbl1));

    BOOST_REQUIRE_EQUAL(to_hex(F1, reduce(F1, dbl1)), to_hex(F, reduce(F, dbl)));
  }
}

template <class Field>
void test_base_field_half_trace(Field const &F) {
  using FE = typename Field::Element;

  static_assert(std::is_same<std::decay_t<decltype(half_trace(F, zero_expr(F)))>, Zero>::value,"");
  static_assert(std::is_same<std::decay_t<decltype(half_trace(F, one_expr(F)))>,
                             std::conditional_t<((Field::degree() - 1) / 2) % 2 == 0, One, Zero>>::value,
                "");

  int num_iters = 100;
  for (int iter = 0; iter < num_iters; ++iter) {
    FE a = pseudo_rand_element(F);
#define REQUIRE_HEX_EQUAL(a, b)  BOOST_REQUIRE_EQUAL(to_hex(F, a), to_hex(F, b));
    // test half trace: HTr(a) + HTr(a)^2 = a + Tr(a)
    {
      auto htr = half_trace(F, a);
      REQUIRE_HEX_EQUAL(add(F, htr, square(F, htr)), add(F, a, convert(F, trace(F, a))));
    }
#undef REQUIRE_HEX_EQUAL
  }
}

template <class Field>
void test_base_field_properties(Field const &F, GF2m const &F1) {
  test_generic_field_properties(F);
  test_base_field_io(F);
  test_generic_field_properties(F1);
  test_base_field_io(F1);
  test_match(F, F1);
  test_reduce(F, F1);
  test_base_field_endian_convert(F);
  test_base_field_endian_convert(F1);
  test_base_field_half_trace(F);
}


}
}


#endif /* HEADER GUARD */
