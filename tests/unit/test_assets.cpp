/// @file test_assets.cpp

#include <catch2/catch_test_macros.hpp>

#include "nift/server/assets.hpp"

using namespace nift::server::assets;

TEST_CASE("detect_image_format: PNG", "[server][assets]") {
  std::string png = "\x89PNG\r\n\x1A\n more data";
  CHECK(detect_image_format(png) == ImageFormat::PNG);
}

TEST_CASE("detect_image_format: JPEG", "[server][assets]") {
  std::string jpg = "\xFF\xD8\xFF\xE0";
  CHECK(detect_image_format(jpg) == ImageFormat::JPEG);
}

TEST_CASE("detect_image_format: WEBP", "[server][assets]") {
  std::string webp = "RIFF....WEBPVP8 ";
  CHECK(detect_image_format(webp) == ImageFormat::WEBP);
}

TEST_CASE("detect_image_format: GIF", "[server][assets]") {
  CHECK(detect_image_format("GIF89a") == ImageFormat::GIF);
  CHECK(detect_image_format("GIF87a") == ImageFormat::GIF);
}

TEST_CASE("detect_image_format: SVG", "[server][assets]") {
  CHECK(detect_image_format("<svg xmlns='...'>") == ImageFormat::SVG);
  CHECK(detect_image_format("  \n  <svg>") == ImageFormat::SVG);
}

TEST_CASE("detect_image_format: AVIF", "[server][assets]") {
  std::string avif(12, '\0');
  std::memcpy(avif.data() + 4, "ftypavif", 8);
  CHECK(detect_image_format(avif) == ImageFormat::AVIF);
}

TEST_CASE("detect_image_format: unknown", "[server][assets]") {
  CHECK(detect_image_format("plain text") == ImageFormat::Unknown);
  CHECK(detect_image_format("") == ImageFormat::Unknown);
}

TEST_CASE("image_extension", "[server][assets]") {
  CHECK(image_extension(ImageFormat::PNG) == "png");
  CHECK(image_extension(ImageFormat::JPEG) == "jpg");
  CHECK(image_extension(ImageFormat::WEBP) == "webp");
  CHECK(image_extension(ImageFormat::Unknown) == "");
}

TEST_CASE("minify_css: strips comments", "[server][assets]") {
  auto out = minify_css("/* comment */ body { color: red; }");
  CHECK(out.find("/*") == std::string::npos);
  CHECK(out.find("comment") == std::string::npos);
  CHECK(out.find("body") != std::string::npos);
}

TEST_CASE("minify_css: collapses whitespace", "[server][assets]") {
  auto out = minify_css("body  {\n  color:   red;\n  margin: 0;\n}");
  CHECK(out == "body{color:red;margin:0}");
}

TEST_CASE("minify_css: drops trailing semicolon before brace",
          "[server][assets]") {
  auto out = minify_css("a { color: red; }");
  CHECK(out == "a{color:red}");
}

TEST_CASE("minify_css: empty input", "[server][assets]") {
  CHECK(minify_css("") == "");
  CHECK(minify_css("   \n\t  ") == "");
}

TEST_CASE("minify_js: strips line comments", "[server][assets]") {
  auto out = minify_js("var x = 1; // hello\nvar y = 2;");
  CHECK(out.find("hello") == std::string::npos);
  CHECK(out.find("var x") != std::string::npos);
  CHECK(out.find("var y") != std::string::npos);
}

TEST_CASE("minify_js: strips block comments", "[server][assets]") {
  auto out = minify_js("/* big comment */ function f(){}");
  CHECK(out.find("/*") == std::string::npos);
  CHECK(out.find("function f") != std::string::npos);
}

TEST_CASE("minify_js: preserves string literals", "[server][assets]") {
  auto out = minify_js(R"(var s = "hello // not a comment"; )");
  CHECK(out.find("hello // not a comment") != std::string::npos);
}

TEST_CASE("minify_js: preserves single quotes", "[server][assets]") {
  auto out = minify_js(R"(var s = 'a /* b */ c';)");
  CHECK(out.find("/* b */") != std::string::npos);
}

TEST_CASE("minify_js: preserves template literals", "[server][assets]") {
  auto out = minify_js("var s = `tpl // x`;");
  CHECK(out.find("tpl // x") != std::string::npos);
}

TEST_CASE("minify_js: handles escaped quotes", "[server][assets]") {
  auto out = minify_js(R"(var s = "a\"b"; var t = 1;)");
  CHECK(out.find(R"(a\"b)") != std::string::npos);
}

TEST_CASE("minify_js: collapses whitespace", "[server][assets]") {
  auto out = minify_js("var   x   =   1;");
  CHECK(out == "var x = 1;");
}
