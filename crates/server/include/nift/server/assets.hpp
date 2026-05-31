/// @file assets.hpp
/// @brief Asset pipeline — CSS/JS minification, image classification.
///
/// Phase 5 scope:
///   - CSS minifier (whitespace + comment strip, simple but correct).
///   - JS minifier (placeholder: comment + whitespace strip).
///   - Image format detection (magic bytes).
///   - Tailwind v4 + libvips integration is deferred to Phase 6 along with
///     the CLI: the CLI shells out to `tailwindcss --minify` and `vips`
///     binaries which keeps Phase 5 dep-free.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace nift::server::assets {

/// @brief Detected image format via magic bytes.
enum class ImageFormat : std::uint8_t {
  Unknown,
  PNG,
  JPEG,
  WEBP,
  GIF,
  SVG,
  AVIF,
};

/// @brief Detect image format from buffer prefix. Reads up to 16 bytes.
ImageFormat detect_image_format(std::string_view buffer);

/// @brief Human-readable extension (no dot) for an image format.
std::string_view image_extension(ImageFormat fmt) noexcept;

/// @brief Minify CSS — strip comments, collapse whitespace, drop trailing `;`.
std::string minify_css(std::string_view css);

/// @brief Minify JS — strip line and block comments, collapse whitespace.
/// This is intentionally conservative: we do NOT attempt token-aware
/// whitespace removal (that's a Phase 6+ project). Safe for hand-written
/// site JS; not a replacement for Terser/esbuild.
std::string minify_js(std::string_view js);

}  // namespace nift::server::assets
