#!/usr/bin/env python2

import os

def write_benchmark(out_name, curve_name):

    with open(out_name + '.tmp', 'w') as out:
        out.write(('#include "test/binary_elliptic_curve/benchmark.hpp"\n' +
                   '#include "jbms/binary_elliptic_curve/{curve_name}.hpp"\n' +
                   'int main(int argc, char **argv) {{\n' +
                   '  using Curve = jbms::binary_elliptic_curve::{curve_name};\n' +
                   '  jbms::binary_elliptic_curve::benchmark_elliptic_curve(Curve());\n' +
                   '  return 0;\n' +
                   '}}\n').format(curve_name = curve_name))
    os.rename(out_name + '.tmp', out_name)
if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary elliptic curve benchmark')
    parser.add_argument('--out_name', help = 'output path for C++ benchmark source file', required = True)
    parser.add_argument('--curve_name', type = str, help = 'Binary elliptic curve name without namespace', required = True)

    write_benchmark(**parser.parse_args().__dict__)
