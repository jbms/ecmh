#ifndef HEADER_GUARD_8a365b920f822560308da10720fa1560
#define HEADER_GUARD_8a365b920f822560308da10720fa1560

#include <boost/test/unit_test.hpp>
#include "jbms/binary_field/rand_element.hpp"
#include "jbms/binary_field/batch_invert.hpp"

namespace jbms {
namespace binary_field {

#define REQUIRE_HEX_EQUAL(a, b)  BOOST_REQUIRE_EQUAL(to_hex(F, a), to_hex(F, b));

template <class Field>
void test_batch_invert(Field const &F, size_t batch_size) {
  using FE = typename Field::Element;
  std::vector<FE> input(batch_size), output(batch_size), input_copy(batch_size);
  for (auto &x : input)
    x = pseudo_rand_element(F);
  input_copy = input;

  batch_invert(F, output.begin(), input);

  for (size_t i = 0; i < batch_size; ++i) {
    REQUIRE_HEX_EQUAL(input[i], input_copy[i]);
    REQUIRE_HEX_EQUAL(output[i], invert(F, input_copy[i]));
  }
}

template <class Field>
void test_generic_field_properties(Field const &F) {
  using FE = typename Field::Element;

  static_assert(std::is_same<decltype(add(F, zero_expr(F), one_expr(F))),One>::value,"");
  static_assert(std::is_same<decltype(add(F, one_expr(F), zero_expr(F))),One>::value,"");
  static_assert(std::is_same<decltype(add(F, zero_expr(F), zero_expr(F))),Zero>::value,"");
  static_assert(std::is_same<decltype(add(F, one_expr(F), one_expr(F))),Zero>::value,"");

  static_assert(std::is_same<decltype(invert(F, one_expr(F))), One>::value,"");

  static_assert(std::is_same<decltype(solve_quadratic(F, zero_expr(F))), Zero>::value,"");

  static_assert(std::is_same<decltype(square(F, one_expr(F))), One>::value,"");
  static_assert(std::is_same<decltype(square(F, zero_expr(F))), Zero>::value,"");

  static_assert(std::is_same<decltype(multiply(F, zero_expr(F), std::declval<FE>())),Zero>::value,"");
  static_assert(std::is_same<decltype(multiply(F, std::declval<FE>(), zero_expr(F))),Zero>::value,"");
  static_assert(std::is_same<decltype(multiply(F, one_expr(F), zero_expr(F))),Zero>::value,"");

  REQUIRE_HEX_EQUAL(one(F), one_expr(F));
  REQUIRE_HEX_EQUAL(zero(F), zero_expr(F));

  BOOST_REQUIRE(equal(F, one(F), one_expr(F)));
  BOOST_REQUIRE(equal(F, one_expr(F), one_expr(F)));
  BOOST_REQUIRE(equal(F, one_expr(F), one(F)));
  BOOST_REQUIRE(equal(F, zero(F), zero_expr(F)));
  BOOST_REQUIRE(equal(F, zero_expr(F), zero(F)));
  BOOST_REQUIRE(equal(F, zero_expr(F), zero_expr(F)));
  BOOST_REQUIRE(!equal(F, one_expr(F), zero_expr(F)));
  BOOST_REQUIRE(!equal(F, zero_expr(F), one_expr(F)));


  int num_iters = 100;
  for (int iter = 0; iter < num_iters; ++iter) {
    FE a = pseudo_rand_element(F);
    FE b = pseudo_rand_element(F);
    FE c = pseudo_rand_element(F);

    BOOST_REQUIRE(!equal(F,a,b));
    BOOST_REQUIRE(!equal(F,a,c));
    BOOST_REQUIRE(!equal(F,b,c));

    BOOST_REQUIRE(!equal(F, a, add(F, a, one(F))));
    BOOST_REQUIRE(!equal(F, a, add(F, a, one_expr(F))));

    BOOST_REQUIRE_EQUAL(to_hex(F, a), to_hex(F, from_hex(F, to_hex(F, a))));




    // test endian conversion
    {
      std::vector<uint8_t> a_le, a_be;
      assign(F, little_endian(a_le), a);
      assign(F, big_endian(a_be), a);

      REQUIRE_HEX_EQUAL(a, convert(F, little_endian(a_le)));
      REQUIRE_HEX_EQUAL(a, convert(F, big_endian(a_be)));
    }

    // test addition commutative
    REQUIRE_HEX_EQUAL(add(F,a,b), add(F,b,a));

    // test addition associative
    REQUIRE_HEX_EQUAL(add(F,a,add(F,b,c)), add(F,add(F,a,b),c));

    // test addition identity
    REQUIRE_HEX_EQUAL(a, add(F,a,zero(F)));
    REQUIRE_HEX_EQUAL(a, add(F,a,zero_expr(F)));
    REQUIRE_HEX_EQUAL(a, add(F,zero_expr(F),a));
    REQUIRE_HEX_EQUAL(add(F,a,one(F)), add(F,a,one_expr(F)));
    REQUIRE_HEX_EQUAL(add(F,a,one(F)), add(F,one_expr(F),a));

    // test addition inverse
    REQUIRE_HEX_EQUAL(a, add(F,add(F,a,b),b));


    // test multiplication commutative
    REQUIRE_HEX_EQUAL(multiply(F,a,b), multiply(F,b,a));

    // test multiplication associative
    REQUIRE_HEX_EQUAL(multiply(F,a,multiply(F,b,c)), multiply(F,multiply(F,a,b),c));

    // test multiplication identity
    REQUIRE_HEX_EQUAL(a, multiply(F,a,one(F)));
    REQUIRE_HEX_EQUAL(a, multiply(F,a,one_expr(F)));
    REQUIRE_HEX_EQUAL(a, multiply(F,one_expr(F),a));

    // test multiplication inverse
    REQUIRE_HEX_EQUAL(one(F), multiply(F,a,invert(F,a)));

    // test multiplication zero property
    REQUIRE_HEX_EQUAL(zero(F), multiply(F,a,zero(F)));
    REQUIRE_HEX_EQUAL(zero(F), multiply(F,a,zero_expr(F)));
    REQUIRE_HEX_EQUAL(zero(F), multiply(F,zero_expr(F),a));

    // test multiplication distributive
    REQUIRE_HEX_EQUAL(add(F,multiply(F,a,b),multiply(F,a,c)), multiply(F, a, add(F, b, c)));


    // test squaring
    REQUIRE_HEX_EQUAL(square(F, a), multiply(F, a, a));

    // test solve quadratic: HTr(a) + HTr(a)^2 = a  if Tr(a) = 0
    if (!trace(F, a)) {
      auto v = solve_quadratic(F, a);
      REQUIRE_HEX_EQUAL(add(F, v, square(F, v)), a);
    }

    for (int batch_size = 0; batch_size < 10; ++batch_size) {
      test_batch_invert(F, batch_size);
    }
  }
}

#undef REQUIRE_HEX_EQUAL


}
}


#endif /* HEADER GUARD */
