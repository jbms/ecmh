#include "jbms/binary_field/GF2m.hpp"
#include "jbms/binary_field/detail/limb.hpp"

#include <boost/program_options.hpp>
#include <boost/io/ios_state.hpp>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <random>

namespace po = boost::program_options;

namespace jbms {
namespace binary_field {

struct CodeWriter {
  using Field = jbms::binary_field::GF2m;
  using FE = Field::Element;
  std::vector<int> modulus;
  Field F;
  std::ofstream h_out, cpp_out;
  std::string modulus_with_commas, modulus_with_underscores, field_name;
  bool copy_to_huge_pages = false;
  struct HugePageTable {
    std::string name;
    size_t block_bits, num_blocks, max_val /* actually 1 + maximum block value */;
  };
  std::vector<HugePageTable> huge_page_tables;

  CodeWriter(std::vector<int> const &modulus)
    : modulus(modulus), F(modulus) {
    for (auto &&x : modulus) {
      if (x == 0)
        break;
      if (!modulus_with_commas.empty()) modulus_with_commas += ',';
      if (!modulus_with_underscores.empty()) modulus_with_underscores += '_';
      modulus_with_commas += std::to_string(x);
      modulus_with_underscores += std::to_string(x);
    }
    field_name = "GF2<" + modulus_with_commas + ">";
  }


  void write_header_guard() {
    jbms::openssl::bignum x;
    rand(x, 128, -1, 0);
    h_out << "#ifndef HEADER_GUARD_" << x.to_hex() << "\n"
          << "#define HEADER_GUARD_" << x.to_hex() << "\n";
  }

  void open_ns(std::ostream &os) {
    os << "namespace jbms {\n"
       << "namespace binary_field {\n";

  }

  void write_h_include(std::string const &field_name) {
    cpp_out << "#include \"" << field_name << ".hpp\"\n";
    cpp_out << "#include \"jbms/hugepages.hpp\"\n";
  }

  void close_ns(std::ostream &os) {
    os << "} // namespace jbms::binary_field\n} // namespace jbms\n";
  }

  void write_start() {
    write_header_guard();
    h_out << "#include \"jbms/binary_field/detail/polynomial_base.hpp\"\n"
          << "#include \"jbms/binary_field/detail/polynomial_reduce_" << modulus_with_underscores << ".hpp\"\n"
          << "#include \"jbms/binary_field/detail/polynomial_multiply.hpp\"\n"
          << "#include \"jbms/binary_field/detail/apply_linear_table_transform.hpp\"\n";
    open_ns(h_out);
    h_out << "using GF2_" << modulus_with_underscores << " = GF2<" << modulus_with_commas << ">;\n";
    close_ns(h_out);

    // GCC 4.9.1 has a bug: if the array definitions are preceeded by extern declarations, we get an ICE
    // We work around that by skipping the extern definitions in the cpp file.

    // This indicates to the header file that it is being included from the cpp file
    // This is needed in order to skip the extern declarations for the arrays
    //cpp_out << "#define JBMS_BINARY_FIELD_GF2_" << F.degree() << "_IMPL\n";
    //cpp_out << "#include \"jbms/binary_field/detail/polynomial_base.hpp\"\n";
  }

  void write_end() {
    h_out << "#endif // HEADER GUARD\n";
  }

  void write_element(std::ostream &os, FE const &element) {
    size_t m = F.degree();
    size_t num_words = div_ceil(m, word_bits);
    os << "BinaryPolynomial<" << m << ">{";
    for (size_t word_i = 0; word_i < num_words; ++word_i) {
      uint64_t word = 0;
      size_t base_bit = word_i * word_bits;
      size_t max_bit = std::min(word_bits, m - base_bit);
      for (size_t bit_i = 0; bit_i < max_bit; ++bit_i) {
        if (element.is_bit_set((int)(bit_i + base_bit)))
          word |= (uint64_t(1) << bit_i);
      }
      if (word_i != 0)
        os << ", ";
      {
        boost::io::ios_all_saver saver(os);
        os << "0x" << std::setfill('0') << std::hex << std::setw(word_bits / 4) << word << "ul";
      }
    }
    os << "}";
  }

  void write_trace() {
    open_ns(h_out);
    h_out << "inline bool trace(" << field_name << " const &F, BinaryPolynomial<" << F.degree() << "> const &x) {\n"
          << "  return (";
    bool first_bit = true;
    size_t degree = F.degree();
    size_t first_trace_bit = 0;
    for (size_t i = 0; i < degree; ++i) {
      FE x;
      x.set_zero();
      x.set_bit(i);
      if (trace(F, x)) {
        if (!first_bit) {
          h_out << "^";
        } else {
          first_trace_bit = i;
        }
        first_bit = false;
        auto vec_i = i / limb_bits;
        auto vec_off = i % limb_bits;
        auto word_i = vec_off / word_bits;
        auto word_off = vec_off % word_bits;

        h_out << "(x.limbs[" << vec_i << "][" << word_i << "] >> " << word_off << ")";
      }
    }
    h_out << ") & 0x1;\n}\n";

    // Function that returns value x' = x if Tr(x) = 0, and otherwise differs from x only in one bit.
    h_out << "inline void set_trace_zero(" << field_name << " const &F, BinaryPolynomial<" << F.degree() << "> &x) {\n";
    {
      size_t i = first_trace_bit;
      auto vec_i = i / limb_bits;
      auto vec_off = i % limb_bits;
      auto word_i = vec_off / word_bits;
      auto word_off = vec_off % word_bits;
      h_out << "x.limbs[" << vec_i << "][" << word_i << "] ^= (word_t(trace(F, x)) << " << word_off << ");\n";
    }
    h_out << "}\n";
    close_ns(h_out);
  }

  void write_linear_operation_table(std::string const &name, size_t block_bits, std::function<FE (FE const &)> const &op) {
    std::vector<FE> monomial_results;
    size_t degree = F.degree();
    for (size_t i = 0; i < degree; ++i) {
      FE x;
      x.set_zero();
      x.set_bit(i);
      monomial_results.push_back(op(x));
    }


    size_t num_blocks = div_ceil(degree, block_bits);
    size_t max_val = (size_t(1) << block_bits);

    // If copy_to_huge_pages is set, at static initialization time, we allocate an array backed by huge pages sufficient to hold
    // all of the tables, copy the tables from the data section into the newly allocated array, and update the global references
    // for each table to point to the allocated table.

    open_ns(h_out);

    h_out << "namespace detail {\n"
          << "extern const BinaryPolynomial<" << degree << "> ";
    if (copy_to_huge_pages)
      h_out << "(&" << name << ")";
    else
      h_out << name;
    h_out << "[" << num_blocks << "][" << max_val << "];\n"
          << "}\n";

    close_ns(h_out);

    open_ns(cpp_out);
    cpp_out << "namespace detail {\n";

    // Write the actual data-section array
    // If copy_to_huge_pages, then this is declared static and with a "_orig"-suffixed name since it won't be used directly
    if (copy_to_huge_pages)
      cpp_out << "static ";
    cpp_out << "const BinaryPolynomial<" << degree << "> " << name;
    if (copy_to_huge_pages)
      cpp_out << "_orig";
    cpp_out << "[" << num_blocks << "][" << max_val << "] = {\n";

    for (size_t block_i = 0; block_i < num_blocks; ++block_i) {
      cpp_out << "  /* block " << block_i << " */ {\n";
      for (size_t val = 0; val < max_val; ++val) {
        FE x;
        x.set_zero();
        size_t base_bit = block_i * block_bits;
        size_t max_bit = std::min(block_bits, degree - base_bit);
        for (size_t bit_i = 0; bit_i < max_bit; ++bit_i) {
          if ((val >> bit_i) & 1)
            add(F, x, x, monomial_results[bit_i + base_bit]);
        }
        cpp_out << "    ";
        write_element(cpp_out, x);
        if (val + 1 != max_val)
          cpp_out << ",";
        cpp_out << "\n";
      }
      cpp_out << "  }"; // end block
      if (block_i + 1 != num_blocks)
        cpp_out << ",";
      cpp_out << "\n";
    }

    cpp_out << "}; // end " << name << "\n\n";

    if (copy_to_huge_pages) {
      // We will allocate a single huge page array for all tables
      // We will write the initialization code later
      HugePageTable t;
      t.name = name;
      t.block_bits = block_bits;
      t.max_val = max_val;
      t.num_blocks = num_blocks;
      huge_page_tables.push_back(t);
    }
    cpp_out << "}\n"; // end detail namespace
    close_ns(cpp_out);
  }

  void write_huge_page_tables_initializer() {
    if (huge_page_tables.empty()) return;


    size_t total_elements = 0;
    for (auto &&t : huge_page_tables) {
      total_elements += t.max_val * t.num_blocks;
    }
    auto degree = F.degree();

    open_ns(cpp_out);
    cpp_out << "namespace detail {\n"
            << "static struct hugepage_table_initializer_t {\n"
            << "unique_hugepage_ptr<BinaryPolynomial<" << degree << ">[]> buffer;\n"
            << "hugepage_table_initializer_t()"
            << " : buffer(allocate_hugepage_array<BinaryPolynomial<" << degree << ">>(" << total_elements << ")) {";
    {
      size_t off = 0;
      for (auto &&t : huge_page_tables) {
        cpp_out << "memcpy(buffer.get() + " << off << ", " << t.name << "_orig, sizeof(" << t.name << "_orig));\n";
        off += t.max_val * t.num_blocks;
      }
    }
    cpp_out << "}\n"
            << "} hugepage_table_initializer;\n";
    size_t off = 0;
    for (auto &&t : huge_page_tables) {
      cpp_out << "const BinaryPolynomial<" << degree << "> (&" << t.name << ")[" << t.num_blocks << "][" << t.max_val << "] = "
              << "*(const BinaryPolynomial<" << degree << "> (*)[" << t.num_blocks << "][" << t.max_val << "])(hugepage_table_initializer.buffer.get() + " << off << ");\n";
      off += t.max_val * t.num_blocks;
    }
    cpp_out << "} // namespace detail\n";
    close_ns(cpp_out);
  }

  void write_half_trace(size_t block_bits) {
    auto m = F.degree();
    auto name = "half_trace_table_GF2_" + modulus_with_underscores;
    write_linear_operation_table(name, block_bits, [&](FE const &x) {
        auto result = half_trace(F, x);
        return result;
      });

    open_ns(h_out);
    h_out << "inline void half_trace(" << field_name << " const &F, BinaryPolynomial<" << m << "> &result, BinaryPolynomial<" << m
          << "> const &x) {\n"
          << "  apply_linear_table_transform<" << block_bits << ">(result, x, detail::" << name << ");\n"
          << "}\n";
    close_ns(h_out);
  }
  void write_multi_square(size_t n, size_t block_bits) {
    auto m = F.degree();
    auto name = "multi_square_table_" + std::to_string(n) + "_GF2_" + modulus_with_underscores;
    write_linear_operation_table(name, block_bits, [&](FE const &x) {
        FE result = x;
        for (size_t i = 0; i < n; ++i)
          square(F, result, result);
        return result;
      });
    open_ns(h_out);
    h_out << "template<>\n"
          << "inline void multi_square<" << n << ">(" << field_name << " const &F, BinaryPolynomial<" << m << "> &result, BinaryPolynomial<" << m
          << "> const &x) {\n"
          << "  apply_linear_table_transform<" << block_bits << ">(result, x, detail::" << name << ");\n"
          << "}\n";
    close_ns(h_out);
  }

  void write_invert(std::vector<std::pair<int,int>> const &action_chain) {
    auto m = F.degree();
    auto fe_str = "BinaryPolynomial<" + std::to_string(m) + ">";
    auto impl_proto = "void invert_impl(" + fe_str + " &result, " + fe_str + " const &a1)";

    open_ns(h_out);
    h_out << "namespace detail {\n"
          << impl_proto << ";\n"
          << "}\n"
          << "inline void invert(" << field_name << " const &F, " << fe_str << " &result, " << fe_str << " const &x) {\n"
          << "  detail::invert_impl(result, x);\n"
          << "}\n";

    // #define WRITE_INVERT_INLINE
#ifdef WRITE_INVERT_INLINE
    // FIXME: for debugging invert performance only
    h_out << "inline void invert_inline(" << field_name << " const &F, " << fe_str << " &result, " << fe_str << " const &a1){\n";
    for (auto &&action : action_chain) {
      auto x = action.first + action.second;
      h_out << fe_str << " a" << x << "; multi_square<" << action.first << ">(F, a" << x << ", a" << action.second << "); "
            << "multiply(F, a" << x << ", a" << x << ", a" << action.first << ");\n";
    }
    h_out << "square(F, result, a" << (m - 1) << ");\n"
          << "}\n";
#endif // ifdef WRITE_INVERT_INLINE

    // #define WRITE_INVERT_PREFIX
#ifdef WRITE_INVERT_PREFIX
    // FIXME: For debugging invert performance
    h_out << "template <size_t PrefixLen> void invert_prefix(" << field_name << " const &F, " << fe_str << " &result, "
          << fe_str << " const &a1){\n";
    for (size_t i = 0; i < action_chain.size(); ++i) {
      auto &&action = action_chain[i];
      auto x = action.first + action.second;
      h_out << fe_str << " a" << x << ";\n"
            << "if (" << i << " < PrefixLen) {\n"
            << "multi_square<" << action.first << ">(F, a" << x << ", a" << action.second << "); "
            << "multiply(F, a" << x << ", a" << x << ", a" << action.first << ");\n"
            << "}\n";
      h_out << "if (" << i + 1 << " == std::min(PrefixLen,(size_t)" << action_chain.size() << ")) {\n"
            << "square(F, result, a" << x << ");\n"
            << "}\n";
    }
    h_out << "}\n";
#endif // ifdef WRITE_INVERT_PREFIX

    close_ns(h_out);

    open_ns(cpp_out);

    cpp_out << "namespace detail {\n"
            << impl_proto << " {\n"
            << field_name << " F;\n";

    for (auto &&action : action_chain) {
      auto x = action.first + action.second;
      cpp_out << fe_str << " a" << x << "; multi_square<" << action.first << ">(F, a" << x << ", a" << action.second << "); "
              << "multiply(F, a" << x << ", a" << x << ", a" << action.first << ");\n";
    }

    cpp_out << "square(F, result, a" << (m - 1) << ");\n"
            << "}\n"
            << "}\n";

    close_ns(cpp_out);
  }
};
}
}
using jbms::binary_field::CodeWriter;
int main(int argc, char **argv) {
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("field_name", po::value<std::string>(), "field name (base name for header include)")
    ("h_name", po::value<std::string>(), "header output name")
    ("cpp_name", po::value<std::string>(), "cpp output name")
    ("modulus", po::value<std::vector<int>>(), "modulus")
    ("multi_square_block_size", po::value<size_t>(), "multi-square table block size (bits)")
    ("half_trace_block_size", po::value<size_t>(), "half trace table block size (bits)")
    ("multi_square_table", po::value<std::vector<int>>(), "multi-square tables")
    ("addition_chain", po::value<std::vector<int>>(), "addition chain (as action pairs)")
    ("copy_to_huge_pages", po::value<bool>(), "copy tables to huge pages")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("Help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  auto modulus = vm["modulus"].as<std::vector<int>>();
  auto field_name = vm["field_name"].as<std::string>();
  auto h_name = vm["h_name"].as<std::string>();
  auto cpp_name = vm["cpp_name"].as<std::string>();
  std::vector<int> multi_square_tables;
  try {
    multi_square_tables = vm["multi_square_table"].as<std::vector<int>>();
  } catch (std::exception const &) {}
  auto multi_square_block_size = vm["multi_square_block_size"].as<size_t>();
  auto half_trace_block_size = vm["half_trace_block_size"].as<size_t>();
  auto addition_chain_raw = vm["addition_chain"].as<std::vector<int>>();
  auto copy_to_huge_pages = vm["copy_to_huge_pages"].as<bool>();

  std::vector<std::pair<int,int>> addition_chain;
  if ((addition_chain_raw.size() % 2) != 0)
    throw std::runtime_error("addition chain must be list of pairs");
  for (size_t i = 0; i < addition_chain_raw.size(); i += 2)
    addition_chain.emplace_back(addition_chain_raw[i],addition_chain_raw[i+1]);

  CodeWriter writer{modulus};
  writer.h_out.open(h_name);
  writer.cpp_out.open(cpp_name);
  writer.copy_to_huge_pages = copy_to_huge_pages;

  writer.write_start();
  writer.write_h_include(field_name);

  writer.write_half_trace(half_trace_block_size);

  writer.write_trace();

  for (auto n : multi_square_tables)
    writer.write_multi_square(n, multi_square_block_size);

  writer.write_huge_page_tables_initializer();

  writer.h_out << "\n\n/**\n"
               << " * Itoh-Tsujii invert implementation using polynomial basis\n"
               << " * Addition chain:";

  for (auto &&x : addition_chain) {
    writer.h_out << " " << x.first + x.second << "=(" << x.first << " + " << x.second << ")";
  }

  writer.h_out << "\n"
               << " * Multi-square tables:";
  for (auto x : multi_square_tables)
    writer.h_out << " " << x;

  writer.h_out << "\n"
               << " **/\n\n";

  writer.write_invert(addition_chain);


  writer.write_end();

  return 0;
}
