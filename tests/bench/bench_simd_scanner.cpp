/// @file bench_simd_scanner.cpp
/// @brief Micro-benchmark: SIMD scanner vs scalar baseline.
///
/// Run: ./build/debug/tests/bench/bench_simd_scanner
/// Output: median ns/byte for both paths.

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "nift/build/simd_scanner.hpp"

namespace {

std::vector<std::size_t> scalar_scan(std::string_view buf, char target) {
  std::vector<std::size_t> out;
  for (std::size_t i = 0; i < buf.size(); ++i) {
    if (buf[i] == target) out.push_back(i);
  }
  return out;
}

template <typename Fn>
double measure_ns_per_byte(std::size_t n_bytes, Fn fn, int reps) {
  std::vector<long long> samples;
  samples.reserve(reps);
  for (int r = 0; r < reps; ++r) {
    auto t0 = std::chrono::steady_clock::now();
    fn();
    auto t1 = std::chrono::steady_clock::now();
    samples.push_back(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
  }
  std::sort(samples.begin(), samples.end());
  return static_cast<double>(samples[samples.size() / 2]) /
         static_cast<double>(n_bytes);
}

}  // namespace

int main() {
  // 1 MB buffer with sparse `@` markers (~ 1 in every 100 bytes).
  constexpr std::size_t kSize = 1 * 1024 * 1024;
  std::string buf(kSize, 'x');
  std::mt19937 rng(0xC0FFEE);
  std::uniform_int_distribution<std::size_t> dist(0, kSize - 1);
  for (int i = 0; i < 10000; ++i) buf[dist(rng)] = '@';

  // Warmup
  for (int i = 0; i < 5; ++i) {
    (void)::nift::build::scan_byte(buf, '@');
    (void)scalar_scan(buf, '@');
  }

  constexpr int kReps = 50;
  double simd_ns_per_byte = measure_ns_per_byte(
      kSize, [&] { (void)::nift::build::scan_byte(buf, '@'); }, kReps);
  double scalar_ns_per_byte = measure_ns_per_byte(
      kSize, [&] { (void)scalar_scan(buf, '@'); }, kReps);

  std::cout << "Buffer size:    " << kSize << " bytes\n";
  std::cout << "SIMD scan:      " << simd_ns_per_byte << " ns/byte\n";
  std::cout << "Scalar scan:    " << scalar_ns_per_byte << " ns/byte\n";
  std::cout << "Speedup:        " << (scalar_ns_per_byte / simd_ns_per_byte)
            << "x\n";

  return 0;
}
