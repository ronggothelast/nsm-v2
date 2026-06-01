# Fuzz targets for Nift v2
# Build with: cmake -DNIFT_QA_FUZZING=ON -DCMAKE_CXX_COMPILER=clang++

if(NOT NIFT_QA_FUZZING)
  return()
endif()

# Parser fuzz harness
add_executable(fuzz_parser fuzz_parser.cpp)
target_link_libraries(fuzz_parser PRIVATE nift_parser nift_core)
target_compile_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address,undefined)

# Lexer fuzz harness
add_executable(fuzz_lexer fuzz_lexer.cpp)
target_link_libraries(fuzz_lexer PRIVATE nift_parser nift_core)
target_compile_options(fuzz_lexer PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(fuzz_lexer PRIVATE -fsanitize=fuzzer,address,undefined)
