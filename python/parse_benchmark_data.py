from __future__ import division
from __future__ import print_function

import re

def parse_benchmark_data(filename):
    costs = {}
    with open(filename, 'r') as f:
        out = f.read()
        for match in re.finditer(r'^([a-zA-Z0-9_\-+]+): ([0-9\.]+)$', out, re.M):
            costs[match.group(1)] = float(match.group(2))
    return costs

def estimate_square_and_multiply_cost(benchmark_data, include_0 = False, repeat_count = 8):
    multi_square_and_multiply_costs = []
    num_squarings = []

    if include_0:
        multi_square_and_multiply_costs.append(benchmark_data['multiply'])
        num_squarings.append(0)

    n_squarings = 1
    while n_squarings <= 10:
        key = 'multi_square%d_and_multiply' % n_squarings
        if repeat_count is not None:
            key += '_%d' % repeat_count

        if key in benchmark_data:
            multi_square_and_multiply_costs.append(benchmark_data[key])
            num_squarings.append(n_squarings)
            n_squarings += 1
        else:
            break

    import numpy as np
    estimate, cov = np.polyfit(np.array(num_squarings), np.array(multi_square_and_multiply_costs), 1, cov = True)
    square_cost, multiply_cost = estimate
    benchmark_data['estimated_square'] = square_cost
    benchmark_data['estimated_square_var'] = cov[0][0]
    benchmark_data['estimated_multiply'] = multiply_cost
    benchmark_data['estimated_multiply_var'] = cov[1][1]
    return square_cost, multiply_cost

def estimate_linear_transform_costs(benchmark_data, block_size, suffix = '_hugepage'):
    multiply_cost = benchmark_data['estimated_multiply']

    num_tables = 1
    table_lookup_costs = []
    
    while True:
        key = 'linear_transform%d_and_multiply_%d%s_with_multi_square2' % (block_size, num_tables, suffix)
        if key in benchmark_data:
            table_lookup_costs.append(benchmark_data[key] - multiply_cost - benchmark_data['multi_square2_and_multiply'])
            num_tables += 1
        else:
            break

    # make sure the table_lookup_costs are non-decreasing, in case of any benchmark irregularities
    for i in xrange(1, len(table_lookup_costs)):
        table_lookup_costs[i] = max(table_lookup_costs[i], table_lookup_costs[i-1])

    return table_lookup_costs
