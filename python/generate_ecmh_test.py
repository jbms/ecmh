#!/usr/bin/env python2
import os

def write_test(out_name, curve, hash, blinded):

    with open(out_name + '.tmp', 'w') as out:
        lines = [
            '#include "test/multiset_hash/test_generic_multiset_hash.hpp"\n',
            '#include "jbms/binary_elliptic_curve/{curve}.hpp"\n',
            '#include "jbms/multiset_hash/ECMH.hpp"\n',
            '#include "jbms/hash/{hash_name}.hpp"\n',
            '#include <boost/test/unit_test.hpp>\n\n',
            'BOOST_AUTO_TEST_CASE(test_ecmh_{curve}) {{\n',
            '  jbms::multiset_hash::ECMH<jbms::binary_elliptic_curve::{curve}, jbms::hash::{hash_name}, {blinded}> ecmh;\n',
            '  jbms::multiset_hash::test_generic_multiset_hash(ecmh);\n',
            '}}\n']
        out.write(''.join(lines).format(curve = curve, hash_name = hash, blinded = 'true' if blinded else 'false'))
    os.rename(out_name + '.tmp', out_name)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate elliptic curve multiset hash test')
    parser.add_argument('--out_name', help = 'output path for C++ test source file', required = True)
    parser.add_argument('--curve', type = str, help = 'Curve type name', required = True)
    parser.add_argument('--hash', type = str, help = 'Hash name', required = True)
    parser.add_argument('--blinded', help = 'Use blinded implementation', action = 'store_true')

    write_test(**parser.parse_args().__dict__)
