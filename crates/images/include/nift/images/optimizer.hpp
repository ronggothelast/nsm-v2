#pragma once
// ─── nift/images/optimizer.hpp ───────────────────────────────────────
// Image optimization: resize, format conversion (WebP/AVIF), srcset generation.
// Uses external tools via subprocess (cwebp, avifenc, convert/vipsthumbnail).

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nift::images {

namespace fs = std::filesystem;

// Image processing result.
struct ImageResult {
  bool success = false;
  std::string output_path;
  std::string error_message;
  size_t input_size = 0;
  size_t output_size = 0;
  int width = 0;
  int height = 0;
  double duration_ms = 0.0;
};

// Image format.
enum class Format {
  Original,  // Keep original format
  WebP,
  AVIF,
  Png,
  Jpeg,
};

// Resize options.
struct ResizeOptions {
  int width = 0;   // Target width (0 = auto from aspect ratio)
  int height = 0;  // Target height (0 = auto from aspect ratio)
  bool maintain_aspect = true;
  int quality = 80;  // 1-100
  Format format = Format::Original;
  bool strip_metadata = true;  // Remove EXIF/IPTC
};

// Responsive image breakpoint.
struct Breakpoint {
  int width;
  std::string suffix;  // e.g., "640w", "1024w"
};

// Default responsive breakpoints.
[[nodiscard]] std::vector<Breakpoint> default_breakpoints();

// Find available image processing tool.
// Checks: vipsthumbnail, convert (ImageMagick), cwebp, avifenc.
[[nodiscard]] std::string find_tool(const std::string& tool_name);

// Check tool availability.
[[nodiscard]] bool has_vipsthumbnail();
[[nodiscard]] bool has_convert();
[[nodiscard]] bool has_cwebp();
[[nodiscard]] bool has_avifenc();

// Convert format enum to extension string.
[[nodiscard]] std::string format_ext(Format fmt);

// Resize an image.
[[nodiscard]] ImageResult resize(const fs::path& input, const fs::path& output,
                                 const ResizeOptions& opts);

// Convert image to WebP format.
[[nodiscard]] ImageResult to_webp(const fs::path& input, const fs::path& output,
                                  int quality = 80);

// Convert image to AVIF format.
[[nodiscard]] ImageResult to_avif(const fs::path& input, const fs::path& output,
                                  int quality = 80);

// Generate responsive images: multiple sizes + formats.
struct ResponsiveResult {
  std::vector<ImageResult> images;
  std::string srcset_attribute;  // Ready-to-use srcset string
  std::string sizes_attribute;   // Ready-to-use sizes string
};
[[nodiscard]] ResponsiveResult generate_responsive(
    const fs::path& input, const fs::path& output_dir,
    const std::vector<Breakpoint>& breakpoints = default_breakpoints(),
    Format format = Format::WebP, int quality = 80);

// Process all images in a directory.
struct ImagePipelineConfig {
  fs::path input_dir;
  fs::path output_dir;
  ResizeOptions resize_opts;
  std::vector<std::string> extensions = {".jpg", ".jpeg", ".png",
                                         ".gif", ".bmp",  ".tiff"};
  bool generate_webp = true;
  bool generate_avif = false;
  bool generate_responsive = false;
  std::vector<Breakpoint> breakpoints = default_breakpoints();
};
[[nodiscard]] std::vector<ImageResult> run_image_pipeline(
    const ImagePipelineConfig& cfg);

// Generate HTML <picture> element with multiple sources.
[[nodiscard]] std::string picture_tag(
    const fs::path& image_path, const std::string& alt,
    const std::vector<Breakpoint>& breakpoints = default_breakpoints());

}  // namespace nift::images
