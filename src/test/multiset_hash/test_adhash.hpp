#ifndef HEADER_GUARD_900ccc4beef0f0bce1814e5816f66fd2
#define HEADER_GUARD_900ccc4beef0f0bce1814e5816f66fd2

#include "jbms/multiset_hash/AdHash.hpp"
#include "./test_generic_multiset_hash.hpp"

namespace jbms {
namespace multiset_hash {

template <class Hash>
void test_adhash(Hash const &H) {

  // Test some odd sizes as well to make sure things work when number of bits is not equal to the word size
  for (auto bits : {128, 256, 512, 513, 541}) {
    AdHash<Hash> ah(bits, H);
    test_generic_multiset_hash(ah);
  }
}

}
}

#endif /* HEADER GUARD */
