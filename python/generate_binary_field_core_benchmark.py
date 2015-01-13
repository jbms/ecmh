#!/usr/bin/env python2

## Generates code to benchmark multiply, square, and table lookup operations for a binary field
## This does not depend on any generated binary field code, as the result is used to select the addition chain for inversion.

import os

def write_benchmark(out_name, modulus):

    modulus_with_commas = ','.join(map(str,modulus[:-1]))
    modulus_with_underscores = '_'.join(map(str,modulus[:-1]))

    with open(out_name + '.tmp', 'w') as out:
        out.write(('#include "test/binary_field/benchmark.hpp"\n' +
                   '#include "jbms/binary_field/detail/polynomial_base.hpp"\n' +
                   '#include "jbms/binary_field/detail/polynomial_reduce_{modulus_with_underscores}.hpp"\n' +
                   '#include "jbms/binary_field/detail/polynomial_multiply.hpp"\n' +
                   'int main(int argc, char **argv) {{\n' +
                   '  using Field = jbms::binary_field::GF2<{modulus_with_commas}>;\n' +
                   '  jbms::binary_field::run_core_benchmarks<Field>();\n' +
                   '  jbms::binary_field::benchmark_linear_transform<Field>();\n' +
                   '  return 0;\n' +
                   '}}\n').format(modulus_with_underscores = modulus_with_underscores,
                                  modulus_with_commas = modulus_with_commas))
    os.rename(out_name + '.tmp', out_name)
if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary field test')
    parser.add_argument('--out_name', help = 'output path for C++ benchmark source file', required = True)
    parser.add_argument('modulus', type = int, nargs ='+', help = 'Modulus coefficients')

    write_benchmark(**parser.parse_args().__dict__)
