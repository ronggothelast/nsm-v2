/// @file filesystem.cpp
/// @brief Implementation of nift::core filesystem utilities.

#include "nift/core/filesystem.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <system_error>

namespace nift::core {

namespace fs_std = std::filesystem;

Result<std::string> read_file(const Path& path) {
  std::error_code ec;
  if (!fs_std::exists(path.native(), ec)) {
    return unexpected(Error::not_found);
  }

  std::ifstream ifs(path.native(), std::ios::binary | std::ios::ate);
  if (!ifs) {
    return unexpected(Error::io_error);
  }

  auto size = ifs.tellg();
  ifs.seekg(0);

  std::string content;
  content.resize(static_cast<std::size_t>(size));
  ifs.read(content.data(), size);

  if (!ifs) {
    return unexpected(Error::io_error);
  }

  return content;
}

Status write_file(const Path& path, std::string_view content) {
  // Create parent dirs
  auto parent = path.parent();
  if (!parent.empty()) {
    std::error_code ec;
    fs_std::create_directories(parent.native(), ec);
    if (ec) {
      return unexpected(Error::io_error);
    }
  }

  std::ofstream ofs(path.native(), std::ios::binary | std::ios::trunc);
  if (!ofs) {
    return unexpected(Error::io_error);
  }

  ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
  if (!ofs) {
    return unexpected(Error::io_error);
  }

  return ok();
}

bool file_exists(const Path& path) noexcept {
  std::error_code ec;
  return fs_std::exists(path.native(), ec);
}

bool is_directory(const Path& path) noexcept {
  std::error_code ec;
  return fs_std::is_directory(path.native(), ec);
}

Result<std::vector<Path>> list_directory(const Path& dir) {
  std::error_code ec;
  if (!fs_std::is_directory(dir.native(), ec)) {
    return unexpected(Error::not_found);
  }

  std::vector<Path> entries;
  for (const auto& entry : fs_std::directory_iterator(dir.native(), ec)) {
    entries.emplace_back(entry.path());
  }
  if (ec) {
    return unexpected(Error::io_error);
  }

  std::sort(entries.begin(), entries.end());
  return entries;
}

Result<std::vector<Path>> list_directory_recursive(const Path& dir) {
  std::error_code ec;
  if (!fs_std::is_directory(dir.native(), ec)) {
    return unexpected(Error::not_found);
  }

  std::vector<Path> entries;
  for (const auto& entry :
       fs_std::recursive_directory_iterator(dir.native(), ec)) {
    entries.emplace_back(entry.path());
  }
  if (ec) {
    return unexpected(Error::io_error);
  }

  std::sort(entries.begin(), entries.end());
  return entries;
}

Status create_directories(const Path& path) {
  std::error_code ec;
  fs_std::create_directories(path.native(), ec);
  if (ec) {
    return unexpected(Error::io_error);
  }
  return ok();
}

Status remove_path(const Path& path) {
  std::error_code ec;
  fs_std::remove(path.native(), ec);
  if (ec) {
    return unexpected(Error::io_error);
  }
  return ok();
}

Status remove_all(const Path& path) {
  std::error_code ec;
  fs_std::remove_all(path.native(), ec);
  if (ec) {
    return unexpected(Error::io_error);
  }
  return ok();
}

Status copy_file(const Path& from, const Path& to) {
  std::error_code ec;
  // Create parent dirs for destination
  auto parent = to.parent();
  if (!parent.empty()) {
    fs_std::create_directories(parent.native(), ec);
    if (ec) {
      return unexpected(Error::io_error);
    }
  }

  fs_std::copy_file(from.native(), to.native(),
                    fs_std::copy_options::overwrite_existing, ec);
  if (ec) {
    return unexpected(Error::io_error);
  }
  return ok();
}

Result<std::uintmax_t> file_size(const Path& path) {
  std::error_code ec;
  auto sz = fs_std::file_size(path.native(), ec);
  if (ec) {
    return unexpected(Error::not_found);
  }
  return sz;
}

Result<std::int64_t> last_modified(const Path& path) {
  std::error_code ec;
  auto ftime = fs_std::last_write_time(path.native(), ec);
  if (ec) {
    return unexpected(Error::not_found);
  }
  // Convert to system_clock epoch seconds.
  auto sctp =
      std::chrono::time_point_cast<std::chrono::seconds>(
          std::chrono::file_clock::to_sys(ftime));
  return sctp.time_since_epoch().count();
}

}  // namespace nift::core
