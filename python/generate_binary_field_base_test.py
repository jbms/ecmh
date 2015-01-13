#!/usr/bin/env python2
import os

def write_test(out_name, modulus, field_name):

    m = modulus[0]
    modulus_with_commas = ','.join(map(str,modulus[:-1]))
    modulus_with_underscores = '_'.join(map(str,modulus[:-1]))

    with open(out_name + '.tmp', 'w') as out:
        out.write((
            '#include "test/binary_field/test_base_field.hpp"\n' +
            '#include "jbms/binary_field/{field_name}.hpp"\n' +
            '#include "jbms/binary_field/GF2m.hpp"\n' +
            '#include <boost/test/unit_test.hpp>\n\n' +
            'BOOST_AUTO_TEST_CASE(test_{modulus_with_underscores}) {{\n' +
            '  jbms::binary_field::GF2<{modulus_with_commas}> F;\n' +
            '  jbms::binary_field::GF2m F1({{{modulus}}});\n' +
            '  jbms::binary_field::test_base_field_properties(F, F1);\n' +
            '}}\n').format(modulus_with_underscores = modulus_with_underscores,
                           modulus_with_commas = modulus_with_commas,
                           field_name = field_name,
                           modulus = ', '.join(map(str,modulus))))
    os.rename(out_name + '.tmp', out_name)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary field test')
    parser.add_argument('--out_name', help = 'output path for C++ test source file')
    parser.add_argument('--field_name', help = 'field name')
    parser.add_argument('modulus', type = int, nargs ='+', help = 'Modulus coefficients')

    write_test(**parser.parse_args().__dict__)
