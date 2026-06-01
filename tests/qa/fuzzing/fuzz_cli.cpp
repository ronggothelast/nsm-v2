// tests/qa/fuzzing/fuzz_cli.cpp
// libFuzzer harness for the CLI argument parser. Splits the input on NUL
// bytes to synthesize argv, then feeds it to the same parser the binary uses.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "nift/cli/argparse.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  if (size == 0 || size > 64 * 1024)
    return 0;

  std::vector<std::string> argv_storage;
  argv_storage.emplace_back("nift");
  std::string current;
  for (std::size_t i = 0; i < size; ++i) {
    if (data[i] == '\0') {
      argv_storage.push_back(std::move(current));
      current.clear();
      if (argv_storage.size() > 64)
        break;  // bound argv length
    } else {
      current.push_back(static_cast<char>(data[i]));
    }
  }
  if (!current.empty())
    argv_storage.push_back(std::move(current));

  std::vector<const char*> argv;
  argv.reserve(argv_storage.size());
  for (auto& s : argv_storage)
    argv.push_back(s.c_str());

  try {
    nift::cli::ArgParser parser;
    (void)parser.parse(static_cast<int>(argv.size()), argv.data());
  } catch (const std::exception&) {
  } catch (...) {}
  return 0;
}
