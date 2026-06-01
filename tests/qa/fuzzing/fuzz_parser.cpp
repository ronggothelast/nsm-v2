// tests/qa/fuzzing/fuzz_parser.cpp
// libFuzzer harness for the @-directive parser. Targets template parsing
// where an attacker controls a content file. Builds with:
//   clang++ -std=c++20 -fsanitize=fuzzer,address,undefined ...
//
// Catches: heap-buffer-overflow, stack-overflow, infinite recursion, UB.
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "nift/parser/lexer.hpp"
#include "nift/parser/parser.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size == 0 || size > 1 << 20) return 0;  // 1 MiB cap
    std::string_view sv(reinterpret_cast<const char*>(data), size);

    try {
        nift::parser::Lexer lexer(sv);
        auto tokens = lexer.tokenize();
        nift::parser::Parser parser(std::move(tokens));
        (void)parser.parse();
    } catch (const std::exception&) {
        // Expected: malformed input throws. UB / OOM / crash → libFuzzer catches.
    } catch (...) {
        // Don't let unknown exception types bubble out of the harness.
    }
    return 0;
}
