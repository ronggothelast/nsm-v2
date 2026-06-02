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
// YAML features supported:
//   - Simple key: value with type inference (bool, int, float, null, string)
//   - Indentation-based nesting (2-space)
//   - Multi-line strings: literal (|) and folded (>) with chomping indicators
//   - Block sequences (- item)
//   - Inline flow mappings ({key: value})
//   - Inline flow sequences ([a, b, c])
//   - Comments (#)
//   - Quoted strings (single/double with escape sequences)
//   - YAML 1.1 boolean aliases (yes/no, on/off)
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
[[nodiscard]] ParsedDocument parse_front_matter(std::string_view source);

// Parse a YAML front matter block into JSON.
// Supports full YAML front matter syntax including:
//   key: value          → {"key": "value"}
//   key: 123            → {"key": 123}
//   key: true           → {"key": true}
//   key: [a, b, c]      → {"key": ["a", "b", "c"]}
//   key:                → {"key": ""}
//     nested: value     → {"key": {"nested": "value"}}
//   key: |              → {"key": "multi\nline\n"}
//     multi
//     line
//   key: >              → {"key": "folded line\n"}
//     folded
//     line
//   - item1             → ["item1", "item2"]
//   - item2
[[nodiscard]] nlohmann::json parse_yaml_simple(std::string_view yaml);

// Strip front matter from source, returning only the body.
[[nodiscard]] std::string strip_front_matter(std::string_view source);

}  // namespace nift::markdown
