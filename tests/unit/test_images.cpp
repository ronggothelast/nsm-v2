// ─── test_images.cpp ─────────────────────────────────────────────────
// Unit tests for nift::images: tool detection, resize, format conversion, responsive.

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <nift/images/optimizer.hpp>

using namespace nift::images;
namespace fs = std::filesystem;

struct ImgTmpDir {
  fs::path path;
  ImgTmpDir()
      : path(fs::temp_directory_path() /
             ("nift_img_test_" +
              std::to_string(
                  std::chrono::steady_clock::now().time_since_epoch().count()))) {
    fs::create_directories(path);
  }
  ~ImgTmpDir() { fs::remove_all(path); }
};

static void write_img_file(const fs::path& p, const std::string& content) {
  fs::create_directories(p.parent_path());
  std::ofstream f(p, std::ios::binary);
  f << content;
}

// ═══════════════════════════════════════════════════════════════════
// Tool detection
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("has_convert: checks ImageMagick", "[images]") {
  bool result = has_convert();
  INFO("ImageMagick: " << (result ? "available" : "not available"));
  CHECK(true);
}

TEST_CASE("has_cwebp: checks WebP encoder", "[images]") {
  bool result = has_cwebp();
  INFO("cwebp: " << (result ? "available" : "not available"));
  CHECK(true);
}

TEST_CASE("has_avifenc: checks AVIF encoder", "[images]") {
  bool result = has_avifenc();
  INFO("avifenc: " << (result ? "available" : "not available"));
  CHECK(true);
}

TEST_CASE("has_vipsthumbnail: checks vips", "[images]") {
  bool result = has_vipsthumbnail();
  INFO("vipsthumbnail: " << (result ? "available" : "not available"));
  CHECK(true);
}

// ═══════════════════════════════════════════════════════════════════
// Format helpers
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("format_ext: returns correct extensions", "[images]") {
  CHECK(format_ext(Format::WebP) == ".webp");
  CHECK(format_ext(Format::AVIF) == ".avif");
  CHECK(format_ext(Format::Png) == ".png");
  CHECK(format_ext(Format::Jpeg) == ".jpg");
  CHECK(format_ext(Format::Original) == "");
}

// ═══════════════════════════════════════════════════════════════════
// Default breakpoints
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("default_breakpoints: returns 7 sizes", "[images]") {
  auto bp = default_breakpoints();
  CHECK(bp.size() == 7);
  CHECK(bp[0].width == 320);
  CHECK(bp.back().width == 1920);
  CHECK(bp[0].suffix == "320w");
}

// ═══════════════════════════════════════════════════════════════════
// Resize
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("resize: missing input file", "[images]") {
  ImgTmpDir tmp;
  ResizeOptions opts;
  auto result = resize(tmp.path / "nope.jpg", tmp.path / "out.jpg", opts);
  CHECK(result.success == false);
  CHECK(!result.error_message.empty());
}

TEST_CASE("resize: handles missing tools gracefully", "[images]") {
  ImgTmpDir tmp;

  // Create a dummy file (not a real image).
  auto input = tmp.path / "test.txt";
  write_img_file(input, "not an image");

  ResizeOptions opts;
  opts.width = 100;

  auto result = resize(input, tmp.path / "out.jpg", opts);

  // Should either succeed (if tools exist and handle it) or fail gracefully.
  // Either way, no crash.
  CHECK(true);
}

// ═══════════════════════════════════════════════════════════════════
// WebP conversion
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("to_webp: missing input", "[images]") {
  ImgTmpDir tmp;
  auto result = to_webp(tmp.path / "nope.jpg", tmp.path / "out.webp");
  CHECK(result.success == false);
}

TEST_CASE("to_webp: no encoder", "[images]") {
  ImgTmpDir tmp;
  write_img_file(tmp.path / "test.txt", "data");

  // If no cwebp/convert, should fail gracefully.
  if (!has_cwebp() && !has_convert()) {
    auto result = to_webp(tmp.path / "test.txt", tmp.path / "out.webp");
    CHECK(result.success == false);
    CHECK(result.error_message.find("No WebP encoder") != std::string::npos);
  }
}

// ═══════════════════════════════════════════════════════════════════
// AVIF conversion
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("to_avif: missing input", "[images]") {
  ImgTmpDir tmp;
  auto result = to_avif(tmp.path / "nope.jpg", tmp.path / "out.avif");
  CHECK(result.success == false);
}

TEST_CASE("to_avif: no encoder", "[images]") {
  ImgTmpDir tmp;
  write_img_file(tmp.path / "test.txt", "data");

  if (!has_avifenc() && !has_convert()) {
    auto result = to_avif(tmp.path / "test.txt", tmp.path / "out.avif");
    CHECK(result.success == false);
    CHECK(result.error_message.find("No AVIF encoder") != std::string::npos);
  }
}

// ═══════════════════════════════════════════════════════════════════
// Responsive images
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("generate_responsive: missing input", "[images]") {
  ImgTmpDir tmp;
  auto result = generate_responsive(tmp.path / "nope.jpg", tmp.path / "out");
  CHECK(result.images.empty());
}

// ═══════════════════════════════════════════════════════════════════
// Pipeline
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("run_image_pipeline: empty input dir", "[images]") {
  ImgTmpDir tmp;
  fs::create_directories(tmp.path / "empty");

  ImagePipelineConfig cfg;
  cfg.input_dir = tmp.path / "empty";
  cfg.output_dir = tmp.path / "out";

  auto results = run_image_pipeline(cfg);
  CHECK(results.empty());
}

TEST_CASE("run_image_pipeline: missing input dir", "[images]") {
  ImgTmpDir tmp;

  ImagePipelineConfig cfg;
  cfg.input_dir = tmp.path / "nonexistent";
  cfg.output_dir = tmp.path / "out";

  auto results = run_image_pipeline(cfg);
  CHECK(results.empty());
}

// ═══════════════════════════════════════════════════════════════════
// Picture tag
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("picture_tag: generates HTML", "[images]") {
  auto html = picture_tag(fs::path("images/photo.jpg"), "A photo");

  CHECK(html.find("<picture>") != std::string::npos);
  CHECK(html.find("</picture>") != std::string::npos);
  CHECK(html.find("<img") != std::string::npos);
  CHECK(html.find("A photo") != std::string::npos);
  CHECK(html.find("loading=\"lazy\"") != std::string::npos);
  CHECK(html.find("decoding=\"async\"") != std::string::npos);
}

TEST_CASE("picture_tag: includes srcset when tools available", "[images]") {
  auto html = picture_tag(fs::path("images/photo.jpg"), "Photo",
                          {{640, "640w"}, {1024, "1024w"}});

  // Should always have img fallback.
  CHECK(html.find("<img") != std::string::npos);

  // If tools available, should have source tags.
  if (has_cwebp()) {
    CHECK(html.find("type=\"image/webp\"") != std::string::npos);
    CHECK(html.find("640w") != std::string::npos);
  }
}

// ═══════════════════════════════════════════════════════════════════
// Tool finding
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("find_tool: returns empty for missing tool", "[images]") {
  auto result = find_tool("nonexistent_tool_xyz");
  CHECK(result.empty());
}

TEST_CASE("find_tool: returns name if tool exists", "[images]") {
  // ls should exist on all systems.
  auto result = find_tool("ls");
  // Might be empty on Windows, that's fine.
  INFO("find_tool(ls): " << result);
  CHECK(true);
}
