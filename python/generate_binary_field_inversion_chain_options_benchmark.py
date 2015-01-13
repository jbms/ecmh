#!/usr/bin/env python2

import os

def solution_name(solution, is_ns_name = False):
    ns_name = 'b{block_size}_a{action_str}_t{table_str}'.format(
        block_size = solution['multi_square_block_size'],
        action_str = '_'.join('%d%s%d' % (x[0],'p' if is_ns_name else '+', x[1]) for x in solution['actions']),
        table_str = '_'.join('%d' % x for x in solution['tables']))
    return ns_name

def write_benchmark(out_name, chain_data, modulus, prefix = False):

    modulus_with_commas = ','.join(map(str,modulus[:-1]))
    modulus_with_underscores = '_'.join(map(str,modulus[:-1]))
    m = modulus[0]
    import json
    solutions = []

    def add_solutions(sols):
        if isinstance(sols, dict):
            sols = [sols]
        solutions.extend(sols)

    for chain_name in chain_data:
        with open(chain_name, 'r') as f:
            add_solutions(json.load(f))

    with open(out_name + '.tmp', 'w') as out:
        out.write(('#include "test/binary_field/benchmark.hpp"\n' +
                   '#include "jbms/binary_field/detail/polynomial_base.hpp"\n' +
                   '#include "jbms/binary_field/detail/polynomial_reduce_{modulus_with_underscores}.hpp"\n' +
                   '#include "jbms/binary_field/detail/polynomial_multiply.hpp"\n' +
                   '#include "jbms/binary_field/detail/GF2.hpp"\n' +
                   '#include "jbms/binary_field/compute_linear_operation_table.hpp"\n' +
                   '#include "jbms/binary_field/detail/apply_linear_table_transform.hpp"\n' +
                   '#include "jbms/hugepages.hpp"\n' +
                   'namespace inv_benchmarks {{\n' +
                   'using Field = jbms::binary_field::GF2<{modulus_with_commas}>;\n' +
                   'constexpr static size_t Degree = Field::degree();\n' +
                   'using FE = Field::Element;\n' +
                   'template <size_t BlockBits>\n' +
                   'using linear_table_t = FE[(Degree+BlockBits-1)/BlockBits][1<<BlockBits];\n' +
                   '')
                  .format(modulus_with_underscores = modulus_with_underscores,
                          modulus_with_commas = modulus_with_commas))
        out.write('template <size_t BlockBits>\n' +
                  'void generate_multi_square_table(linear_table_t<BlockBits> table, size_t power) {\n' +
                  '  jbms::binary_field::compute_linear_operation_table<BlockBits>(table, [&](FE x) {\n' +
                  '    Field F;\n' +
                  '    for (size_t i = 0; i < power; ++i) square(F, x, x);\n' +
                  '    return x;\n' +
                  '  });\n' +
                  '}\n')

        ns_names = []
        ns_names_set = set()
        for solution in solutions:
            block_size = solution['multi_square_block_size']
            tables = solution['tables']
            actions = solution['actions']
            ns_name = solution_name(solution, is_ns_name = True)
            benchmark_name = solution_name(solution, is_ns_name = False)
            if ns_name in ns_names_set:
                continue
            ns_names_set.add(ns_name)
            ns_names.append(ns_name)

            out.write('namespace %s {\n' % ns_name)
            if len(tables) > 0:
                out.write('  constexpr static size_t BlockBits = {block_size};\n'.format(block_size = block_size))
                out.write('  using Table = linear_table_t<BlockBits>;\n')
            for t in tables:
                out.write('Table *table_{t} = nullptr;\n'.format(t = t))

            def write_invert(prefix_len = None):
                if prefix_len is None:
                    prefix_len = len(actions)
                name = 'invert'
                if prefix_len < len(actions):
                    name += ('%d' % prefix_len)

                out.write('__attribute__((noinline)) void %s_impl(FE &result, FE const &a1) {\n' % name)
                out.write('  Field F;\n')
                for action_i, action in enumerate(actions):
                    x = action[0] + action[1]
                    if action_i < prefix_len:
                        out.write('  FE a{x};\n'.format(x = x))
                        if action[0] in tables:
                            out.write('  jbms::binary_field::apply_linear_table_transform<BlockBits>(a{x}, a{x1}, *table_{x0}); '
                                      .format(x = x, x0 = action[0], x1 = action[1]))
                        else:
                            out.write('  jbms::binary_field::direct_multi_square<{x0}>(F, a{x}, a{x1}); '
                                      .format(x = x, x0 = action[0], x1 = action[1]))
                        out.write('multiply(F, a{x}, a{x}, a{x0});\n'.format(x = x, x0 = action[0]))
                    if action_i + 1 == prefix_len:
                        out.write('  square(F, result, a{x});\n'.format(x = x))

                out.write('}\n')

                out.write(('template <bool C>\n') +
                          '__attribute__((noinline)) void test_%s(FE &result, FE const &a1) {\n' % name)
                out.write('  __asm__ volatile("");\n')
                out.write('  if (!C) return;\n')
                out.write('  %s_impl(result, a1);\n' % name)
                out.write('}\n')

            write_invert()
            if prefix:
                for prefix_len in xrange(1,len(actions)):
                    write_invert(prefix_len)

            out.write('void run_benchmark() {\n')

            if len(tables) > 0:
                # true -> verify allocation
                out.write('  auto tables = jbms::allocate_hugepage_array<Table>({num_tables}, true);\n'.format(num_tables = len(tables)))

            for t_i, t in enumerate(tables):
                out.write('  table_{t} = &tables[{t_i}];\n'.format(t = t, t_i = t_i))
                out.write('  generate_multi_square_table<BlockBits>(*table_{t}, {t});\n'.format(t = t))

            out.write('  constexpr size_t num_examples = %d;\n' % max(256, 2**(block_size)))
            out.write('  auto examples = jbms::binary_field::generate_table_examples<Field>(num_examples);\n')
            out.write('  FE result;\n')

            out.write(r"""
      auto do_for_examples = [&](auto &&callback) {
        if (num_examples == 256) {
          auto loop_over_examples = [&](auto &&inner_callback) {
            for (auto &&e : examples)
              inner_callback(e);
          };
          callback(loop_over_examples);
        } else {
          size_t example_i = 0;
          auto loop_over_examples = [&](auto &&inner_callback) {
            example_i %= examples.size();
            for (size_t example_end = example_i + 256; example_i < example_end; ++example_i) {
              inner_callback(examples[example_i]);
            }
          };
          callback(loop_over_examples);
        }
      };
""")

            out.write('  do_for_examples([&](auto &&loop_over_examples) {{ jbms::benchmark_function("{benchmark_name}", [&](auto C) {{ loop_over_examples([&](auto &&e) {{ test_invert<C()>(result, e); }}); }}, 256); }});\n'.format(benchmark_name = benchmark_name))

            # out.write('  jbms::benchmark_function("{benchmark_name}", [&](auto C) {{ for (auto &&e : examples) test_invert<C()>(result, e); }}, examples.size());\n'.format(benchmark_name = benchmark_name))


            # out.write('  for (int k = 0; k < 5; ++k) jbms::benchmark_function_foreach("{benchmark_name}", [&](auto C, auto &&e) {{ test_invert<C()>(result, e); }}, examples);\n'.format(benchmark_name = benchmark_name))

            if prefix:
                for prefix_len in xrange(1, len(actions)):
                    out.write('  do_for_examples([&](auto &&loop_over_examples) {{ jbms::benchmark_function("{benchmark_name}_prefix{prefix_len}", [&](auto C) {{ loop_over_examples([&](auto &&e) {{ test_invert{prefix_len}<C()>(result, e); }}); }}, 256); }});\n'.format(benchmark_name = benchmark_name, prefix_len = prefix_len))

            for t_i, t in enumerate(tables):
                out.write('  table_{t} = nullptr;\n'.format(t = t))
            out.write('} // end run_benchmark\n')

            out.write('} // namespace %s\n' % ns_name)

        out.write('} // namespace inv_benchmarks\n')

        out.write('int main(int argc, char **argv) {\n')
        out.write('  using namespace inv_benchmarks;\n')
        for ns_name in ns_names:
            out.write('  {ns_name}::run_benchmark();\n'.format(ns_name = ns_name))
        out.write('  return 0;\n' +
                  '}\n')

    os.rename(out_name + '.tmp', out_name)
if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate binary field inversion chain options benchmark')
    parser.add_argument('--out_name', help = 'output path for C++ benchmark source file', required = True)
    parser.add_argument('--chain_data', type = str, nargs = '+', help = 'chain data JSON file', required = True)
    parser.add_argument('modulus', type = int, nargs ='+', help = 'Modulus coefficients')
    write_benchmark(**parser.parse_args().__dict__)
