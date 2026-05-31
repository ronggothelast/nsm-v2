/// @file mmap_reader.hpp
/// @brief Memory-mapped file reader for zero-copy template parsing.
///
/// Wraps `mio::mmap_source` with a clean nift API. On platforms where
/// mmap is unavailable (or for very small files), falls back to a heap
/// read — caller code stays the same since both expose `string_view`.

#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "nift/core/path.hpp"
#include "nift/core/types.hpp"

namespace nift::build {

class MmapReader {
 public:
  /// Open a file via mmap. On success, `data()` is a non-owning view over
  /// the mapped region until the reader is destructed.
  static ::nift::Expected<MmapReader, ::nift::Error> open(
      const ::nift::core::Path& path);

  ~MmapReader();
  MmapReader(MmapReader&&) noexcept;
  MmapReader& operator=(MmapReader&&) noexcept;

  MmapReader(const MmapReader&) = delete;
  MmapReader& operator=(const MmapReader&) = delete;

  /// View over the mapped (or buffered) bytes. Stable until destruction.
  std::string_view data() const noexcept;

  /// Size in bytes.
  std::size_t size() const noexcept;

  /// Whether this reader uses mmap (false → heap fallback).
  bool is_mapped() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  MmapReader();
};

}  // namespace nift::build
