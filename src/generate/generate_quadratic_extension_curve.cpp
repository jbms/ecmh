#include <boost/program_options.hpp>
#include "jbms/binary_field/detail/limb.hpp"

#include "jbms/openssl/ec.hpp"
#include "jbms/binary_field/QuadraticExtension.hpp"
#include "jbms/binary_field/GF2m.hpp"
#include "jbms/openssl/bn.hpp"
#include <openssl/objects.h>

#include "jbms/division.hpp"

#include <boost/io/ios_state.hpp>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <random>

namespace po = boost::program_options;
using jbms::openssl::bignum;
using jbms::openssl::bn_ctx;

struct CodeWriter {
  std::ofstream h_out, cpp_out;
  std::vector<int> modulus;
  using BaseField = jbms::binary_field::GF2m;
  BaseField base_field{modulus};
  using Field = jbms::binary_field::QuadraticExtension<BaseField>;
  Field F{base_field};
  std::string curve_name;
  std::string modulus_with_commas, modulus_with_underscores, base_field_name;
  typename Field::Element a, b, b_sqrt;

  CodeWriter(std::vector<int> const &modulus, std::string const &curve_name, std::string const &a_hex, std::string const &b_hex)
    : modulus(modulus), curve_name(curve_name) {
    for (auto &&x : modulus) {
      if (x == 0)
        break;
      if (!modulus_with_commas.empty()) modulus_with_commas += ',';
      if (!modulus_with_underscores.empty()) modulus_with_underscores += '_';
      modulus_with_commas += std::to_string(x);
      modulus_with_underscores += std::to_string(x);
    }
    base_field_name = "GF2<" + modulus_with_commas + ">";
    assign_from_hex(F, a, a_hex);
    assign_from_hex(F, b, b_hex);
    sqrt(F, b_sqrt, b);
  }

  void write_element(std::ostream &os, bignum const &element) {
    using jbms::binary_field::word_bits;
    using jbms::binary_field::word_t;

    size_t num_words = std::max(size_t(1), jbms::div_ceil(element.num_bits(), word_bits));
    os << "jbms::binary_field::BinaryPolynomial<" << modulus.front() << ">{";
    for (size_t word_i = 0; word_i < num_words; ++word_i) {
      word_t word = 0;
      size_t base_bit = word_i * word_bits;
      for (size_t bit_i = 0; bit_i < word_bits; ++bit_i) {
        if (element.is_bit_set((int)(bit_i + base_bit)))
          word |= (word_t(1) << bit_i);
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

  std::string element_str(bignum const &element) {
    std::ostringstream ostr;
    write_element(ostr, element);
    return ostr.str();
  }

  void write_header_guard() {
    bignum x;
    rand(x, 128, -1, 0);
    h_out << "#ifndef HEADER_GUARD_" << x.to_hex() << "\n"
          << "#define HEADER_GUARD_" << x.to_hex() << "\n";
  }
  void open_ns(std::ostream &os) {
    os << "namespace jbms {\n"
       << "namespace binary_elliptic_curve {\n";

  }
  void close_ns(std::ostream &os) {
    os << "} // namespace jbms::binary_elliptic_curve\n} // namespace jbms\n";
  }

  void write() {
    write_header_guard();
    h_out << "#include \"jbms/binary_field/GF2_" << F.degree() << ".hpp\"\n";
    cpp_out << "#include \"./" << curve_name << ".hpp\"\n";

    open_ns(h_out);
    open_ns(cpp_out);

    h_out << "class " << curve_name << " {\n"
          << "public:\n"
          << "  using Field = jbms::binary_field::GF2_" << F.degree() << ";\n"
          << "  constexpr static Field field() { return {}; }\n"
          << "  constexpr static size_t num_compressed_bytes() {\n"
          << "    return field().num_bytes();\n"
          << "  }\n";

    auto write_value = [&](std::string const &name, Field::Element const &x) {
      std::string t = "Field::Element";
      std::string init_expr;
      if (is_zero(F, x)) {
        t = "jbms::binary_field::One";
      } else if (is_one(F, x)) {
        t = "jbms::binary_field::Zero";
      } else if (is_zero(F.base_field(), x[1])) {
        t = curve_name + "::Field::BaseField::Element";
        init_expr = "{" + element_str(x[0]) + "}";
      } else if (is_zero(F.base_field(), x[0]) || equal(F.base_field(), x[0], x[1])) {
        std::string sub_t = curve_name + "::Field::BaseField::Element";
        std::string name = is_zero(F.base_field(), x[0]) ? "QuadraticU" : "QuadraticUp1";
        if (is_one(F, x[1])) {
          sub_t = "jbms::binary_field::One";
        } else
          init_expr = "{" + element_str(x[1]) + "}";
        t = "jbms::binary_field::" + name + "<" + sub_t + ">";
      } else {
        init_expr = "= {{" + element_str(x[0]) + ", " + element_str(x[1]) + "}}";
      }

      if (!init_expr.empty()) {
        h_out << "private:\n"
              << "  static const " << t << " " << name << "_;\n"
              << "public:\n"
              << "  constexpr static " << t << " const &" << name << "() { return " << name << "_; }\n";
        cpp_out << t << " const " << curve_name << "::" << name << "_ " << init_expr << ";\n";
      } else {
        h_out << "public:\n"
              << "  constexpr static " << t << " const " << name << "() { return {}; }\n";
      }
    };

    write_value("a", a);
    write_value("b", b);
    write_value("sqrt_b", b_sqrt);

    h_out << "};\n"; // end class

    close_ns(h_out);
    close_ns(cpp_out);
    h_out << "#endif // HEADER GUARD\n";
  }
};

int main(int argc, char **argv) {
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("h_name", po::value<std::string>(), "header output name")
    ("cpp_name", po::value<std::string>(), "cpp output name")
    ("curve", po::value<std::string>(), "curve name")
    ("modulus", po::value<std::vector<int>>(), "modulus")
    ("a", po::value<std::string>(), "a hex representation")
    ("b", po::value<std::string>(), "b hex representation")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("Help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  auto modulus = vm["modulus"].as<std::vector<int>>();
  auto h_name = vm["h_name"].as<std::string>();
  auto curve = vm["curve"].as<std::string>();
  auto a_hex = vm["a"].as<std::string>();
  auto b_hex = vm["b"].as<std::string>();
  auto cpp_name = vm["cpp_name"].as<std::string>();

  CodeWriter writer(modulus, curve, a_hex, b_hex);

  writer.h_out.open(h_name + ".tmp");
  writer.cpp_out.open(cpp_name + ".tmp");
  if (rename((h_name + ".tmp").c_str(), h_name.c_str()) != 0) {
    perror("rename");
    return 1;
  }
  if (rename((cpp_name + ".tmp").c_str(), cpp_name.c_str()) != 0) {
    perror("rename");
    return 1;
  }

  writer.write();
  return 0;
}
