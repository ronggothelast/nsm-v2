# tests/bench/CMakeLists.txt
# Micro-benchmarks for hot paths.

add_executable(bench_simd_scanner bench_simd_scanner.cpp)
target_link_libraries(bench_simd_scanner PRIVATE nift::build)
