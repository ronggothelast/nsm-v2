#pragma once
/// @file datetime.hpp
/// @brief Date/time utilities for nift — timestamps, formatting.

#include <chrono>
#include <string>

namespace nift::core {

/// Get current time as epoch seconds.
[[nodiscard]] std::int64_t now_epoch_seconds() noexcept;

/// Format epoch seconds as ISO 8601 string (YYYY-MM-DDTHH:MM:SSZ).
[[nodiscard]] std::string format_iso8601(std::int64_t epoch_seconds);

/// Format epoch seconds as human-readable date (YYYY-MM-DD).
[[nodiscard]] std::string format_date(std::int64_t epoch_seconds);

/// Format epoch seconds as human-readable datetime (YYYY-MM-DD HH:MM:SS).
[[nodiscard]] std::string format_datetime(std::int64_t epoch_seconds);

}  // namespace nift::core
