#ifndef HEADER_GUARD_45e9b66d9a8fc17f72906dde9d362447
#define HEADER_GUARD_45e9b66d9a8fc17f72906dde9d362447

#include "jbms/benchmark.hpp"
#include "jbms/binary_elliptic_curve/sw.hpp"
#include "jbms/binary_elliptic_curve/add.hpp"
#include "jbms/binary_elliptic_curve/compress_point.hpp"
#include "./rand_point_not_infinity.hpp"
#include "jbms/binary_field/rand_element.hpp"

namespace jbms {
namespace binary_elliptic_curve {

template <bool C, class Curve, class Point1, class Point2, JBMS_ENABLE_IF_C(is_lambda_point<Curve,Point1>::value &&
                                                                    is_lambda_point<Curve,Point2>::value)>
__attribute__((noinline)) void test_add(Curve const &curve,
                                        LambdaProjectivePoint<Curve> &result,
                                        Point1 const &a,
                                        Point2 const &b) {
  __asm__ volatile("");
  if (C) add(curve, result, a, b);
}

template <bool C, class Curve, class Point1, JBMS_ENABLE_IF_C(is_lambda_point<Curve,Point1>::value)>
__attribute__((noinline)) void test_double_point(Curve const &curve,
                                                 LambdaProjectivePoint<Curve> &result,
                                                 Point1 const &a) {
  __asm__ volatile("");
  if (C) double_point(curve, result, a);
}

template <bool C, class Curve, class Point1, JBMS_ENABLE_IF_C(is_lambda_point<Curve,Point1>::value)>
__attribute__((noinline)) void test_negate(Curve const &curve,
                                           Point1 &result,
                                           Point1 const &a) {
  __asm__ volatile("");
  if (C) negate(curve, result, a);
}

template <bool C, class Curve, class Data, boost::endian::order order,
          class Point1, JBMS_ENABLE_IF_C(is_lambda_point<Curve,Point1>::value)>
__attribute__((noinline)) void test_compress(Curve const &curve,
                                             endian_wrapper<Data,order> result,
                                             Point1 const &a) {
  __asm__ volatile("");
  if (C) compress_point(curve, result, a);
}

template <bool C, class Curve, class Data, boost::endian::order order>
__attribute__((noinline)) void test_decompress(Curve const &curve,
                                               LambdaProjectivePoint<Curve> &result,
                                               endian_wrapper<Data,order> source) {
  __asm__ volatile("");
  if (C) decompress_point(curve, result, source);
}

template <bool C, class Curve>
__attribute__((noinline)) void test_sw_map(Curve const &curve,
                                           sw::Encoder<Curve> const &encoder,
                                           LambdaAffinePoint<Curve> &result,
                                           typename Curve::Field::Element const &w) {
  __asm__ volatile("");
  if (C) map(curve, encoder, result, w);
}

template <bool C, class Curve, class OutputIterator, class InputRange>
__attribute__((noinline)) void test_sw_map_batch(Curve const &curve,
                                                 sw::Encoder<Curve> const &encoder,
                                                 OutputIterator output_it, InputRange const &w_range) {
  __asm__ volatile("");
  if (C) batch_map(curve, encoder, output_it, w_range);
}





template <class Curve>
void benchmark_elliptic_curve(Curve const &curve) {

  LambdaAffinePoint<Curve> P_aff, Q_aff, result_aff;
  LambdaProjectivePoint<Curve> result;
  LambdaProjectivePoint<Curve> P_proj, Q_proj;

  size_t n = 256;
  std::vector<LambdaAffinePoint<Curve>> P_affs(n);
  std::vector<LambdaProjectivePoint<Curve>> P_projs;
  for (auto &P : P_affs) {
    P = rand_point_not_infinity(curve);
    LambdaProjectivePoint<Curve> P_p;
    assign(curve, P_p, P);
    P_projs.push_back(P_p);
  }

  P_aff = rand_point_not_infinity(curve);
  Q_aff = rand_point_not_infinity(curve);

  assign(curve, P_proj, P_aff);
  assign(curve, Q_proj, Q_aff);

  std::vector<uint8_t> arr(curve.num_compressed_bytes() * n);

  benchmark_function("add_laff", [&](auto C) { test_add<C()>(curve, result, P_aff, Q_aff); });
  benchmark_function("add_laff_lproj", [&](auto C){ test_add<C()>(curve, result, P_aff, Q_proj); });
  benchmark_function("add_lproj_laff", [&](auto C){ test_add<C()>(curve, result, P_proj, Q_aff); });
  benchmark_function("add_full", [&](auto C){ test_add<C()>(curve, result, P_proj, Q_proj); });

  benchmark_function("double_laff", [&](auto C){ test_double_point<C()>(curve, result, P_aff); });
  benchmark_function("double_lproj", [&](auto C){ test_double_point<C()>(curve, result, P_proj); });

  benchmark_function("negate_laff", [&](auto C){ test_negate<C()>(curve, result_aff, P_aff); });
  benchmark_function("negate_lproj", [&](auto C){ test_negate<C()>(curve, result, P_proj); });

  auto do_compress_decompress = [&](auto order) {
    std::string suffix = (order() == boost::endian::order::little ? "le" : "be");
    benchmark_function("compress_laff_" + suffix,
                       [&](auto C) {
                         for (auto &&P : P_affs)
                           test_compress<C()>(curve, jbms::make_endian_wrapper<order()>(arr), P);
                       },
                       n);
    benchmark_function("compress_lproj_" + suffix,
                       [&](auto C) {
                         for (auto &&P : P_projs)
                           test_compress<C()>(curve, jbms::make_endian_wrapper<order()>(arr), P);
                       },
                       n);

    for (size_t i = 0; i < n; ++i)
      test_compress<true>(curve,
                          jbms::make_endian_wrapper<order()>(
                              make_view(arr.data() + i * curve.num_compressed_bytes(), curve.num_compressed_bytes())),
                          P_projs[i]);
    benchmark_function("decompress_" + suffix,
                       [&](auto C) {
                         for (size_t i = 0; i < n; ++i)
                           test_decompress<C()>(
                               curve,
                               result,
                               jbms::make_endian_wrapper<order()>(
                                   make_view(arr.data() + i * curve.num_compressed_bytes(), curve.num_compressed_bytes())));
                       },
                       n);
  };

  do_compress_decompress(std::integral_constant<boost::endian::order, boost::endian::order::little>{});
  do_compress_decompress(std::integral_constant<boost::endian::order, boost::endian::order::big>{});

  sw::Encoder<Curve> encoder(curve);
  {
    // To properly reflect the average number of iterations (1 to 3) required by the SW algorithm, we need to average for multiple input values w rather than a single w value.
    size_t batch_size = 256;
    std::vector<typename Curve::Field::Element> w_arr(batch_size);
    for (auto &w : w_arr)
      w = pseudo_rand_element(curve.field());
    benchmark_function("sw",
                       [&](auto C) {
                         for (auto &&w : w_arr)
                           test_sw_map<C()>(curve, encoder, result_aff, w);
                       },
                       batch_size);
  }

  auto do_sw_batch = [&](size_t batch_size) {
    std::vector<LambdaAffinePoint<Curve>> result_arr(batch_size);
    std::vector<typename Curve::Field::Element> w_arr(batch_size);
    for (auto &w : w_arr)
      w = pseudo_rand_element(curve.field());
    benchmark_function("sw_batch" + std::to_string(batch_size),
                       [&](auto C) { test_sw_map_batch<C()>(curve, encoder, &result_arr[0], w_arr); },
                       batch_size);
  };

  for (size_t batch_size = 32; batch_size <= 256; batch_size *= 2) {
    do_sw_batch(batch_size);
  }
}


}
}

#endif /* HEADER GUARD */
