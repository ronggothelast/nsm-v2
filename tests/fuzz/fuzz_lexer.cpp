/// @file fuzz_lexer.cpp
/// @brief libFuzzer harness for the Nift template lexer.

#include <cstddef>
#include <cstdint>
#include <string>

#include "nift/parser/lexer.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size == 0)
    return 0;

  std::string input(reinterpret_cast<const char*>(data), size);

  try {
    nift::parser::Lexer lexer(input);
    while (true) {
      auto tok = lexer.next_token();
      if (tok.type == nift::parser::TokenType::Eof)
        break;
    }
  } catch (...) {
    // Acceptable — fuzzer finds UB, not exceptions.
  }

  return 0;
}
