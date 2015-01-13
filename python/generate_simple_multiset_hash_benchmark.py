#!/usr/bin/env python2
import os

def write_benchmark(out_name, hash, name):
    with open(out_name + '.tmp', 'w') as out:
        lines = [
            '#include "test/multiset_hash/benchmark_{name}.hpp"\n',
            '#include "jbms/hash/{hash_name}.hpp"\n',
            'int main(int argc, char **argv) {{\n',
            '  if (argc != 2) return 1;\n',
            '  int num_bits = atoi(argv[1]);\n',
            '  using Hash = jbms::hash::{hash_name};\n',
            '  jbms::multiset_hash::benchmark_{name}(num_bits, Hash());\n',
            '}}\n']
        out.write(''.join(lines).format(hash_name = hash, name = name))
    os.rename(out_name + '.tmp', out_name)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate MuHash/AdHash benchmark')
    parser.add_argument('--out_name', help = 'output path for C++ benchmark source file', required = True)
    parser.add_argument('--name', type = str, help = 'Name (adhash or muhash)', required = True)
    parser.add_argument('--hash', type = str, help = 'Hash name', required = True)

    write_benchmark(**parser.parse_args().__dict__)
