#!/usr/bin/env python2
import os

def write_benchmark(out_name, curve, hash):
    with open(out_name + '.tmp', 'w') as out:
        lines = [
            '#include "test/multiset_hash/benchmark_generic_multiset_hash.hpp"\n',
            '#include "test/multiset_hash/benchmark_binary_field_assign_hash.hpp"\n',
            '#include "jbms/binary_elliptic_curve/{curve}.hpp"\n',
            '#include "jbms/multiset_hash/ECMH.hpp"\n',
            '#include "jbms/hash/{hash_name}.hpp"\n',
            'int main(int argc, char **argv) {{\n',
            '  using Curve = jbms::binary_elliptic_curve::{curve};\n',
            '  using Hash = jbms::hash::{hash_name};\n',
            '  jbms::multiset_hash::ECMH<Curve, Hash> ecmh;\n',
            '  jbms::multiset_hash::benchmark_generic_multiset_hash(ecmh);\n',
            '  jbms::multiset_hash::benchmark_binary_field_assign_hash<Curve::Field,Hash>();\n',
            '}}\n']
        out.write(''.join(lines).format(curve = curve, hash_name = hash))
    os.rename(out_name + '.tmp', out_name)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate elliptic curve multiset hash benchmark')
    parser.add_argument('--out_name', help = 'output path for C++ benchmark source file', required = True)
    parser.add_argument('--curve', type = str, help = 'Curve type name', required = True)
    parser.add_argument('--hash', type = str, help = 'Hash name', required = True)

    write_benchmark(**parser.parse_args().__dict__)
