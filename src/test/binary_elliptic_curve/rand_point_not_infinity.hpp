#ifndef HEADER_GUARD_924585bc76f6478f5a7a5f93df763c95
#define HEADER_GUARD_924585bc76f6478f5a7a5f93df763c95

#include "jbms/binary_elliptic_curve/Point.hpp"
#include "jbms/binary_field/rand_element.hpp"

namespace jbms {
namespace binary_elliptic_curve {

template <class Curve>
LambdaAffinePoint<Curve> rand_point_not_infinity(Curve const &curve) {
  LambdaAffinePoint<Curve> P;
  while (true) {
    P.x() = pseudo_rand_element(curve.field());
    if (is_zero(curve.field(), P.x())) {
      set_zero(curve.field(), P.m()); // special non-lambda point
      return P;
    } else {
      valid_lambda_from_non_zero_x(curve, P.m(), P.x());
      if (is_rational(curve, P))
        return P;
    }
  }
}

}
}

#endif /* HEADER GUARD */
