/// @file test_argparse.cpp

#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "nift/cli/argparse.hpp"

using namespace nift::cli;

namespace {
ParsedArgs parse_v(std::vector<const char*> argv,
                   const std::vector<std::string>& bools = {}) {
  std::vector<char*> mut;
  mut.reserve(argv.size());
  for (auto* s : argv)
    mut.push_back(const_cast<char*>(s));
  return parse(static_cast<int>(mut.size()), mut.data(), bools);
}
}  // namespace

TEST_CASE("argparse: subcommand", "[cli][argparse]") {
  auto a = parse_v({"nift", "build"});
  CHECK(a.subcommand == "build");
}

TEST_CASE("argparse: positional", "[cli][argparse]") {
  auto a = parse_v({"nift", "build", "./mysite"});
  CHECK(a.subcommand == "build");
  REQUIRE(a.positional.size() == 1);
  CHECK(a.positional[0] == "./mysite");
}

TEST_CASE("argparse: long flag with =", "[cli][argparse]") {
  auto a = parse_v({"nift", "serve", "--port=3000"});
  CHECK(a.get("port") == "3000");
}

TEST_CASE("argparse: long flag with separate value", "[cli][argparse]") {
  auto a = parse_v({"nift", "serve", "--host", "0.0.0.0"});
  CHECK(a.get("host") == "0.0.0.0");
}

TEST_CASE("argparse: bool flag", "[cli][argparse]") {
  auto a = parse_v({"nift", "build", "--quiet"}, {"quiet"});
  CHECK(a.get_bool("quiet") == true);
}

TEST_CASE("argparse: missing flag -> default", "[cli][argparse]") {
  auto a = parse_v({"nift", "build"});
  CHECK(a.get("port", "8080") == "8080");
  CHECK(a.get_bool("quiet", false) == false);
}

TEST_CASE("argparse: short flag bundle", "[cli][argparse]") {
  auto a = parse_v({"nift", "build", "-qV"}, {"q", "V"});
  CHECK(a.get_bool("q") == true);
  CHECK(a.get_bool("V") == true);
}

TEST_CASE("argparse: -- stops flag parsing", "[cli][argparse]") {
  auto a = parse_v({"nift", "build", "--", "--quiet"});
  CHECK_FALSE(a.has("quiet"));
  REQUIRE(a.positional.size() == 1);
  CHECK(a.positional[0] == "--quiet");
}

TEST_CASE("argparse: has() works for bool and string flags", "[cli][argparse]") {
  auto a = parse_v({"nift", "build", "--port=3000", "--quiet"}, {"quiet"});
  CHECK(a.has("port"));
  CHECK(a.has("quiet"));
  CHECK_FALSE(a.has("nonexistent"));
}

TEST_CASE("argparse: empty argv", "[cli][argparse]") {
  auto a = parse_v({"nift"});
  CHECK(a.subcommand.empty());
  CHECK(a.positional.empty());
}

TEST_CASE("argparse: numeric value via short flag", "[cli][argparse]") {
  auto a = parse_v({"nift", "serve", "-p", "9000"});
  CHECK(a.get("p") == "9000");
}

TEST_CASE("argparse: get_bool with --flag=true", "[cli][argparse]") {
  auto a = parse_v({"nift", "build", "--quiet=true"});
  CHECK(a.get_bool("quiet") == true);
}

TEST_CASE("argparse: get_bool with --flag=false", "[cli][argparse]") {
  auto a = parse_v({"nift", "build", "--quiet=false"});
  CHECK(a.get_bool("quiet") == false);
}
