/// @file datetime.cpp
/// @brief Implementation of datetime utilities.

#include "nift/core/datetime.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace nift::core {

std::int64_t now_epoch_seconds() noexcept {
  auto now = std::chrono::system_clock::now();
  auto epoch =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
  return static_cast<std::int64_t>(epoch);
}

namespace {
std::string format_time(std::int64_t epoch_seconds, const char* fmt) {
  auto t = static_cast<std::time_t>(epoch_seconds);
  std::tm tm_buf{};
  // Cross-platform thread-safe gmtime: MSVC + MinGW expose gmtime_s,
  // POSIX exposes gmtime_r. Argument order differs between the two.
#if defined(_WIN32)
  gmtime_s(&tm_buf, &t);
#else
  gmtime_r(&t, &tm_buf);
#endif

  std::ostringstream oss;
  oss << std::put_time(&tm_buf, fmt);
  return oss.str();
}
}  // namespace

std::string format_iso8601(std::int64_t epoch_seconds) {
  return format_time(epoch_seconds, "%Y-%m-%dT%H:%M:%SZ");
}

std::string format_date(std::int64_t epoch_seconds) {
  return format_time(epoch_seconds, "%Y-%m-%d");
}

std::string format_datetime(std::int64_t epoch_seconds) {
  return format_time(epoch_seconds, "%Y-%m-%d %H:%M:%S");
}

}  // namespace nift::core
