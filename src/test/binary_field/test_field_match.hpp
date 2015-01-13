#ifndef HEADER_GUARD_a592099900759922ed9e7c53d02e23fa
#define HEADER_GUARD_a592099900759922ed9e7c53d02e23fa

#include <boost/test/unit_test.hpp>
#include "jbms/binary_field/rand_element.hpp"

namespace jbms {
namespace binary_field {


template <size_t N, class Field, class Field1, JBMS_ENABLE_IF_C(N == 0)>
void test_multi_square(Field const &F, Field1 const &F1,
                       typename Field::Element const &a,
                       typename Field1::Element const &a1) {
}

template <size_t N, class Field, class Field1, JBMS_ENABLE_IF_C(N > 0)>
void test_multi_square(Field const &F, Field1 const &F1,
                       typename Field::Element const &a,
                       typename Field1::Element const &a1) {

  test_multi_square<N-1>(F, F1, a, a1);

  using jbms::binary_field::multi_square;
  BOOST_REQUIRE_EQUAL(to_hex(F1, multi_square<N>(F1,a1)), to_hex(F, multi_square<N>(F,a)));
}

template <class Field, class Field1>
void test_match(Field const &F, Field1 const &F1) {
  using FE = typename Field::Element;
  using FE1 = typename Field1::Element;

  int num_iters = 100;

  for (int iter = 0; iter < num_iters; ++iter) {

    FE1 a1 = pseudo_rand_element(F1);
    FE1 b1;
    do {
      b1 = pseudo_rand_element(F1);
    } while (equal(F1, b1, a1));

    FE1 c1;
    do {
      c1 = pseudo_rand_element(F1);
    } while (equal(F1, b1, c1) || equal(F1, c1, a1));

    FE a = from_hex(F, to_hex(F1, a1));
    FE b = from_hex(F, to_hex(F1, b1));
    FE c = from_hex(F, to_hex(F1, c1));

    {
      std::vector<uint8_t> a_le, a1_le, a_be, a1_be;
      assign(F, little_endian(a_le), a);
      assign(F, big_endian(a_be), a);
      assign(F1, little_endian(a1_le), a1);
      assign(F1, big_endian(a1_be), a1);

      BOOST_REQUIRE_EQUAL_COLLECTIONS(a_le.begin(), a_le.end(), a1_le.begin(), a1_le.end());
      BOOST_REQUIRE_EQUAL_COLLECTIONS(a_be.begin(), a_be.end(), a1_be.begin(), a1_be.end());
    }


    BOOST_REQUIRE_EQUAL(to_hex(F1, a1), to_hex(F, a));
    BOOST_REQUIRE_EQUAL(to_hex(F1, b1), to_hex(F, b));
    BOOST_REQUIRE_EQUAL(to_hex(F1, c1), to_hex(F, c));

    BOOST_REQUIRE(equal(F, a, a));
    BOOST_REQUIRE(!equal(F, a, add(F, a, one(F))));
    BOOST_REQUIRE(!equal(F, a, b));

    BOOST_REQUIRE_EQUAL(to_hex(F1, add(F1, a1, b1)), to_hex(F, add(F, a, b)));
    BOOST_REQUIRE_EQUAL(to_hex(F1, square(F1, a1)), to_hex(F, square(F, a)));
    BOOST_REQUIRE_EQUAL(trace(F1, a1), trace(F, a));

    if (trace(F1,a1) == false) {
      BOOST_REQUIRE_EQUAL(to_hex(F1, solve_quadratic(F1, a1)), to_hex(F, solve_quadratic(F, a)));
    }

    BOOST_REQUIRE_EQUAL(to_hex(F1, multiply(F1, a1, b1)), to_hex(F, multiply(F, a, b)));

    // multi-square
    //test_multi_square<F.degree()>(F, F1, a, a1);

    BOOST_REQUIRE_EQUAL(to_hex(F1, invert(F1, a1)), to_hex(F, invert(F, a)));

  }
}

}
}


#endif /* HEADER GUARD */
