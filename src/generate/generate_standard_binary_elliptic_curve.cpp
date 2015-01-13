#include <boost/program_options.hpp>
#include "jbms/binary_field/detail/limb.hpp"

#include "jbms/openssl/ec.hpp"
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
void write_element(std::ostream &os, bignum const &element) {
  using jbms::binary_field::word_bits;
  using jbms::binary_field::word_t;

  size_t num_words = jbms::div_ceil(element.num_bits(), word_bits);
  os << "{";
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

struct CodeWriter {
  std::ofstream h_out, cpp_out;
  jbms::openssl::ec_group ec_group;
  std::string curve_name;
  jbms::openssl::bignum p, a, b, b_sqrt;
  std::vector<int> modulus;
  std::string modulus_with_commas, modulus_with_underscores, field_name;
  CodeWriter(std::string const &curve_name)
    : curve_name(curve_name) {
    ec_group = jbms::openssl::ec_group::by_curve_name(OBJ_txt2nid(curve_name.c_str()));
    bn_ctx ctx;

    ec_group.get_curve_GF2m(p, a, b, ctx);
    GF2m_mod_sqrt(b_sqrt, b, p, ctx);
    modulus = GF2m_poly2arr(p);

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
    h_out << "#include \"jbms/binary_field/GF2_" << modulus_with_underscores << ".hpp\"\n";
    cpp_out << "#include \"./" << curve_name << ".hpp\"\n";

    open_ns(h_out);
    open_ns(cpp_out);

    h_out << "class " << curve_name << " {\n"
          << "public:\n"
          << "  using Field = jbms::binary_field::" << field_name << ";\n"
          << "  constexpr static Field field() { return {}; }\n"
          << "  constexpr static size_t num_compressed_bytes() {\n"
          << "    return Field::num_bytes();\n"
          << "  }\n";

    auto write_value = [&](std::string const &name, bignum const &x) {
      std::string t, init;
      if (x.is_one()) {
        t = "jbms::binary_field::One";
      } else if (x.is_zero()) {
        t = "jbms::binary_field::Zero";
      }
      if (!t.empty()) {
        h_out << "public:\n"
              << "  constexpr static const " << t << " " << name << "() { return {}; }\n";
      }
      else {
        h_out << "private:\n"
              << "  static const Field::Element " << name << "_;\n"
        << "public:\n"
        << "  constexpr static Field::Element const &" << name << "() { return " << name << "_; }\n";
        cpp_out << curve_name << "::Field::Element const " << curve_name << "::" << name << "_";
        write_element(cpp_out, x);
        cpp_out << ";\n";
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
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("Help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  auto curve = vm["curve"].as<std::string>();
  auto h_name = vm["h_name"].as<std::string>();
  auto cpp_name = vm["cpp_name"].as<std::string>();

  CodeWriter writer(curve);

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
