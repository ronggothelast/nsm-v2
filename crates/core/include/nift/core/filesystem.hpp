#pragma once
/// @file filesystem.hpp
/// @brief File I/O utilities — read, write, exists, directory ops.
///        Returns nift::Result<T> instead of throwing.

#include <string>
#include <string_view>
#include <vector>

#include "nift/core/path.hpp"
#include "nift/core/types.hpp"

namespace nift::core {

/// Read entire file into string. Returns Error::not_found or Error::io_error.
[[nodiscard]] Result<std::string> read_file(const Path& path);

/// Write string to file, creating parent dirs. Returns Error::io_error.
[[nodiscard]] Status write_file(const Path& path, std::string_view content);

/// Check if path exists on disk.
[[nodiscard]] bool file_exists(const Path& path) noexcept;

/// Check if path is a directory.
[[nodiscard]] bool is_directory(const Path& path) noexcept;

/// List directory entries (non-recursive). Returns sorted vector.
[[nodiscard]] Result<std::vector<Path>> list_directory(const Path& dir);

/// List directory entries recursively. Returns sorted vector.
[[nodiscard]] Result<std::vector<Path>> list_directory_recursive(
    const Path& dir);

/// Create directory and all parents. No-op if exists.
[[nodiscard]] Status create_directories(const Path& path);

/// Remove file or empty directory.
[[nodiscard]] Status remove_path(const Path& path);

/// Remove directory and all contents.
[[nodiscard]] Status remove_all(const Path& path);

/// Copy file. Overwrites if exists.
[[nodiscard]] Status copy_file(const Path& from, const Path& to);

/// Get file size in bytes.
[[nodiscard]] Result<std::uintmax_t> file_size(const Path& path);

/// Get last modification time as epoch seconds.
[[nodiscard]] Result<std::int64_t> last_modified(const Path& path);

}  // namespace nift::core
