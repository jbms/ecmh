#!/usr/bin/env python2

from __future__ import division
from __future__ import print_function

import math
import subprocess
import sys
import random
import os
import time
import itertools

def generate(modulus,
             generator_program,
             h_name,
             cpp_name,
             field_name,
             copy_to_huge_pages,
             inversion_chain,
             half_trace_block_size):
    m = modulus[0]

    import json
    with open(inversion_chain, 'r') as f:
        d = json.load(f)


    inv_actions = [(x[0],x[1]) for x in d['actions']]

    subprocess.check_call([generator_program,
                           "--h_name", h_name + '.tmp',
                           "--cpp_name", cpp_name + '.tmp',
                           "--field_name", field_name,
                           "--copy_to_huge_pages", str(int(copy_to_huge_pages)),
                           "--multi_square_block_size", str(d['multi_square_block_size']),
                           "--half_trace_block_size", str(half_trace_block_size)] +
                          sum((['--multi_square_table', str(x)] for x in d['tables']), []) +
                          sum((['--modulus', str(x)] for x in modulus), []) +
                          sum((['--addition_chain', str(x)] for x in itertools.chain.from_iterable(inv_actions)), []))

    os.rename(h_name + '.tmp', h_name)
    os.rename(cpp_name + '.tmp', cpp_name)
    #print('[m = %d] Done' % m)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary finite field code')
    parser.add_argument('--h_name', help = 'output path for header file', required = True)
    parser.add_argument('--cpp_name', help = 'output path for C++ source file', required = True)
    parser.add_argument('--generator_program', help = 'generator program executable', required = True)
    parser.add_argument('modulus', type = int, nargs ='+', help = 'Modulus coefficients')
    parser.add_argument('--half_trace_block_size', type = int, required = True)
    parser.add_argument('--inversion_chain', type = str, help = 'Inversion addition chain JSON file', required = True)
    parser.add_argument('--field_name', type = str, required = True)
    parser.add_argument('--copy_to_huge_pages', type = bool, help = 'Ensure look-up tables are backed by huge pages.', required = True)

    args = parser.parse_args()

    generate(**args.__dict__)
