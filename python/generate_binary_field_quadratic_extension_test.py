#!/usr/bin/env python2
import os

def write_test(out_name, m):

    with open(out_name + '.tmp', 'w') as out:
        out.write((
            '#include "test/binary_field/test_quadratic_extension.hpp"\n' +
            '#include "test/binary_field/test_field_match.hpp"\n' +
            '#include "test/binary_field/test_generic_field.hpp"\n' +
            '#include "jbms/binary_field/GF2_{m}.hpp"\n' +
            '#include <boost/test/unit_test.hpp>\n\n' +
            'BOOST_AUTO_TEST_CASE(test_{m}) {{\n' +
            '  jbms::binary_field::GF2_{m} F;\n' +
            '  jbms::binary_field::test_quadratic_extension_properties(F);\n' +
            '}}\n').format(m = m))
    os.rename(out_name + '.tmp', out_name)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary field test')
    parser.add_argument('--out_name', help = 'output path for C++ test source file')
    parser.add_argument('m', type = int, help = 'Extension field degree')

    write_test(**parser.parse_args().__dict__)
