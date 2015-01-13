#ifndef HEADER_GUARD_2c8ee494a57d0e8b65c2ff2977f51bb2
#define HEADER_GUARD_2c8ee494a57d0e8b65c2ff2977f51bb2

#include <boost/test/unit_test.hpp>
#include "./test_generic_field.hpp"
#include "./EmbeddedField.hpp"
#include "jbms/binary_field/QuadraticExtension.hpp"

namespace jbms {
namespace binary_field {

template <class BaseField>
void test_quadratic_extension_io(QuadraticExtension<BaseField> const &F) {
  BOOST_REQUIRE_EQUAL("0,0", to_hex(F, zero(F)));
  BOOST_REQUIRE_EQUAL("0,0", to_hex(F, convert(F,false)));
  BOOST_REQUIRE_EQUAL("1,0", to_hex(F, one(F)));
  BOOST_REQUIRE_EQUAL("1,0", to_hex(F, convert(F,true)));
}

template <class BaseField>
auto quadratic_extension_embedded_base_field(QuadraticExtension<BaseField> const &F) {
  auto embed = [](auto const &F, auto &result, auto const &x) {
    result[0] = x;
    set_zero(F.base_field(), result[1]);
    return result;
  };
  auto extract = [](auto const &F, auto &result, auto const &x) {
    if (!is_zero(F.base_field(),x[1]))
      return false;
    result = x[0];
    return true;
  };

  return make_embedded_field(F, F.base_field(), embed, extract);
}

template <class BaseField>
void test_quadratic_extension_properties(QuadraticExtension<BaseField> const &F) {
  test_quadratic_extension_io(F);
  test_generic_field_properties(F);
  test_match(quadratic_extension_embedded_base_field(F), F.base_field());
}

}
}

#endif /* HEADER GUARD */
