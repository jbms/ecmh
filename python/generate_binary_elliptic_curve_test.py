#!/usr/bin/env python2
import os

def write_test(out_name, curve, against_openssl):

    with open(out_name + '.tmp', 'w') as out:
        lines = [
            '#include "test/binary_elliptic_curve/test_generic_curve.hpp"\n',
            '#include "jbms/binary_elliptic_curve/{curve}.hpp"\n',
            '#include <boost/test/unit_test.hpp>\n\n',
            'BOOST_AUTO_TEST_CASE(test_{curve}) {{\n',
            '  jbms::binary_elliptic_curve::{curve} curve;\n',
            '  jbms::binary_elliptic_curve::test_generic_curve_properties(curve);\n',
            '}}\n']
        if against_openssl:
            lines += [
                'BOOST_AUTO_TEST_CASE(test_{curve}_openssl) {{\n',
                '  jbms::binary_elliptic_curve::{curve} curve;\n',
                '  jbms::binary_elliptic_curve::test_curve_against_openssl(curve, "{curve}");\n',
                '}}\n']
        out.write(''.join(lines).format(curve = curve))
    os.rename(out_name + '.tmp', out_name)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary elliptic curve test')
    parser.add_argument('--out_name', help = 'output path for C++ test source file', required = True)
    parser.add_argument('--curve', type = str, help = 'Curve type name', required = True)
    parser.add_argument('--against_openssl', action = 'store_true', help = 'Test against OpenSSL implementation')

    write_test(**parser.parse_args().__dict__)
