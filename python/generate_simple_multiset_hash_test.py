#!/usr/bin/env python2
import os

def write_test(out_name, hash, name):

    with open(out_name + '.tmp', 'w') as out:
        lines = [
            '#include "test/multiset_hash/test_{name}.hpp"\n',
            '#include "jbms/hash/{hash_name}.hpp"\n',
            '#include <boost/test/unit_test.hpp>\n\n',
            'BOOST_AUTO_TEST_CASE(test_{name}) {{\n',
            '  jbms::multiset_hash::test_{name}(jbms::hash::{hash_name}());\n'
            '}}\n']
        out.write(''.join(lines).format(hash_name = hash, name = name))
    os.rename(out_name + '.tmp', out_name)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate MuHash multiset hash test')
    parser.add_argument('--out_name', help = 'output path for C++ test source file', required = True)
    parser.add_argument('--name', type = str, help = 'Name (adhash or muhash)', required = True)
    parser.add_argument('--hash', type = str, help = 'Hash name', required = True)

    write_test(**parser.parse_args().__dict__)
