#!/usr/bin/env python2

import os

def write_benchmark(out_name, field_name, field_header_basename, linear_transforms):

    with open(out_name + '.tmp', 'w') as out:
        if linear_transforms:
            extra_code = '  jbms::binary_field::benchmark_linear_transform<Field>();\n'
        else:
            extra_code = ''
        out.write(('#include "test/binary_field/benchmark.hpp"\n' +
                   '#include "jbms/binary_field/{field_header_basename}.hpp"\n' +
                   'int main(int argc, char **argv) {{\n' +
                   '  using Field = jbms::binary_field::{field_name};\n' +
                   '  jbms::binary_field::run_benchmarks<Field>();\n' +
                   extra_code +
                   '  return 0;\n' +
                   '}}\n').format(field_name = field_name, field_header_basename = field_header_basename))
    os.rename(out_name + '.tmp', out_name)
if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary field test')
    parser.add_argument('--out_name', help = 'output path for C++ benchmark source file', required = True)
    parser.add_argument('--linear_transforms', action = 'store_true', help = 'Benchmark linear transform code')
    parser.add_argument('--field_name', type = str, help = 'Binary field name without namespace', required = True)
    parser.add_argument('--field_header_basename', type = str, help = 'Binary field header basename without extension', required = True)

    write_benchmark(**parser.parse_args().__dict__)
