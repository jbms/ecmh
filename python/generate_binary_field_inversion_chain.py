#!/usr/bin/env python2

from __future__ import division
from __future__ import print_function

def generate(modulus,
             alternate_chain,
             benchmark_data,
             inverse_table_opt,
             multi_square_block_size,
             output):

    m = modulus[0]
    #print('[m = %d] Generating' % m)
    import os, math
    from generate_chains import find_best_chain, estimate_cost
    from parse_benchmark_data import parse_benchmark_data, estimate_square_and_multiply_cost, estimate_linear_transform_costs

    benchmark_data = parse_benchmark_data(benchmark_data)
    square_cost, multiply_cost = estimate_square_and_multiply_cost(benchmark_data)
    table_lookup_costs = estimate_linear_transform_costs(benchmark_data, multi_square_block_size)

    search_args_orig = dict(total = m - 1,
                            multiply_cost = multiply_cost,
                            square_cost = square_cost,
                            extra_table_cost = 0,
                            table_lookup_costs = table_lookup_costs)
    search_args = search_args_orig.copy()

    if inverse_table_opt == 'none':
        # Ensure that we don't use tables at all
        search_args['square_cost'] = 1
        search_args['table_lookup_costs'] = [1000000]
    elif inverse_table_opt == 'unoptimized':
        # Don't optimize for table use
        search_args['square_cost'] = 1
        search_args['table_lookup_costs'] = [1000000]
    elif inverse_table_opt == 'single':
        search_args['table_lookup_costs'] = table_lookup_costs[:1]

    solution = find_best_chain(**search_args)
    solution['multi_square_block_size'] = multi_square_block_size
    solution['actions'] = tuple(tuple(x) for x in solution['actions'])

    if inverse_table_opt == 'unoptimized':
        # Use multi-square tables whenever the squaring costs exceed the single-table lookup cost
        min_table_value = math.ceil(table_lookup_costs[0] / square_cost)
        solution['tables'] = list(set(x[0] for x in solution['actions'] if x[0] >= min_table_value))
        solution['tables'].sort()

        solution['actions'] = tuple((x[0], x[1], 1 if x[0] in solution['tables'] else -1) for x in solution['actions'])
    
    import json

    def print_stats(name, chain):
        tables = set(x[0] for x in chain if x[2] == 1)
        num_lookups = sum((1 for x in chain if x[2] == 1), 0)
        num_sq = sum((x[0] for x in chain if x[2] == -1), 0)
        print('%s: num_tables = %d, num_lookups = %d, num_sq = %d, len = %d, cost = %r, full cost = %r' %
              (name, len(tables), num_lookups, num_sq, len(chain), estimate_cost(chain, **search_args),
               estimate_cost(chain, **search_args_orig)))

    print('cur_chain: %r' % (solution['actions'],))

    if alternate_chain:
        with open(alternate_chain, 'r') as f:
            alt_data = json.load(f)
        # print_stats('cur chain', solution['actions'])
        # print_stats('alt chain', alt_data['actions'])

        cur_cost = estimate_cost(solution['actions'], **search_args)
        alt_cost = estimate_cost(alt_data['actions'], **search_args)
        if alt_cost < cur_cost + 0.1:
            print('%s: Using alternate chain %s since it has the same cost' % (output, alternate_chain))
            if alt_data['actions'] != solution['actions']:
                print('WARNING: same chain not found')
                print('alt: %r\ncur: %r' % (alt_data['actions'], solution['actions']))
            solution = alt_data

    with open(output + '.tmp', 'w') as out:
        json.dump(solution, out)
    os.rename(output + '.tmp', output)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary finite field inversion chain')
    parser.add_argument('modulus', type = int, nargs ='+', help = 'Modulus coefficients')

    parser.add_argument('--inverse_table_opt', type = str, help = 'Must be one of "none", "unoptimized", "single", or "multi".  If none, then multi-squaring tables are not used for computing the inverse.  If set to "unoptimized", then tables are used when single-table performance would predict an improvement, but the addition chain is not optimized for table use.  If set to "single", then the addition chain is optimized for single-table use.  If set to "multi", then the addition chain is optimized for multi-table use.', default = 'multi')
    parser.add_argument('--multi_square_block_size', type = int, help = 'Block size for multi-square tables', required = True)
    parser.add_argument('--benchmark_data', type = str, help = 'Benchmark result file', required = True)
    parser.add_argument('--alternate_chain', type = str, help = 'Alternate chain to use if cost is the same', required = False)
    parser.add_argument('--output', type = str, help = 'JSON output filename', required = True)

    args = parser.parse_args()

    generate(**args.__dict__)
