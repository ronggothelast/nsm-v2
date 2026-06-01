#pragma once
// ─── nift/markdown/markdown.hpp ──────────────────────────────────────
// Public API: convert Markdown (GFM) → HTML via cmark-gfm.

#include <cmark-gfm.h>

#include <string>
#include <string_view>
#include <vector>

namespace nift::markdown {

// Options for GFM rendering.
struct MarkdownOptions {
  bool hardbreaks = false;    // Treat newlines as <br>
  bool unsafe = false;        // Allow raw HTML in input
  bool smart = false;         // Smart quotes, dashes, ellipses
  bool strikethrough = true;  // ~~text~~ → <del>text</del>
  bool autolink = true;       // Auto-detect URLs
  bool table = true;          // Pipe tables
  bool tagfilter = true;      // Filter dangerous HTML tags
  int heading_max = 6;        // Max heading level (1-6)
};

// Convert a Markdown string to HTML.
// Throws std::runtime_error on cmark internal failure.
[[nodiscard]] std::string to_html(std::string_view markdown,
                                  const MarkdownOptions& opts = {});

// Convert a Markdown string to plain text (strip all markup).
[[nodiscard]] std::string to_plaintext(std::string_view markdown);

// Extract all headings as a table-of-contents structure.
struct TocEntry {
  int level;         // 1-6
  std::string text;  // Heading text (plain)
  std::string id;    // HTML id attribute
};
[[nodiscard]] std::vector<TocEntry> extract_toc(std::string_view markdown);

// Extract all links from markdown.
struct LinkInfo {
  std::string text;
  std::string url;
  bool is_image = false;
};
[[nodiscard]] std::vector<LinkInfo> extract_links(std::string_view markdown);

}  // namespace nift::markdown
