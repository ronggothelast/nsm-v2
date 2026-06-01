/// @file simd_scanner.cpp
/// @brief xsimd-based byte scanning with scalar fallback.

#include "nift/build/simd_scanner.hpp"

#include <algorithm>
#include <cstring>
#include <xsimd/xsimd.hpp>

namespace nift::build {

namespace {

/// SIMD batch type — picks AVX2 / SSE4.2 / NEON depending on the build.
using batch_t = xsimd::batch<std::uint8_t>;
constexpr std::size_t kBatch = batch_t::size;

/// SIMD scan for any of `n_targets` bytes — returns positions sorted asc.
std::vector<std::size_t> scan_simd(std::string_view buf, const char* targets,
                                   std::size_t n_targets) {
  std::vector<std::size_t> out;
  if (buf.empty() || n_targets == 0)
    return out;

  const std::uint8_t* data = reinterpret_cast<const std::uint8_t*>(buf.data());
  std::size_t n = buf.size();

  // Pre-broadcast each target as a SIMD batch.
  batch_t target_batches[8]{};  // up to 8 targets — we use ≤ 3
  std::size_t actual_targets = std::min<std::size_t>(n_targets, 8);
  for (std::size_t t = 0; t < actual_targets; ++t) {
    target_batches[t] = batch_t::broadcast(static_cast<std::uint8_t>(targets[t]));
  }

  std::size_t i = 0;
  // Main SIMD loop.
  for (; i + kBatch <= n; i += kBatch) {
    batch_t v = batch_t::load_unaligned(data + i);
    auto matched = (v == target_batches[0]);
    for (std::size_t t = 1; t < actual_targets; ++t) {
      matched = matched | (v == target_batches[t]);
    }
    // Convert mask to bitmask and bit-scan.
    auto mask = matched.mask();
    std::uint64_t bits = static_cast<std::uint64_t>(mask);
    while (bits) {
      // count trailing zeros
      unsigned int b;
#if defined(__GNUC__) || defined(__clang__)
      b = static_cast<unsigned>(__builtin_ctzll(bits));
#else
      b = 0;
      while (((bits >> b) & 1u) == 0)
        ++b;
#endif
      out.push_back(i + b);
      bits &= bits - 1;
    }
  }

  // Scalar tail.
  for (; i < n; ++i) {
    for (std::size_t t = 0; t < actual_targets; ++t) {
      if (data[i] == static_cast<std::uint8_t>(targets[t])) {
        out.push_back(i);
        break;
      }
    }
  }
  return out;
}

}  // namespace

std::vector<std::size_t> scan_positions(std::string_view buffer, ScanTargets targets) {
  char tgt[3];
  std::size_t n = 0;
  if (targets.find_at)
    tgt[n++] = '@';
  if (targets.find_dollar)
    tgt[n++] = '$';
  if (targets.find_backslash)
    tgt[n++] = '\\';
  if (n == 0)
    return {};
  return scan_simd(buffer, tgt, n);
}

std::vector<std::size_t> scan_byte(std::string_view buffer, char target) {
  return scan_simd(buffer, &target, 1);
}

bool contains_any(std::string_view buffer, ScanTargets targets) {
  return find_first(buffer, targets) != std::string_view::npos;
}

std::size_t find_first(std::string_view buffer, ScanTargets targets) {
  auto positions = scan_positions(buffer, targets);
  if (positions.empty())
    return std::string_view::npos;
  return positions.front();
}

}  // namespace nift::build
