#!/usr/bin/env python2

from __future__ import division
from __future__ import print_function

def generate(modulus,
             multi_square_block_size,
             output):

    m = modulus[0]
    #print('[m = %d] Generating' % m)
    import os, math
    from generate_chains import find_best_chain

    base_search_args = dict(total = m - 1,
                            multiply_cost = 1,
                            square_cost = 1,
                            extra_table_cost = 0)
    table_free_args = base_search_args.copy()
    table_free_args.update(max_num_tables = 0, table_lookup_costs = [1000000])

    # Compute table-free solution
    print('Computing table-free solution')
    table_free_solution = find_best_chain(**table_free_args)

    print('   %r' % (table_free_solution,))
    table_free_solution['multi_square_block_size'] = 0
    solutions = []
    solutions.append(table_free_solution)
    all_tables = list(set(x[1] for x in table_free_solution['actions']))
    all_tables.sort()

    for block_size in multi_square_block_size:
        for num_tables in xrange(1,len(all_tables)):
            new_solution = table_free_solution.copy()
            tables = all_tables[-num_tables:]
            new_solution['tables'] = tables
            actions = table_free_solution['actions']
            actions = tuple((x[0], x[1], 1 if x[0] in tables else -1) for x in actions)
            new_solution['actions'] = actions
            new_solution['multi_square_block_size'] = block_size
            solutions.append(new_solution)

    import json

    with open(output + '.tmp', 'w') as out:
        json.dump(solutions, out)
    os.rename(output + '.tmp', output)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary finite field inversion chain')
    parser.add_argument('modulus', type = int, nargs ='+', help = 'Modulus coefficients')
    parser.add_argument('--multi_square_block_size', type = int, nargs = '+', help = 'Block sizes for multi-square tables', required = True)
    parser.add_argument('--output', type = str, help = 'JSON output filename', required = True)
    args = parser.parse_args()
    generate(**args.__dict__)
