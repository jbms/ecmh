#ifndef HEADER_GUARD_ad763b657bee4d6d63f945a352d325eb
#define HEADER_GUARD_ad763b657bee4d6d63f945a352d325eb

#include <boost/test/unit_test.hpp>
#include "jbms/binary_elliptic_curve/Point.hpp"
#include "jbms/binary_elliptic_curve/compress_point.hpp"
#include "jbms/binary_elliptic_curve/add.hpp"
#include "jbms/binary_elliptic_curve/sw.hpp"
#include "jbms/binary_elliptic_curve/sw_blinded.hpp"
#include "jbms/assign_endian.hpp"
#include "jbms/binary_field/rand_element.hpp"
#include "jbms/openssl/ec.hpp"
#include <iostream>
#include <openssl/objects.h>
#include "./rand_point_not_infinity.hpp"

#define REQUIRE_HEX_EQUAL(a, b)  BOOST_REQUIRE_EQUAL(to_affine_hex(curve, a), to_affine_hex(curve, b))

namespace jbms {
namespace binary_elliptic_curve {

template <class Curve>
void test_compress_point(Curve const &curve, LambdaProjectivePoint<Curve> const &P1) {
  std::vector<uint8_t> P1_le;
  std::vector<uint8_t> P1_be;
  compress_point(curve, jbms::little_endian(P1_le), P1);
  compress_point(curve, jbms::big_endian(P1_be), P1);

  LambdaProjectivePoint<Curve> P1_2, P1_3;
  decompress_point(curve, P1_2, jbms::little_endian(P1_le));
  decompress_point(curve, P1_3, jbms::big_endian(P1_be));

  BOOST_REQUIRE(equal(curve, P1_2, P1));
  BOOST_REQUIRE(equal(curve, P1_3, P1));
  REQUIRE_HEX_EQUAL(P1_2, P1);
  REQUIRE_HEX_EQUAL(P1_3, P1);
}

template <class Curve>
void test_sw_encode(Curve const &curve) {
  sw::Encoder<Curve> encoder(curve);

  LambdaProjectivePoint<Curve> P_non_lambda;
  set_non_lambda_point(curve, P_non_lambda);

  int num_iter = 100;
  // Test non-batch
  for (int i = 0; i < num_iter; ++i) {
    LambdaAffinePoint<Curve> P, P_blinded, P1, P1_blinded;
    auto w = pseudo_rand_element(curve.field());
    map(curve, encoder, P, w);
    map_blinded(curve, encoder, P_blinded, w);
    REQUIRE_HEX_EQUAL(P, P_blinded);

    map(curve, encoder, P1, add(curve.field(), w, one_expr(curve.field())));
    map(curve, encoder, P1_blinded, add(curve.field(), w, one_expr(curve.field())));
    BOOST_REQUIRE(is_rational(curve, P));
    REQUIRE_HEX_EQUAL(P, P1);
    REQUIRE_HEX_EQUAL(P, P1_blinded);

    // verify that at least two different points are possible
    LambdaAffinePoint<Curve> Q;
    do {
      auto w2 = pseudo_rand_element(curve.field());
      map(curve, encoder, Q, w2);
    } while (equal(curve, P, Q));
  }

  using FE = typename Curve::Field::Element;
  FE w_special;
  solve_quadratic(curve.field(), w_special, curve.a());

  // Test c = 0 special case (only applies if Tr(curve.a()) == 0
  if (trace(curve.field(), curve.a()) == 0) {
    {
      LambdaAffinePoint<Curve> P;
      map(curve, encoder, P, w_special);
      BOOST_REQUIRE(equal(curve, P, P_non_lambda));
    }
    {
      LambdaAffinePoint<Curve> P;
      map_blinded(curve, encoder, P, w_special);
      BOOST_REQUIRE(equal(curve, P, P_non_lambda));
    }
  }

  // Test batch
  for (size_t batch_size = 0; batch_size < 10; ++batch_size) {
    std::vector<typename Curve::Field::Element> w_arr(batch_size);
    std::vector<LambdaAffinePoint<Curve>> result_arr(batch_size);
    std::vector<LambdaAffinePoint<Curve>> result_arr_blinded(batch_size);
    for (size_t j = 0; j < batch_size; ++j)
      w_arr[j] = pseudo_rand_element(curve.field());

    // Make sure batch_map handles c = 0 special case as well
    if (trace(curve.field(), curve.a()) == 0) {
      if (batch_size > 0) {
        w_arr.front() = w_special;
        w_arr.back() = w_special;
      }
    }

    batch_map(curve, encoder, result_arr.begin(), w_arr);
    batch_map_blinded(curve, encoder, result_arr_blinded.begin(), w_arr);
    for (size_t j = 0; j < batch_size; ++j) {
      LambdaAffinePoint<Curve> result;
      map(curve, encoder, result, w_arr[j]);
      REQUIRE_HEX_EQUAL(result, result_arr[j]);
      REQUIRE_HEX_EQUAL(result, result_arr_blinded[j]);
    }
  }
}

template <class Curve>
void test_generic_curve_properties(Curve const &curve) {
  LambdaProjectivePoint<Curve> P_inf;
  set_infinity(curve, P_inf);

  LambdaProjectivePoint<Curve> P_non_lambda, P_non_lambda_a;
  set_non_lambda_point(curve, P_non_lambda);
  set_non_lambda_point(curve, P_non_lambda_a);

  BOOST_REQUIRE_EQUAL("inf", to_affine_hex(curve, P_inf));
  BOOST_REQUIRE_EQUAL("inf", to_affine_hex(curve, negate(curve, P_inf)));

  {
    LambdaProjectivePoint<Curve> P;
    assign_from_affine_hex(curve, P, std::string("inf"));
    BOOST_REQUIRE(equal(curve, P_inf, P));
  }

  {
    BOOST_REQUIRE_EQUAL(to_hex(curve.field(), zero(curve.field())) + " " + to_hex(curve.field(), curve.sqrt_b()),
                        to_affine_hex(curve, P_non_lambda));
  }

  {
    LambdaAffinePoint<Curve> P;
    set_non_lambda_point(curve, P);
    BOOST_REQUIRE_EQUAL(to_hex(curve.field(), zero(curve.field())) + " " + to_hex(curve.field(), curve.sqrt_b()),
                        to_affine_hex(curve, P));
  }

  test_compress_point(curve, P_inf);
  test_compress_point(curve, P_non_lambda);

  BOOST_REQUIRE(is_rational(curve, P_inf));
  BOOST_REQUIRE(is_rational(curve, P_non_lambda));
  BOOST_REQUIRE(is_rational(curve, P_non_lambda_a));

  REQUIRE_HEX_EQUAL(P_non_lambda, P_non_lambda_a);

  int num_iters = 100;

  test_sw_encode(curve);

  for (int iter = 0; iter < num_iters; ++iter) {
    LambdaAffinePoint<Curve> P1_a, P2_a, P3_a;
    P1_a = rand_point_not_infinity(curve);

    BOOST_REQUIRE(equal(curve, P1_a, P1_a));

    do {
      P2_a = rand_point_not_infinity(curve);
    } while (equal(curve, P2_a, P1_a) || equal(curve, P2_a, negate(curve, P1_a)));

    do {
      P3_a = rand_point_not_infinity(curve);
    } while (equal(curve, P3_a, P1_a) || equal(curve, P3_a, negate(curve, P1_a)) ||
             equal(curve, P3_a, P2_a) || equal(curve, P3_a, negate(curve, P2_a)));

    BOOST_REQUIRE(to_affine_hex(curve, P1_a) != to_affine_hex(curve, P2_a));

    BOOST_REQUIRE(!equal(curve, P1_a, P_inf));

    BOOST_REQUIRE(to_affine_hex(curve, P1_a) != to_affine_hex(curve, P_inf));

    LambdaProjectivePoint<Curve> P1(curve, P1_a), P2(curve, P2_a), P3(curve, P3_a);

    BOOST_REQUIRE(equal(curve, P1, P1_a));

    {
      LambdaProjectivePoint<Curve> P1_a2;
      assign(curve, P1_a2, P1);
      BOOST_REQUIRE(equal(curve, P1_a2, P1_a));
    }

    // test compress point
    test_compress_point(curve, P1);

    // Test commutativity
    REQUIRE_HEX_EQUAL(add(curve, P1, P2), add(curve, P2, P1));
    REQUIRE_HEX_EQUAL(add(curve, P1_a, P2_a), add(curve, P2_a, P1_a)); // affine version
    REQUIRE_HEX_EQUAL(add(curve, P1, P_non_lambda), add(curve, P_non_lambda, P1));
    BOOST_REQUIRE(is_rational(curve, add(curve, P1, P2)));
    BOOST_REQUIRE(!is_infinity(curve, add(curve, P1, P2)));
    BOOST_REQUIRE(is_rational(curve, add(curve, P1, P_non_lambda)));

    // Test associativity
    REQUIRE_HEX_EQUAL(add(curve, P1, add(curve, P2, P3)), add(curve, add(curve, P1, P2), P3));
    BOOST_REQUIRE(is_rational(curve, add(curve, P1, add(curve, P2, P3))));
    REQUIRE_HEX_EQUAL(add(curve, P1, add(curve, P2, P_non_lambda)), add(curve, add(curve, P1, P2), P_non_lambda));
    BOOST_REQUIRE(is_rational(curve, add(curve, P1, add(curve, P2, P_non_lambda))));

    // Test doubling
    //std::cout << to_affine_hex(curve, double_point(curve, P1)) << std::endl;
    BOOST_REQUIRE(is_rational(curve, double_point(curve, P1)));
    REQUIRE_HEX_EQUAL(add(curve, double_point(curve,P1), negate(curve,P1)), P1);
    BOOST_REQUIRE(is_rational(curve, double_point(curve, P_non_lambda)));
    REQUIRE_HEX_EQUAL(double_point(curve,P_non_lambda), P_inf);
    REQUIRE_HEX_EQUAL(double_point(curve,P_inf), P_inf);


    // Test identity
    REQUIRE_HEX_EQUAL(add(curve, P1, P_inf), P1);
    REQUIRE_HEX_EQUAL(add(curve, P_non_lambda, P_inf), P_non_lambda);

    // Test inverse
    REQUIRE_HEX_EQUAL(add(curve, P1, negate(curve, P1)), P_inf);
    BOOST_REQUIRE(is_rational(curve, negate(curve, P1)));

    // Test non-lambda result
    REQUIRE_HEX_EQUAL(add(curve, add(curve, P1, P_non_lambda), negate(curve, P1)), P_non_lambda);
    REQUIRE_HEX_EQUAL(add(curve, negate(curve, P1), add(curve, P1, P_non_lambda)), P_non_lambda);


    // Test with projective z != 1
    LambdaProjectivePoint<Curve> P1_p, P2_p, P3_p;
    do {
      auto temp = rand_point_not_infinity(curve);
      add(curve, P1_p, add(curve, P1, temp), negate(curve, temp));
    } while (is_affine(curve, P1_p));

    do {
      auto temp = rand_point_not_infinity(curve);
      add(curve, P2_p, add(curve, P2, temp), negate(curve, temp));
    } while (is_affine(curve, P1_p));

    do {
      auto temp = rand_point_not_infinity(curve);
      add(curve, P3_p, add(curve, P3, temp), negate(curve, temp));
    } while (is_affine(curve, P3_p));

    REQUIRE_HEX_EQUAL(P1_p, P1);
    BOOST_REQUIRE(equal(curve, P1_p, P1));
    BOOST_REQUIRE(equal(curve, P1, P1_p));
    BOOST_REQUIRE(equal(curve, P1_p, P1_a));
    BOOST_REQUIRE(equal(curve, P1_a, P1_p));

    // Test that mixed and affine addition are equivalent to full addition
    REQUIRE_HEX_EQUAL(add(curve, P1_p, P2_p), add(curve, P1, P2));
    REQUIRE_HEX_EQUAL(add(curve, P1_p, P_non_lambda), add(curve, P1, P_non_lambda));
    REQUIRE_HEX_EQUAL(add(curve, P1_a, P_non_lambda_a), add(curve, P1, P_non_lambda));
    REQUIRE_HEX_EQUAL(add(curve, P1, P_non_lambda_a), add(curve, P1, P_non_lambda));

    REQUIRE_HEX_EQUAL(add(curve, P_non_lambda, P1_p), add(curve, P1, P_non_lambda));
    REQUIRE_HEX_EQUAL(add(curve, P_non_lambda_a, P1_a), add(curve, P1, P_non_lambda));
    REQUIRE_HEX_EQUAL(add(curve, P_non_lambda_a, P1), add(curve, P1, P_non_lambda));
    REQUIRE_HEX_EQUAL(add(curve, P_non_lambda_a, P_non_lambda), P_inf);
    REQUIRE_HEX_EQUAL(add(curve, P_non_lambda_a, P_non_lambda_a), P_inf);
    REQUIRE_HEX_EQUAL(add(curve, P_non_lambda, P_non_lambda_a), P_inf);

    REQUIRE_HEX_EQUAL(add(curve, P1_p, P2_p), add(curve, P1_a, P2_a));
    REQUIRE_HEX_EQUAL(add(curve, P1_p, P2_a), add(curve, P1, P2));
    REQUIRE_HEX_EQUAL(add(curve, P1_a, P2_p), add(curve, P1, P2));

    REQUIRE_HEX_EQUAL(double_point(curve, P1_p), double_point(curve, P1));

    REQUIRE_HEX_EQUAL(add(curve, P1_p, add(curve, P2_p, P3_p)), add(curve, P1_a, add(curve, P2_a, P3_a)));

    REQUIRE_HEX_EQUAL(negate(curve, P1_p), negate(curve, P1_a));
    REQUIRE_HEX_EQUAL(negate(curve, P1_p), negate(curve, P1));

    AffinePoint<Curve> P1_aa;
    assign(curve, P1_aa, P1_p);
    REQUIRE_HEX_EQUAL(P1_aa, P1_p);
  }

}

#undef REQUIRE_HEX_EQUAL

#if 0
inline jbms::binary_field::GF2m get_field(jbms::openssl::ec_group const &curve) {
  jbms::openssl::bignum p, a, b;
  curve.get_curve_GF2m(p, a, b);
  return jbms::binary_field::GF2m(GF2m_poly2arr(p));
}
#endif

inline std::string to_affine_hex(jbms::openssl::ec_group const &curve, jbms::openssl::ec_point const &P) {
  if (is_at_infinity(curve, P))
    return "inf";
  jbms::openssl::bignum x, y;
  get_affine_coordinates_GF2m(curve, P, x, y);
  return x.to_canonical_hex() + " " + y.to_canonical_hex();
}

inline void assign_from_affine_hex(jbms::openssl::ec_group const &curve, jbms::openssl::ec_point &P, std::string const &s) {
  if (s == "inf")
    set_to_infinity(curve, P);
  else {
    auto sep_pos = s.find(' ');
    if (sep_pos == std::string::npos)
      throw std::invalid_argument("Curve affine hex representation must contain a space");

    jbms::openssl::bignum x, y;
    x.set_from_hex(s.substr(0, sep_pos));
    y.set_from_hex(s.substr(sep_pos + 1));
    set_affine_coordinates_GF2m(curve, P, x, y);
  }
}

inline jbms::openssl::ec_point
add(jbms::openssl::ec_group const &curve, jbms::openssl::ec_point const &P, jbms::openssl::ec_point const &Q) {
  jbms::openssl::ec_point result(curve);
  add(curve, result, P, Q);
  return result;
}

inline jbms::openssl::ec_point
double_point(jbms::openssl::ec_group const &curve, jbms::openssl::ec_point const &P) {
  jbms::openssl::ec_point result(curve);
  dbl(curve, result, P);
  return result;
}

inline jbms::openssl::ec_point
negate(jbms::openssl::ec_group const &curve, jbms::openssl::ec_point const &P) {
  jbms::openssl::ec_point result(P, curve);
  invert(curve, result);
  return result;
}


template <class Curve>
void test_curve_against_openssl(Curve const &curve, jbms::openssl::ec_group const &ec_g) {
  using jbms::openssl::ec_point;
  jbms::openssl::bn_ctx ctx;

#define REQUIRE_HEX_EQUAL2(a, b) BOOST_REQUIRE_EQUAL(to_affine_hex(curve,a), to_affine_hex(ec_g, b))

  LambdaProjectivePoint<Curve> P_inf, P1, P2, P3;
  set_infinity(curve, P_inf);

  LambdaProjectivePoint<Curve> P_non_lambda;
  set_non_lambda_point(curve, P_non_lambda);

  ec_point Q_inf(ec_g), Q1(ec_g), Q2(ec_g), Q3(ec_g), Q_non_lambda(ec_g);
  set_to_infinity(ec_g, Q_inf);

  {
    jbms::openssl::bignum p, a, b;
    ec_g.get_curve_GF2m(p, a, b, ctx);
    jbms::openssl::bignum b_sqrt;
    GF2m_mod_sqrt(b_sqrt, b, p, ctx);
    jbms::openssl::bignum zero_val;
    zero_val.set_zero();
    set_affine_coordinates_GF2m(ec_g, Q_non_lambda, zero_val, b_sqrt, ctx);
  }

  REQUIRE_HEX_EQUAL2(P_inf, Q_inf);
  REQUIRE_HEX_EQUAL2(P_non_lambda, Q_non_lambda);

  int num_iters = 100;
  for (int i = 0; i < num_iters; ++i) {
    assign(curve, P1, rand_point_not_infinity(curve));
    do {
      assign(curve, P2, rand_point_not_infinity(curve));
    } while (equal(curve, P1, P2));

    do {
      assign(curve, P3, rand_point_not_infinity(curve));
    } while (equal(curve, P1, P3) || equal(curve, P2, P3));

    assign_from_affine_hex(ec_g, Q1, to_affine_hex(curve, P1));
    assign_from_affine_hex(ec_g, Q2, to_affine_hex(curve, P2));
    assign_from_affine_hex(ec_g, Q3, to_affine_hex(curve, P3));


    REQUIRE_HEX_EQUAL2(P1, Q1);
    REQUIRE_HEX_EQUAL2(P2, Q2);
    REQUIRE_HEX_EQUAL2(P3, Q3);

    REQUIRE_HEX_EQUAL2(add(curve, P1, P2), add(ec_g, Q1, Q2));
    REQUIRE_HEX_EQUAL2(add(curve, P1, add(curve, P2,P3)), add(ec_g, Q1, add(ec_g, Q2,Q3)));

    REQUIRE_HEX_EQUAL2(add(curve, P1, P_inf), add(ec_g, Q1, Q_inf));
    REQUIRE_HEX_EQUAL2(add(curve, P1, P_non_lambda), add(ec_g, Q1, Q_non_lambda));
    REQUIRE_HEX_EQUAL2(add(curve, P_non_lambda, P1), add(ec_g, Q_non_lambda, Q1));

    REQUIRE_HEX_EQUAL2(double_point(curve, P1), double_point(ec_g, Q1));
    REQUIRE_HEX_EQUAL2(double_point(curve, P_inf), double_point(ec_g, Q_inf));
    REQUIRE_HEX_EQUAL2(negate(curve, P_inf), negate(ec_g, Q_inf));
    REQUIRE_HEX_EQUAL2(negate(curve, P1), negate(ec_g, Q1));
  }
#undef REQUIRE_HEX_EQUAL2

}

template <class Curve>
void test_curve_against_openssl(Curve const &curve, std::string const &curve_name) {
  test_curve_against_openssl(curve, jbms::openssl::ec_group::by_curve_name(OBJ_txt2nid(curve_name.c_str())));
}

}
}

#endif /* HEADER GUARD */
