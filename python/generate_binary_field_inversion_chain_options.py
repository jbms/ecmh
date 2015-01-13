#!/usr/bin/env python2

from __future__ import division
from __future__ import print_function

def generate(modulus,
             benchmark_data,
             multi_square_block_size,
             output):

    m = modulus[0]
    #print('[m = %d] Generating' % m)
    import os, math
    from generate_chains import find_best_chain, estimate_cost
    from parse_benchmark_data import parse_benchmark_data, estimate_square_and_multiply_cost, estimate_linear_transform_costs

    benchmark_data = parse_benchmark_data(benchmark_data)
    square_cost, multiply_cost = estimate_square_and_multiply_cost(benchmark_data)

    base_search_args = dict(total = m - 1,
                            multiply_cost = multiply_cost,
                            square_cost = square_cost,
                            extra_table_cost = 0)
    table_free_args = base_search_args.copy()
    table_free_args.update(max_num_tables = 1, table_lookup_costs = [1000000])

    # Compute table-free solution
    print('Computing table-free solution')
    table_free_solution = find_best_chain(**table_free_args)

    print('   %r' % (table_free_solution,))
    table_free_solution['multi_square_block_size'] = 0
    solutions = []
    solutions.append(table_free_solution)

    for block_size in multi_square_block_size:
        table_lookup_costs = estimate_linear_transform_costs(benchmark_data, block_size)

        search_args = base_search_args.copy()
        search_args.update(table_lookup_costs = table_lookup_costs)
        print('Computing unrestricted solution for block size %d' % block_size)
        cur_solution = find_best_chain(**search_args)
        while True:
            if len(cur_solution['tables']) > 0:
                # Only add it if it uses at least one table
                # Otherwise it would just duplicate the table-free solution
                cur_solution['multi_square_block_size'] = block_size
                solutions.append(cur_solution)
                print('  %r' % (cur_solution,))
            else:
                break

            max_num_tables = len(cur_solution['tables']) - 1
            if max_num_tables < 1:
                break

            print('Computing for block size %d, max_num_tables = %d' % (block_size, max_num_tables))
            cur_solution = find_best_chain(max_num_tables = max_num_tables, **search_args)
    
    #print(repr(solutions))

    import json

    with open(output + '.tmp', 'w') as out:
        json.dump(solutions, out)
    os.rename(output + '.tmp', output)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary finite field inversion chain')
    parser.add_argument('modulus', type = int, nargs ='+', help = 'Modulus coefficients')
    parser.add_argument('--multi_square_block_size', type = int, nargs = '+', help = 'Block sizes for multi-square tables', required = True)
    parser.add_argument('--benchmark_data', type = str, help = 'Benchmark result file', required = True)
    parser.add_argument('--output', type = str, help = 'JSON output filename', required = True)
    args = parser.parse_args()
    generate(**args.__dict__)
