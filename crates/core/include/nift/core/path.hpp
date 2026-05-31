#pragma once
/// @file path.hpp
/// @brief Modern path manipulation — wraps std::filesystem::path with
///        Nift-specific conveniences (extension handling, relative resolution).

#include <filesystem>
#include <string>
#include <string_view>

#include "nift/core/types.hpp"

namespace nift::core {

namespace fs = std::filesystem;

/// A thin, value-type wrapper around std::filesystem::path.
/// Provides Nift-specific helpers on top of the standard path.
class Path {
 public:
  Path() = default;
  explicit Path(fs::path p) : path_(std::move(p)) {}
  explicit Path(const char* s) : path_(s) {}
  explicit Path(const std::string& s) : path_(s) {}
  explicit Path(std::string_view sv) : path_(std::string(sv)) {}

  // ── Observers ─────────────────────────────────────────────
  [[nodiscard]] const fs::path& native() const noexcept { return path_; }
  [[nodiscard]] std::string str() const { return path_.string(); }
  [[nodiscard]] std::string_view filename() const;
  [[nodiscard]] std::string_view stem() const;
  [[nodiscard]] std::string_view extension() const;
  [[nodiscard]] Path parent() const;

  [[nodiscard]] bool is_absolute() const { return path_.is_absolute(); }
  [[nodiscard]] bool is_relative() const { return path_.is_relative(); }
  [[nodiscard]] bool empty() const noexcept { return path_.empty(); }

  // ── Manipulation ──────────────────────────────────────────
  /// Replace extension (e.g. ".html" → ".md").
  [[nodiscard]] Path with_extension(std::string_view ext) const;
  /// Join a relative component.
  [[nodiscard]] Path join(std::string_view component) const;
  /// Make relative to a base directory.
  [[nodiscard]] Path relative_to(const Path& base) const;
  /// Normalize (lexically, no disk access).
  [[nodiscard]] Path normalized() const;

  // ── Comparison ────────────────────────────────────────────
  [[nodiscard]] bool operator==(const Path& o) const {
    return path_ == o.path_;
  }
  [[nodiscard]] bool operator!=(const Path& o) const { return !(*this == o); }
  [[nodiscard]] auto operator<=>(const Path& o) const {
    return path_ <=> o.path_;
  }

 private:
  fs::path path_;
  // Cache string reps for string_view returns (lifetime safety).
  mutable std::string filename_cache_;
  mutable std::string stem_cache_;
  mutable std::string ext_cache_;
};

}  // namespace nift::core
