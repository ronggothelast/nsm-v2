#pragma once
// ─── nift/markdown/front_matter.hpp ──────────────────────────────────
// Extract and parse YAML/JSON front matter from the top of Markdown files.
//
// Supported formats:
//   ---           (YAML front matter)
//   key: value
//   ---
//
//   {             (JSON front matter)
//     "key": "value"
//   }
//
// Returns: (metadata as nlohmann::json, remaining body text).

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>

namespace nift::markdown {

struct ParsedDocument {
  nlohmann::json front_matter;  // {} if no front matter found
  std::string body;             // Markdown content after front matter
  bool has_front_matter = false;
};

// Parse a document that may contain front matter at the top.
// Detects `---` (YAML) or `{` (JSON) delimiters.
// YAML front matter is parsed as a simple key: value format
// (not a full YAML parser — uses nlohmann::json for structured data).
[[nodiscard]] ParsedDocument parse_front_matter(std::string_view source);

// Parse a simple YAML-like front matter block into JSON.
// Supports:
//   key: value          → {"key": "value"}
//   key: 123            → {"key": 123}
//   key: true           → {"key": true}
//   key: [a, b, c]      → {"key": ["a", "b", "c"]}
//   key:                → {"key": ""}
//     nested: value     → {"key": {"nested": "value"}}
[[nodiscard]] nlohmann::json parse_yaml_simple(std::string_view yaml);

// Strip front matter from source, returning only the body.
[[nodiscard]] std::string strip_front_matter(std::string_view source);

}  // namespace nift::markdown
