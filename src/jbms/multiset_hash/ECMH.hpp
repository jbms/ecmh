#ifndef HEADER_GUARD_5830dc5f99a3ae4c49a642f70168aa02
#define HEADER_GUARD_5830dc5f99a3ae4c49a642f70168aa02

#include "jbms/binary_elliptic_curve/sw.hpp"
#include "jbms/binary_elliptic_curve/compress_point.hpp"
#include "jbms/binary_field/assign_hash.hpp"
#include "jbms/binary_elliptic_curve/add.hpp"
#include <boost/range/adaptor/transformed.hpp>
#include <boost/function_output_iterator.hpp>
#include "./detail/operation_helpers.hpp"

namespace jbms {
namespace multiset_hash {

template <class Curve_, class Hash_>
class ECMH;

template <class Curve_, class Hash_>
struct is_multiset_hash<ECMH<Curve_,Hash_>> : std::true_type{};


template <class Curve_, class Hash_>
class ECMH {
public:
  using Curve = Curve_;
  using Field = typename Curve::Field;
  using FE = typename Field::Element;
  using Hash = Hash_;
  using Encoder = jbms::binary_elliptic_curve::sw::Encoder<Curve>;

  using State = jbms::binary_elliptic_curve::LambdaProjectivePoint<Curve>;

  friend void initialize(ECMH const &ecmh, State &state) {
    set_infinity(ecmh.curve(), state);
  }

private:

  Curve curve_;
  Encoder encoder_ = Encoder{ curve_ };
  Hash hash_;

public:

  ECMH() = default;

  explicit ECMH(Curve const &curve_)
    : curve_(curve_) {}

  explicit ECMH(Curve const &curve_, Hash const &hash_)
    : curve_(curve_), hash_(hash_) {}

  explicit ECMH(Curve const &curve_, Encoder const &encoder_, Hash const &hash_)
    : curve_(curve_), encoder_(encoder_), hash_(hash_)
  {}

  Curve const &curve() const { return curve_; }
  Field const &field() const { return curve().field(); }
  Hash const &hash() const { return hash_; }
  Encoder const &encoder() const { return encoder_; }

  size_t num_bytes() const { return curve().num_compressed_bytes(); }

  friend void add(ECMH const &ecmh, State &state, jbms::array_view<void const> x) {
    FE hx;
    assign_hash(ecmh.curve().field(), ecmh.hash(), hx, x);
    jbms::binary_elliptic_curve::LambdaAffinePoint<Curve> fhx;
    map(ecmh.curve(), ecmh.encoder(), fhx, hx);
    add(ecmh.curve(), state, state, fhx);
  }

  template <class ElementRange>
  friend void batch_add(ECMH const &ecmh, State &state, ElementRange const &element_range) {
    auto handle_fhx = [&](jbms::binary_elliptic_curve::LambdaAffinePoint<Curve> fhx) {
      add(ecmh.curve(), state, state, fhx);
    };

    auto transform_element = [&](auto const &element) {
      FE hx;
      assign_hash(ecmh.curve().field(), ecmh.hash(), hx, element);
      return hx;
    };
    auto w_range = element_range | boost::adaptors::transformed(transform_element);
    batch_map(ecmh.curve(), ecmh.encoder(),
              boost::make_function_output_iterator(handle_fhx),
              w_range);
  }

  friend void add_hash(ECMH const &ecmh, State &state, State const &x) {
    add(ecmh.curve(), state, state, x);
  }

  friend void remove(ECMH const &ecmh, State &state, jbms::array_view<void const> x) {
    FE hx;
    assign_hash(ecmh.curve().field(), ecmh.hash(), hx, x);
    jbms::binary_elliptic_curve::LambdaAffinePoint<Curve> fhx;
    map(ecmh.curve(), ecmh.encoder(), fhx, hx);
    negate(ecmh.curve(), fhx, fhx);
    add(ecmh.curve(), state, state, fhx);
  }

  template <class ElementRange>
  friend void batch_remove(ECMH const &ecmh, State &state, ElementRange const &element_range) {
    auto handle_fhx = [&](jbms::binary_elliptic_curve::LambdaAffinePoint<Curve> const &fhx) {
      add(ecmh.curve(), state, state, negate(ecmh.curve(), fhx));
    };

    auto transform_element = [&](auto const &element) {
      FE hx;
      assign_hash(ecmh.curve().field(), ecmh.hash(), hx, element);
      return hx;
    };
    auto w_range = element_range | boost::adaptors::transformed(transform_element);
    batch_map(ecmh.curve(), ecmh.encoder(),
              boost::make_function_output_iterator(handle_fhx),
              w_range);
  }


  friend void remove_hash(ECMH const &ecmh, State &state, State const &x) {
    add(ecmh.curve(), state, state, negate(ecmh.curve(), x));
  }

  friend void invert(ECMH const &ecmh, State &result, State const &state) {
    negate(ecmh.curve(), result, state);
  }

  friend bool equal(ECMH const &ecmh, State const &a, State const &b) {
    return equal(ecmh.curve(), a, b);
  }

  template <class Data, boost::endian::order order, JBMS_ENABLE_IF(is_byte_range<Data>)>
  friend void assign(ECMH const &ecmh, State &result, endian_wrapper<Data,order> data) {
    decompress_point(ecmh.curve(), result, data);
  }

  template <class Data, boost::endian::order order, JBMS_ENABLE_IF(is_byte_range<Data>)>
  friend void assign(ECMH const &ecmh, endian_wrapper<Data,order> result, State const &x) {
    compress_point(ecmh.curve(), result, x);
  }
};

}
}

#endif /* HEADER GUARD */
