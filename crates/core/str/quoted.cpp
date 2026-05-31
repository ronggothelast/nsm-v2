/// @file quoted.cpp
/// @brief Implementation of quoted string parsing.

#include "nift/core/quoted.hpp"

namespace nift::core {

std::optional<QuotedResult> parse_quoted(std::string_view sv) noexcept {
  if (sv.empty()) return std::nullopt;

  char quote = sv[0];
  if (quote != '"' && quote != '\'') return std::nullopt;

  std::string content;
  content.reserve(sv.size());

  std::size_t i = 1;
  while (i < sv.size()) {
    char c = sv[i];

    if (c == quote) {
      return QuotedResult{std::move(content), i + 1};
    }

    if (c == '\\' && i + 1 < sv.size()) {
      char next = sv[i + 1];
      switch (next) {
        case '\\': content += '\\'; break;
        case '"':  content += '"';  break;
        case '\'': content += '\''; break;
        case 'n':  content += '\n'; break;
        case 't':  content += '\t'; break;
        case 'r':  content += '\r'; break;
        case '0':  content += '\0'; break;
        default:   content += '\\'; content += next; break;
      }
      i += 2;
      continue;
    }

    content += c;
    ++i;
  }

  // Unterminated quote — return what we have.
  return std::nullopt;
}

std::string escape_string(std::string_view sv) {
  std::string result;
  result.reserve(sv.size() + sv.size() / 4);
  for (char c : sv) {
    switch (c) {
      case '\\': result += "\\\\"; break;
      case '"':  result += "\\\""; break;
      case '\n': result += "\\n";  break;
      case '\t': result += "\\t";  break;
      case '\r': result += "\\r";  break;
      case '\0': result += "\\0";  break;
      default:   result += c;      break;
    }
  }
  return result;
}

std::string_view unquote(std::string_view sv) noexcept {
  if (sv.size() >= 2) {
    char first = sv.front();
    char last = sv.back();
    if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
      return sv.substr(1, sv.size() - 2);
    }
  }
  return sv;
}

}  // namespace nift::core
