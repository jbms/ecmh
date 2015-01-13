#ifndef HEADER_GUARD_67a22dc87b7f39b632dff69bae99e5c4
#define HEADER_GUARD_67a22dc87b7f39b632dff69bae99e5c4

// http://www.ecrypt.eu.org/ebats/cpucycles.html

#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <stdio.h>
#include "jbms/cycle_count.hpp"
#include <vector>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <boost/range/size.hpp>

//#define JBMS_BENCHMARK_DEBUG_OUTPUT

namespace jbms {

template <class T>
inline double getSeconds(T const &d) {
  return std::chrono::duration<double>(d).count();
}

using time_point = std::chrono::time_point<std::chrono::system_clock>;

inline std::chrono::time_point<std::chrono::system_clock> now() { return std::chrono::system_clock::now(); }

struct Timer {
  time_point start;
  Timer() { start = now(); }
  double elapsed() const { return getSeconds(now() - start); }
};

namespace benchmark_detail {

struct Result {
  uint64_t median, lower, upper;
  auto range() const { return upper - lower; }
};

// Note: this reorders elements of values
// However, this should not matter in practice
Result calc_median_with_confidence(std::vector<uint64_t> &values, size_t multiplier = 1) {

  auto n = values.size();
  // 99% confidence interval
  auto pos_off = std::sqrt(double(n) / (multiplier * 2)) * 2.57 * multiplier;
  auto get_at_pos = [&](size_t pos) {
    pos = std::max((size_t)0, std::min(values.size() - 1, pos));
    std::nth_element(values.begin(), values.begin() + pos, values.end());
    return values[pos];
  };
  Result result;
  size_t mid_pos = n / 2;
  size_t lower_pos = (size_t)std::floor(mid_pos - pos_off);
  size_t upper_pos = (size_t)std::ceil(mid_pos + pos_off);
  result.median = get_at_pos(mid_pos);
  result.lower = get_at_pos(lower_pos);
  result.upper = get_at_pos(upper_pos);
  // std::cerr << "    pos_range: " << lower_pos << " -- " << upper_pos << std::endl;
  return result;
}

struct BenchmarkHelper {

  size_t warmup_rounds = 2048;
  size_t initial_rounds = 1024;
  double max_time_bound = 1; // seconds
  double max_overhead_time_bound = 0.5; // seconds

  // All of these are initialized by initialize
  size_t rounds = 0, overhead_rounds = 0;
  size_t measurements_per_round = 0;
  size_t counts_per_measurement = 0;
  std::vector<uint64_t> timings, timings_overhead;
  double time_per_round = 0, time_per_overhead_round = 0;

  Result result, result_overhead;

  template <class C_t, class FillResults>
  Result get_results(C_t C, FillResults &&fill_results, std::vector<uint64_t> &results, size_t already_filled) const {
    size_t n = results.size();
    if (n % measurements_per_round)
      throw std::logic_error("n must be multiple of measurements_per_round");
    if (already_filled != n) {
      size_t warmup_measurements = warmup_rounds * measurements_per_round;
      for (size_t measurement_count = 0; measurement_count < warmup_measurements;) {
        auto inner_warmup_n = std::min(already_filled + warmup_measurements - measurement_count, n);
        measurement_count += (inner_warmup_n - already_filled);
        fill_results(C, results, already_filled, inner_warmup_n);
      }
      fill_results(C, results, already_filled, n);
    }
    return calc_median_with_confidence(results, measurements_per_round);
  }

  template <class FillResults>
  void initialize(size_t measurements_per_round, size_t counts_per_measurement,
                  FillResults &&fill_results) {
    this->measurements_per_round = measurements_per_round;
    this->counts_per_measurement = counts_per_measurement;

    size_t num_measurements = measurements_per_round * initial_rounds;
    timings.resize(num_measurements);
    timings_overhead.resize(num_measurements);

    {
      Timer t;
      result = get_results(std::true_type{}, fill_results, timings, 0);
      time_per_round = t.elapsed() / (initial_rounds + warmup_rounds);
    }
    {
      Timer t;
      result_overhead = get_results(std::false_type{}, fill_results, timings_overhead, 0);
      time_per_overhead_round = t.elapsed() / (initial_rounds + warmup_rounds);
    }
  }

  template <class FillResults>
  void finalize(FillResults &&fill_results) {

    // First get overhead range down
    {
      auto cur_range = result_overhead.range();
      auto desired_range = 0.1 * counts_per_measurement;

#ifdef JBMS_BENCHMARK_DEBUG_OUTPUT
      std::cerr << "    counts_per_measurement = " << counts_per_measurement << ", initial overhead results: "
                << "median = " << double(result_overhead.median) / (counts_per_measurement) << ", "
                << "overhead range = " << double(result_overhead.range()) / counts_per_measurement << ", "
                << "desired = " << double(desired_range) / counts_per_measurement << std::endl;
#endif // JBMS_BENCHMARK_DEBUG_OUTPUT

      auto factor = cur_range / desired_range;

      auto desired_rounds = (size_t)(initial_rounds * factor * factor);
      overhead_rounds = std::max(initial_rounds, std::min((size_t)(max_overhead_time_bound / time_per_overhead_round), desired_rounds));


#ifdef JBMS_BENCHMARK_DEBUG_OUTPUT
      std::cerr << "    Desired overhead rounds: " << desired_rounds << " [expected time: " << time_per_overhead_round *desired_rounds << "]"
                << std::endl;
      std::cerr << "    Requisite overhead rounds: " << overhead_rounds << " [expected time: " << time_per_overhead_round *overhead_rounds << "]" << std::endl;
#endif // JBMS_BENCHMARK_DEBUG_OUTPUT

      size_t num_measurements = overhead_rounds * measurements_per_round;
      timings_overhead.resize(num_measurements);
      result_overhead = get_results(std::false_type{}, fill_results, timings_overhead, initial_rounds * measurements_per_round);
    }

    {
      auto cur_range = result.range();
      auto desired_range =
          std::max(0.1 * counts_per_measurement, double(int64_t(result.median - result_overhead.median)) * 0.001);

#ifdef JBMS_BENCHMARK_DEBUG_OUTPUT
      std::cerr << "    counts_per_measurement = " << counts_per_measurement << ", initial results: "
                << "median = " << double(result.median) / (counts_per_measurement) << " - "
                << double(result_overhead.median) / (counts_per_measurement) << " = "
                << double(int64_t(result.median - result_overhead.median)) / counts_per_measurement << ", "
                << "range = " << double(result.range()) / (counts_per_measurement) << ", "
                << "overhead range = " << double(result_overhead.range()) / counts_per_measurement << ", "
                << "desired = " << double(desired_range) / counts_per_measurement << std::endl;
#endif // JBMS_BENCHMARK_DEBUG_OUTPUT

      auto factor = cur_range / desired_range;

      auto desired_rounds = (size_t)(initial_rounds * factor * factor);
      rounds = std::max(initial_rounds, std::min((size_t)(max_time_bound / time_per_round), desired_rounds));

#ifdef JBMS_BENCHMARK_DEBUG_OUTPUT
      std::cerr << "    Desired rounds: " << desired_rounds << " [expected time: " << time_per_round *desired_rounds << "]"
                << std::endl;
      std::cerr << "    Requisite rounds: " << rounds << " [expected time: " << time_per_round *rounds << "]" << std::endl;
#endif // JBMS_BENCHMARK_DEBUG_OUTPUT

      size_t num_measurements = rounds * measurements_per_round;
      timings.resize(num_measurements);
      result = get_results(std::true_type{}, fill_results, timings, initial_rounds * measurements_per_round);

#ifdef JBMS_BENCHMARK_DEBUG_OUTPUT
      std::cerr << "     final results: "
                << "median = " << double(result.median) / (counts_per_measurement) << " - "
                << double(result_overhead.median) / (counts_per_measurement) << " = "
                << double(int64_t(result.median - result_overhead.median)) / counts_per_measurement << ", "
                << "range = " << double(result.range()) / (counts_per_measurement) << std::endl;
#endif // JBMS_BENCHMARK_DEBUG_OUTPUT

#ifdef JBMS_BENCHMARK_DEBUG_OUTPUT
      auto min_value = std::numeric_limits<uint64_t>::max();
      uint64_t max_value = 0;
      for (auto &&x : timings) {
        min_value = std::min(min_value, x);
        max_value = std::max(max_value, x);
      }
      std::cout << "  min_value = " << double(std::max(int64_t(0), int64_t(min_value - result_overhead.median))) /
                                           counts_per_measurement << "\n";
      std::cout << "  max_value = " << double(std::max(int64_t(0), int64_t(max_value - result_overhead.median))) /
                                           counts_per_measurement << "\n";
#endif // JBMS_BENCHMARK_DEBUG_OUTPUT
    }
  }
  void print_result(std::string name) const {

    std::cout << name << ": " << std::fixed << std::setprecision(0)
              << double(std::max(int64_t(0), int64_t(result.median - result_overhead.median))) / counts_per_measurement
              << std::endl;
  }

  bool overhead_is_okay() const {
    return result_overhead.median * 10 <= ((int64_t(result.median - result_overhead.median)));
  }
};

}

template <class Func>
void benchmark_function(std::string const &name, Func &&f, size_t inner_count = 1) {
  using namespace benchmark_detail;

  int count = 1;

  auto fill_results = [&](auto C, auto &results, size_t i, size_t n) {
    for (; i < n; ++i) {
      auto start = cycle_count_begin();
      for (int j = 0; j < count; ++j)
        f(C);
      auto end = cycle_count_end();
      results[i] = end - start;
    }
  };

  try {

    BenchmarkHelper helper;
    while (true) {

      helper.initialize(1, count * inner_count, fill_results);

      if (count >= 1024 || count * inner_count >= 2048 || helper.overhead_is_okay())
        break;

      count *= 2;
    }

    helper.finalize(fill_results);
    helper.print_result(name);
  }
  catch (...) {
    std::cout << name << ": (exception)" << std::endl;
  }
}

template <class Func>
void benchmark_function_fine(std::string const &name, Func &&f, size_t inner_count = 1) {
  using namespace benchmark_detail;

  auto fill_results = [&](auto C, auto &results, size_t i, size_t n) {
    // n must be a multiple of inner_count

    for (; i < n; i += inner_count) {
      uint64_t start = 0;
      f(C, [&]() { start = cycle_count_begin(); }, [&](size_t j) {
        auto end = cycle_count_end();
        results[i + j] = end - start;
      });
    }
  };

  try {
    BenchmarkHelper helper;
    helper.initialize(inner_count, 1, fill_results);
    helper.finalize(fill_results);
    helper.print_result(name);
  }
  catch (...) {
    std::cout << name << ": (exception)" << std::endl;
  }
}

template <class Func, class Range>
void benchmark_function_foreach_simple(std::string const &name, Func &&f, Range const &range) {
  benchmark_function(name, [&](auto C) {
      for (auto &&x : range)
        f(C, x);
    }, boost::size(range));
}

template <class Func, class Range1, class Range2>
void benchmark_function_foreach_simple(std::string const &name, Func &&f, Range1 const &range1, Range2 const &range2) {
  benchmark_function(name, [&](auto C) {
      for (auto &&x : range1)
        for (auto &&y : range2)
          f(C, x, y);
    }, boost::size(range1) * boost::size(range2));
}


template <class Func, class Range>
void benchmark_function_foreach(std::string const &name, Func &&f, Range const &range) {
  using namespace benchmark_detail;
  size_t size = boost::size(range);
  size_t count = 1;

  auto fill_results = [&](auto C, auto &results, size_t i, size_t n) {
    // n must be a multiple of inner_count
    size_t outer_count = size / count;

    for (; i < n; i += outer_count) {
      size_t off = 0;
      for (size_t j = 0; j < outer_count; ++j) {
        auto start = cycle_count_begin();
        size_t end_off = off + count;
        for (; off < end_off; ++off) {
          f(C, range[off]);
        }
        auto end = cycle_count_end();
        results[i + j] = end - start;
      }
    }
  };

  try {
    BenchmarkHelper helper;
    while (true) {
      helper.initialize(size / count, count, fill_results);

      if ((size % (count * 2)) || helper.overhead_is_okay())
        break;
      count *= 2;

    }

    helper.finalize(fill_results);
    helper.print_result(name);
  }
  catch (...) {
    std::cout << name << ": (exception)" << std::endl;
  }




  // benchmark_function_fine(name, [&](auto C, auto start_timing, auto end_timing) {
  //     for (size_t i = 0; i < size; ++i) {
  //       auto &&e = range[i];
  //       start_timing();
  //       f(C, e);
  //       end_timing(i);
  //     }
  //   },
  //   size);
}

template <class Func, class Range1, class Range2>
void benchmark_function_foreach(std::string const &name, Func &&f, Range1 const &range1, Range2 const &range2) {
  size_t size1 = boost::size(range1);
  size_t size2 = boost::size(range2);
  benchmark_function_fine(name, [&](auto C, auto start_timing, auto end_timing) {
      for (size_t i = 0; i < size1; ++i) {
        auto &&e1 = range1[i];
        for (size_t j = 0; j < size2; ++j) {
          auto &&e2 = range2[j];
          start_timing();
          f(C, e1, e2);
          end_timing(i * size2 + j);
        }
      }
    },
    size1 * size2);
}

}


#endif /* HEADER GUARD */
