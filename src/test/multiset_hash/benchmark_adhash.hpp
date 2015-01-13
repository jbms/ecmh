#ifndef HEADER_GUARD_cedb8c2b028c2d460788a4749050bdc1
#define HEADER_GUARD_cedb8c2b028c2d460788a4749050bdc1

#include "jbms/multiset_hash/AdHash.hpp"
#include "./benchmark_adhash_or_muhash.hpp"

namespace jbms {
namespace multiset_hash {

template <class Hash>
void benchmark_adhash(int bits, Hash const &H) {

  AdHash<Hash> ah(bits, H);
  benchmark_adhash_or_muhash(ah);
}

}
}


#endif /* HEADER GUARD */
