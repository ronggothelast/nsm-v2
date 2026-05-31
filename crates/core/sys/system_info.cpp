/// @file system_info.cpp
/// @brief Implementation of system info utilities.

#include "nift/core/system_info.hpp"

#include <thread>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace nift::core {

std::string hostname() {
  char buf[256] = {};
#if defined(_WIN32)
  DWORD size = sizeof(buf);
  GetComputerNameA(buf, &size);
#else
  gethostname(buf, sizeof(buf));
#endif
  return std::string(buf);
}

unsigned int hardware_concurrency() noexcept {
  auto n = std::thread::hardware_concurrency();
  return n > 0 ? n : 1;
}

}  // namespace nift::core
