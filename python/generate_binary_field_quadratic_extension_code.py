#!/usr/bin/env python2
import os
import random

def write_code(h_name, modulus):

    m = modulus[0]
    modulus_with_commas = ','.join(map(str,modulus[:-1]))
    modulus_with_underscores = '_'.join(map(str,modulus[:-1]))

    with open(h_name + '.tmp', 'w') as out:
        header_guard = 'HEADER_GUARD_%016x' % random.randint(0,2**128)
        out.write((
            '#ifndef {guard}\n' +
            '#define {guard}\n' +
            '#include "jbms/binary_field/GF2_{modulus_with_underscores}.hpp"\n' +
            '#include "jbms/binary_field/QuadraticExtension.hpp"\n' +
            'namespace jbms {{\n' +
            'namespace binary_field {{\n' +
            'using GF2_{m2} = QuadraticExtension<GF2<{modulus_with_commas}>>;\n' +
            '}} // namespace jbms::binary_field\n' +
            '}} // namespace jbms\n' +
            '#endif // HEADER GUARD\n').format(
                modulus_with_underscores = modulus_with_underscores,
                modulus_with_commas = modulus_with_commas,
                guard = header_guard,
                m2 = 2 * m))
    os.rename(h_name + '.tmp', h_name)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary field quadratic extension code')
    parser.add_argument('--h_name', help = 'output path for C++ header file')
    parser.add_argument('modulus', type = int, nargs = '+', help = 'Base field degree')

    write_code(**parser.parse_args().__dict__)
