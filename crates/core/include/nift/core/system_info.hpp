#pragma once
/// @file system_info.hpp
/// @brief System/platform detection utilities.

#include <string>

namespace nift::core {

/// Detected operating system.
enum class OS { linux_os, macos, windows, freebsd, unknown };

/// Get runtime OS.
[[nodiscard]] constexpr OS detect_os() noexcept {
#if defined(__linux__)
  return OS::linux_os;
#elif defined(__APPLE__)
  return OS::macos;
#elif defined(_WIN32)
  return OS::windows;
#elif defined(__FreeBSD__)
  return OS::freebsd;
#else
  return OS::unknown;
#endif
}

/// OS as string.
[[nodiscard]] constexpr const char* os_name() noexcept {
  switch (detect_os()) {
    case OS::linux_os: return "linux";
    case OS::macos:    return "macos";
    case OS::windows:  return "windows";
    case OS::freebsd:  return "freebsd";
    case OS::unknown:  return "unknown";
  }
  return "unknown";
}

/// Get hostname.
[[nodiscard]] std::string hostname();

/// Get number of hardware threads.
[[nodiscard]] unsigned int hardware_concurrency() noexcept;

}  // namespace nift::core
