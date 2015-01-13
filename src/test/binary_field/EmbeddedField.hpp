#ifndef HEADER_GUARD_836372a2aa6abb039f336e069aff67c4
#define HEADER_GUARD_836372a2aa6abb039f336e069aff67c4

#include "jbms/binary_field/detail/field_operation_helpers.hpp"

namespace jbms {
namespace binary_field {

template <class ExtensionField_, class BaseField_, class Embed_, class Extract_>
class EmbeddedField;

template <class ExtensionField_, class BaseField_, class Embed_, class Extract_>
struct is_field<EmbeddedField<ExtensionField_,BaseField_,Embed_,Extract_>> : std::true_type {};

template <class ExtensionField_, class BaseField_, class Embed_, class Extract_>
class EmbeddedField {
public:
  using ExtensionField = ExtensionField_;
  using BaseField = BaseField_;
  using Extract = Extract_;
  using Embed = Embed_;

  using Element = typename ExtensionField::Element;
  using DoubleElement = typename ExtensionField::DoubleElement;

private:
  ExtensionField extension_field_;
  BaseField base_field_;
  Embed embed_;
  Extract extract_;

public:
  constexpr ExtensionField const &extension_field() const { return extension_field_; }
  constexpr BaseField const &base_field() const { return base_field_; }
  constexpr Embed const &embed() const { return embed_; }
  constexpr Extract const &extract() const { return extract_; }

  EmbeddedField(ExtensionField const &extension_field_,
                BaseField const &base_field_,
                Embed const &embed_,
                Extract const &extract_)
    : extension_field_(extension_field_),
      base_field_(base_field_),
      embed_(embed_),
      extract_(extract_)
  {}

  template <class ElementT, JBMS_ENABLE_IF_C(std::is_same<ElementT,Element>::value ||
                                             std::is_same<ElementT,DoubleElement>::value)>
  friend std::string to_hex(EmbeddedField const &F, ElementT const &a) {
    typename BaseField::Element x;
    if (!F.extract()(F, x, a))
      throw std::runtime_error("embedding failure");
    return to_hex(F.base_field(), x);
  }

  template <class ElementT, class Range,
            JBMS_ENABLE_IF_C(std::is_same<ElementT,Element>::value ||
                             std::is_same<ElementT,DoubleElement>::value)>
  friend void assign_from_hex(EmbeddedField const &F, ElementT &a, Range const &range) {
    F.embed()(F, a, from_hex(F.base_field(), range));
  }

  template <class Data, boost::endian::order order>
  friend void assign(EmbeddedField const &F, Element &a, endian_wrapper<Data, order> source) {
    F.embed()(F, a, convert(F.base_field(), source));
  }

  template <class Data, boost::endian::order order>
  friend void assign(EmbeddedField const &F, endian_wrapper<Data, order> dest, Element const &a) {
    typename BaseField::Element x;
    if (!F.extract()(F, x, a))
      throw std::runtime_error("embedding failure");
    assign(F.base_field(), dest, x);
  }

  friend void assign(EmbeddedField const &F, Element &x, bool value) {
    assign(F.extension_field(), x, value);
  }

  friend void set_zero(EmbeddedField const &F, Element &x) {
    set_zero(F.extension_field(), x);
  }

  friend void set_one(EmbeddedField const &F, Element &x) {
    set_one(F.extension_field(), x);
  }

  friend bool is_zero(EmbeddedField const &F, Element const &x) {
    return is_zero(F.extension_field(), x);
  }

  friend bool is_one(EmbeddedField const &F, Element const &x) {
    return is_one(F.extension_field(), x);
  }

  friend bool equal(EmbeddedField const &F, Element const &a, Element const &b) {
    return equal(F.extension_field(), a, b);
  }

  friend bool trace(EmbeddedField const &F, Element const &x) {
    // Check if solve_quadratic produces a result in the field
    try {
      auto htr = solve_quadratic(F.extension_field(), x);
      // may throw an error if no solution exists
      typename BaseField::Element base_element;
      if (equal(F, add(F, htr, square(F, htr)), x) && F.extract()(F, base_element, htr))
        return false;
    } catch (...) {}
    return true;
  }

  friend void add(EmbeddedField const &F, Element &result, Element const &a, Element const &b) {
    add(F.extension_field(), result, a, b);
  }

  friend void multiply(EmbeddedField const &F, Element &result, Element const &a, Element const &b) {
    multiply(F.extension_field(), result, a, b);
  }

  friend void square(EmbeddedField const &F, Element &result, Element const &a) {
    square(F.extension_field(), result, a);
  }

  friend void invert(EmbeddedField const &F, Element &result, Element const &a) {
    invert(F.extension_field(), result, a);
  }

  friend void solve_quadratic(EmbeddedField const &F, Element &result, Element const &a) {
    solve_quadratic(F.extension_field(), result, a);
  }
};


template <class ExtensionField, class BaseField, class Embed, class Extract>
auto make_embedded_field(ExtensionField const &extension_field_,
                         BaseField const &base_field_,
                         Embed const &embed_,
                         Extract const &extract_) {
  return EmbeddedField<ExtensionField, BaseField, Embed, Extract>(extension_field_, base_field_, embed_, extract_);
}

}
}

#endif /* HEADER GUARD */
