/// @file path.cpp
/// @brief Implementation of nift::core::Path.

#include "nift/core/path.hpp"

namespace nift::core {

std::string_view Path::filename() const {
  filename_cache_ = path_.filename().string();
  return filename_cache_;
}

std::string_view Path::stem() const {
  stem_cache_ = path_.stem().string();
  return stem_cache_;
}

std::string_view Path::extension() const {
  ext_cache_ = path_.extension().string();
  return ext_cache_;
}

Path Path::parent() const {
  return Path{path_.parent_path()};
}

Path Path::with_extension(std::string_view ext) const {
  auto copy = path_;
  copy.replace_extension(ext);
  return Path{std::move(copy)};
}

Path Path::join(std::string_view component) const {
  return Path{path_ / component};
}

Path Path::relative_to(const Path& base) const {
  return Path{fs::relative(path_, base.native())};
}

Path Path::normalized() const {
  return Path{path_.lexically_normal()};
}

}  // namespace nift::core
