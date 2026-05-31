/// @file str_fns.cpp
/// @brief Implementation of string utility functions.

#include "nift/core/str_fns.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace nift::core {

std::string_view trim(std::string_view sv) noexcept {
  return trim_right(trim_left(sv));
}

std::string_view trim_left(std::string_view sv) noexcept {
  while (!sv.empty() &&
         std::isspace(static_cast<unsigned char>(sv.front()))) {
    sv.remove_prefix(1);
  }
  return sv;
}

std::string_view trim_right(std::string_view sv) noexcept {
  while (!sv.empty() &&
         std::isspace(static_cast<unsigned char>(sv.back()))) {
    sv.remove_suffix(1);
  }
  return sv;
}

std::vector<std::string_view> split(std::string_view sv, char delim) {
  std::vector<std::string_view> result;
  while (!sv.empty()) {
    auto pos = sv.find(delim);
    if (pos == std::string_view::npos) {
      result.push_back(sv);
      break;
    }
    result.push_back(sv.substr(0, pos));
    sv.remove_prefix(pos + 1);
  }
  return result;
}

std::vector<std::string_view> split(std::string_view sv,
                                     std::string_view delim) {
  std::vector<std::string_view> result;
  if (delim.empty()) {
    result.push_back(sv);
    return result;
  }
  while (!sv.empty()) {
    auto pos = sv.find(delim);
    if (pos == std::string_view::npos) {
      result.push_back(sv);
      break;
    }
    result.push_back(sv.substr(0, pos));
    sv.remove_prefix(pos + delim.size());
  }
  return result;
}

std::string join(const std::vector<std::string_view>& parts,
                 std::string_view sep) {
  if (parts.empty()) return {};

  std::string result;
  std::size_t total = 0;
  for (const auto& p : parts) total += p.size();
  total += sep.size() * (parts.size() - 1);
  result.reserve(total);

  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) result.append(sep);
    result.append(parts[i]);
  }
  return result;
}

std::string replace_all(std::string_view str, std::string_view from,
                         std::string_view to) {
  if (from.empty()) return std::string(str);

  std::string result;
  result.reserve(str.size());

  std::size_t pos = 0;
  while (pos < str.size()) {
    auto found = str.find(from, pos);
    if (found == std::string_view::npos) {
      result.append(str.substr(pos));
      break;
    }
    result.append(str.substr(pos, found - pos));
    result.append(to);
    pos = found + from.size();
  }
  return result;
}

bool starts_with(std::string_view str, std::string_view prefix) noexcept {
  if (prefix.size() > str.size()) return false;
  return str.substr(0, prefix.size()) == prefix;
}

bool ends_with(std::string_view str, std::string_view suffix) noexcept {
  if (suffix.size() > str.size()) return false;
  return str.substr(str.size() - suffix.size()) == suffix;
}

bool contains(std::string_view str, std::string_view sub) noexcept {
  return str.find(sub) != std::string_view::npos;
}

std::string to_lower(std::string_view sv) {
  std::string result(sv);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

std::string to_upper(std::string_view sv) {
  std::string result(sv);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}

}  // namespace nift::core
