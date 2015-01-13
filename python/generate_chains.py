#!/usr/bin/env python2

from __future__ import division

import math, sys, time, operator

if not hasattr(__builtins__, 'profile'):
    def profile(x): return x

import heapq

def aStarSearch(problem, heuristic):
    queue = []
    closedSet = set()
    start_state = problem.getStartState()
    heapq.heappush(queue, (heuristic(start_state,problem), (start_state, None, None, 0)))
    num_expanded = 0
    path = None
    while queue:
        priority, s = heapq.heappop(queue)
        num_expanded += 1
        s_state, s_action, s_parent, s_cost = s
        if s_state in closedSet:
            continue
        closedSet.add(s_state)
        #print('Expanding: [%8.f : %8.f] %r' % (s_cost, priority, s_state))
        if problem.isGoalState(s_state):
            # Check heuristic consistency
            assert(priority == s_cost)

            path = []
            while s_parent != None:
                path.append(s_action)
                s_state, s_action, s_parent, s_cost = s_parent
            path.reverse()
            break

        for child, action, cost in problem.getSuccessors(s_state):
            # Check heuristic consistency
            # h(s_state) <= cost + heuristic(child)
            h_state = priority - s_cost
            h_child = heuristic(child,problem)
            assert(h_state <= cost + h_child #+ 0.01
            ) # 0.01 because there is some imprecision in float arithmetic


            heapq.heappush(queue, (h_child + cost + s_cost, (child,action,s,cost + s_cost)))
    return num_expanded, path

# State is a tuple of (values_reached, num_table_ops)
# values_reached is a tuple of pairs of the form (value, {0,1,-1})  the second part indicates: 0 -> not used as first term of action yet,  1 -> used as table, -1 -> used directly
# num_table_ops is the number of table actions performed
class AdditionChainSearchProblem(object):
    def __init__(self, skip_htable = False, max_num_tables = float('inf'), **kwArgs):
        self.__dict__.update(kwArgs)

        # Ensure cost arithmetic is exact
        def adjust_cost(x):
            assert(x >= 0)
            return round(float(x) * 8) / 8

        self.square_cost = adjust_cost(self.square_cost)
        self.multiply_cost = adjust_cost(self.multiply_cost)
        self.table_lookup_costs = [adjust_cost(x) for x in self.table_lookup_costs]
        self.extra_table_cost = adjust_cost(self.extra_table_cost)
        self.max_num_tables = max_num_tables

        # We will not use a multi-squaring table for values less than min_table_value

        # min_table_value * self.square_cost > self.table_lookup_cost
        # min_table_value > self.table_lookup_cost / self.square_cost
        if self.square_cost == 0:
            self.min_table_value = float('inf')
        else:
            self.min_table_value = math.ceil(self.table_lookup_costs[0] / self.square_cost)

        if not skip_htable:
            self.init_htable()


    def init_htable(self):

        # Generate heuristic cost-to-go table.  Given that we use
        # exactly n additional lookup tables, htable[n]
        # is a list of pairs (w, op_count) such that we can reach a value
        # >= total with op_count table lookup-based actions once we have reached w.  The pairs are sorted by w.

        # op_count decreases as w increases.
        htable = [] # indexed by num_tables

        # Base case: no tables, no cost
        htable.append([(self.total, 0)])

        num_tables = 1
        while num_tables <= self.max_num_tables:
            vals = [2] * num_tables

            cur_table = []

            def add_table_entry():
                cost = (sum(vals) - num_tables)
                factor = reduce(operator.mul, vals)
                w = math.ceil(self.total / factor)
                if len(cur_table) == 0 or cur_table[-1][0] != w:
                    #print('num_tables = %d, factor = %d, vals = %r, cost = %d, w = %d' % (num_tables, factor, vals, cost, w))
                    cur_table.append((w, cost))

            add_table_entry()

            while True:
                if cur_table[-1][0] == 1:
                    break
                for i in xrange(num_tables):
                    vals[i] += 1
                    add_table_entry()
            cur_table.reverse()
            htable.append(cur_table)
            if len(cur_table) == 1:
                # If the minimal values were sufficient to reach total, then adding more tables won't help
                break
            num_tables += 1
        self.htable = htable

        #Lookup table of where we need to start in htable for each value of max_value
        self.htable_start_i = []
        for num_tables, htable_cur in enumerate(htable):
            start_i = 0
            start_i_vals = []
            for max_value in xrange(0, self.total+1):
                while start_i + 1 < len(htable_cur) and htable_cur[start_i+1][0] <= max_value:
                    start_i += 1
                start_i_vals.append(start_i)
            self.htable_start_i.append(start_i_vals)

        self.heuristic_results = dict()
        self.heuristic_calls = 0

    def get_table_lookup_cost(self, num_tables):
        if num_tables == 0:
            return 0
        return self.table_lookup_costs[min(num_tables, len(self.table_lookup_costs)) - 1]

    def get_extra_table_cost(self, num_tables, num_table_ops):
        return self.extra_table_cost + (num_table_ops + 1) * (self.get_table_lookup_cost(num_tables + 1) - self.get_table_lookup_cost(num_tables))

    def getStartState(self):
        return (((1,-1),),0)
    def getSuccessors(self, state_full):
        state, num_table_ops = state_full
        num_tables = sum(1 for s in state if s[1] == 1)
        table_lookup_cost = self.get_table_lookup_cost(num_tables)
        next_table_lookup_cost = self.get_table_lookup_cost(num_tables+1)
        extra_table_cost = self.extra_table_cost + (num_table_ops + 1) * (next_table_lookup_cost - table_lookup_cost)

        new_tables_allowed = (num_tables < self.max_num_tables)
        for s in state:
            if s[1] == 1:
                if s[0] * self.square_cost < next_table_lookup_cost:
                    # Using an additional table would guarantee the resultant state to be suboptimal
                    new_tables_allowed = False
                break

        for i, xi in enumerate(state):
            for j, xj in enumerate(state[:i+1]):
                x = xi[0] + xj[0]
                if x > state[-1][0] and x <= self.total:
                    base_cost = self.multiply_cost
                    new_state = list(state)
                    new_state.append((x, -1 if x < self.min_table_value else 0))

                    new_states = dict()

                    for k, k_other in ((j,i), (i,j)):
                        def add_state(st, extra_table_ops, cost):
                            action = (st[k][0], st[k_other][0], st[k][1])

                            # Don't consider this if direct squaring with k_other would be cheaper, or we are considering a table lookup action and k_other is already fixed to be a table lookup
                            other_cost = base_cost + state[k_other][0] * self.square_cost
                            if state[k_other][1] == 1 and extra_table_ops:
                                other_cost = min(other_cost, base_cost + table_lookup_cost)

                            if other_cost is None or other_cost > cost or (other_cost == cost and state[k][1] != 0):
                                st = (tuple(st), num_table_ops + extra_table_ops)
                                val = new_states.get(st,None)
                                if val is None or val[1] > cost:
                                    new_states[st] = (action, cost)

                        xk = state[k]

                        if xk[1] != 1:
                            # Consider direct multi-squaring with xk
                            new_state[k] = (xk[0],-1)
                            add_state(new_state, 0, base_cost + xk[0] * self.square_cost)

                        if xk[1] != -1:
                            if xk[1] == 1 or new_tables_allowed:
                                # Consider table-based with xk
                                new_state[k] = (xk[0],1)
                                add_state(new_state, 1, base_cost + table_lookup_cost + extra_table_cost * (xk[1]==0))

                        new_state[k] = xk

                    for st, (action, cost) in new_states.iteritems():
                        yield (st, action, cost)

    def isGoalState(self, state_full):
        state, num_table_ops = state_full
        return state[-1][0] == self.total

    def getCostOfActions(self, actions):
        state = [[1,-1]]
        num_table_ops = 0
        cost = 0
        tables = []
        extra_cost = 0
        table_lookup_cost = 0
        for (xi, xj, op) in actions:
            x = xi + xj
            state.append([x, (-1 if x < self.min_table_value else 0)])
            cost += self.multiply_cost + (table_lookup_cost if op == 1 else self.square_cost * xi)
            i = next(i for i,vi in enumerate(state) if vi[0] == xi)
            if op == 1:
                num_table_ops += 1

            if op == 1 and state[i][1] != 1:
                tables.append(xi)
                # We need to account for change in table lookup cost
                new_table_lookup_cost = self.get_table_lookup_cost(len(tables))
                cost += num_table_ops * (new_table_lookup_cost - table_lookup_cost)
                table_lookup_cost = new_table_lookup_cost
                extra_cost += self.extra_table_cost
            state[i][1] = op
        return ((tuple(map(tuple,state)),num_table_ops), tuple(tables), cost, extra_cost)


def check_heuristic_admissibility_on_path(problem, path, heuristic):
  # Compute cost of complete path
  state, tables, cost, extra_cost = problem.getCostOfActions(path)
  true_cost = cost + extra_cost
  for i in xrange(len(path)-1):
      print('\nChecking up to %d' % i)
      cur_state, cur_tables, cost, extra_cost = problem.getCostOfActions(path[:i])
      cur_cost = cost + extra_cost
      hur_cost = heuristic(cur_state, problem)
      if cur_cost + hur_cost > true_cost:
          print('cur_cost = (actual) %.0f + (heuristic) %.0f = %.0f > true_cost = %.0f' % (cur_cost, hur_cost, cur_cost + hur_cost, true_cost))
          print('state = %r' % cur_state)
          print('path = %r' % path)
          print('tables = %r' % (tables,))
          raise

@profile
def addition_heuristic(state_full, problem):
    state, num_table_ops = state_full

    # The maximum value for which we have already computed a table
    max_table = reduce(max, (s[0] for s in state if s[1] == 1), 0)

    # The maximum value i for which the addition chain already computes x^{2^i - 1}
    # This is the last element in the chain since we ensure the chain is ordered.
    max_value = state[-1][0]

    # Number of tables already used by state
    base_num_tables = sum(1 for s in state if s[1] == 1)

    key = (max_table, max_value, base_num_tables, num_table_ops)

    problem.heuristic_calls += 1

    result = problem.heuristic_results.get(key, None)
    if result is not None:
        return result

    best_bound = float('inf')

    num_tables = 0
    num_tables_bound = min(len(problem.htable) + 1, problem.max_num_tables - base_num_tables)
    while num_tables <= num_tables_bound:
        htable_cur = problem.htable[num_tables]
        # start_i = 0
        # while start_i + 1 < len(htable_cur) and htable_cur[start_i+1][0] <= max_value:
        #     start_i += 1

        start_i = problem.htable_start_i[num_tables][max_value]

        table_lookup_cost = problem.get_table_lookup_cost(base_num_tables + num_tables)
        table_lookup_cost_old = problem.get_table_lookup_cost(base_num_tables)
        base_cost = (table_lookup_cost - table_lookup_cost_old) * num_table_ops + problem.extra_table_cost * num_tables
        table_op_cost = table_lookup_cost + problem.multiply_cost

        # w is increasing, cur_bound2 is decreasing
        for w, table_op_count in htable_cur[start_i:]:
            cur_bound2 = base_cost + table_op_cost * table_op_count

            if cur_bound2 >= best_bound:
                continue

            if w <= max_value:
                cur_bound1 = 0
            else:
                # The cost to increase our best_value by remainder, assuming we don't use any additional table lookups (only squarings)
                # This depends on:  max_value, target
                if max_table == 0 or max_table * problem.square_cost <= table_lookup_cost:
                    cur_bound1 = math.ceil(math.log(w / max_value, 2)) * problem.multiply_cost + (w - max_value) * problem.square_cost

                else:
                    remainder = w - max_value

                    # Cost using only the specified table and no other tables
                    min_table_uses = math.floor(remainder / max_table)
                    remainder -= min_table_uses * max_table

                    cur_bound1 = (min_table_uses * (problem.multiply_cost + table_lookup_cost)
                                  + (problem.multiply_cost + min(table_lookup_cost,
                                                                 remainder * problem.square_cost)) * (remainder > 0))
            cur_bound = cur_bound1 + cur_bound2
            best_bound = min(best_bound, cur_bound)

            # cur_bound1 will be non-decreasing in this loop, so check if we can already terminate
            if cur_bound1 + htable_cur[-1][1] * table_op_cost + base_cost >= best_bound:
                break
        if htable_cur[-1][0] <= max_value:
            # Minimal use of tables is already sufficient; adding more tables won't help
            break
        num_tables += 1

    problem.heuristic_results[key] = best_bound
    return best_bound

def find_best_chain(**kwArgs):
    problem = AdditionChainSearchProblem(**kwArgs)
    start_time = time.time()


    num_expanded, solution = aStarSearch(problem = problem, heuristic = addition_heuristic)
    end_time = time.time()
    print('   %d expanded, %d heuristic calls, %d unique, solved in %.4f' % (num_expanded, problem.heuristic_calls, len(problem.heuristic_results), end_time - start_time))
    assert(solution is not None)
    #check_heuristic_admissibility_on_path(problem, solution, lambda state, problem: addition_heuristic(state, problem, debug = True))
    state, tables, cost, extra_cost = problem.getCostOfActions(solution)
    stats = dict(elapsed_time = end_time - start_time,
                 states_expanded = num_expanded,
                 heuristic_calls = problem.heuristic_calls,
                 unique_heuristic_calls = len(problem.heuristic_results))
    print('  cost = %f' % (cost + extra_cost))
    return dict(state = state, tables = tables, actions = solution, cost = cost, extra_cost = extra_cost, stats = stats)

def estimate_cost(solution, **kwArgs):
    problem = AdditionChainSearchProblem(**kwArgs)
    state, tables, cost, extra_cost = problem.getCostOfActions(solution)

    # Extra square operation required at end
    return cost + problem.square_cost

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Compute optimal addition chain')
    parser.add_argument('--total', type = int, required = True)
    parser.add_argument('--multiply_cost', type = float, default = 1)
    parser.add_argument('--square_cost', type = float, default = 1)
    parser.add_argument('--table_lookup_costs', type = str, required = True)
    parser.add_argument('--extra_table_cost', type = float, default = 0)

    args = parser.parse_args()
    args.table_lookup_costs = map(float, args.table_lookup_costs.split())

    start_time = time.time()

    solution = find_best_chain(**args.__dict__)
    state_full = solution['state']
    actions = solution['actions']
    tables = solution['tables']
    cost = solution['cost']
    extra_cost = solution['extra_cost']

    end_time = time.time()
    state, num_table_ops = state_full
    print('Addition chain: %r' % ([x[0] for x in state]))
    print('Length: %r' % (len(state)-1))
    print('Tables(%d): %r' % (len(tables), tables))
    print('Cost: %r' % cost)
    print('Extra cost: %r' % extra_cost)
    print('Actions: %r' % (actions,))
    print('%.4f seconds elapsed' % (end_time - start_time))
