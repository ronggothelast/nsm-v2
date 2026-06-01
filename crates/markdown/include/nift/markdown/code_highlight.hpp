#pragma once
// ─── nift/markdown/code_highlight.hpp ────────────────────────────────
// Code block syntax highlighting for rendered HTML.
// Generates <pre><code class="language-X"> with CSS class-based tokens.
// No JavaScript dependency — works with any CSS highlight theme.
//
// Approach: Post-process the cmark-gfm HTML output to add <span> tags
// with CSS classes for keywords, strings, comments, numbers, etc.
// Language detection from ``` fences.

#include <string>
#include <string_view>
#include <vector>

namespace nift::markdown {

// Language identifier.
enum class Language {
  Unknown,
  Cpp,
  C,
  Python,
  JavaScript,
  TypeScript,
  Rust,
  Go,
  Bash,
  Json,
  Yaml,
  Html,
  Css,
  Lua,
  Diff,
};

// Parse language string from fence info string.
[[nodiscard]] Language detect_language(std::string_view info);

// Highlight a code string with CSS class spans.
// Returns HTML with <span class="hl-keyword">, <span class="hl-string">, etc.
[[nodiscard]] std::string highlight(std::string_view code, Language lang);

// Post-process rendered HTML: find <code class="language-X"> blocks
// and replace their contents with highlighted version.
[[nodiscard]] std::string highlight_html(std::string_view html);

// Generate a minimal CSS theme for code highlighting (dark OLED).
[[nodiscard]] std::string highlight_css();

}  // namespace nift::markdown
