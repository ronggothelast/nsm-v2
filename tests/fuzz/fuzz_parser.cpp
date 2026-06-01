/// @file fuzz_parser.cpp
/// @brief libFuzzer harness for the Nift template parser.
/// Finds crashes, hangs, and memory errors in parser input handling.

#include <cstddef>
#include <cstdint>
#include <string>

#include "nift/parser/lexer.hpp"
#include "nift/parser/parser.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Skip empty inputs
  if (size == 0)
    return 0;

  std::string input(reinterpret_cast<const char*>(data), size);

  try {
    nift::parser::Lexer lexer(input);
    nift::parser::Parser parser(lexer);
    auto result = parser.parse_program();
    (void)result;
  } catch (...) {
    // Parser may throw on malformed input — that is acceptable.
    // Fuzzer finds UB, not exceptions.
  }

  return 0;
}
