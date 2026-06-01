/// @file assets.cpp

#include "nift/server/assets.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace nift::server::assets {

namespace {

bool starts_with(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() &&
         std::memcmp(s.data(), prefix.data(), prefix.size()) == 0;
}

bool starts_with_at(std::string_view s, std::size_t off, std::string_view p) {
  return s.size() >= off + p.size() &&
         std::memcmp(s.data() + off, p.data(), p.size()) == 0;
}

}  // namespace

ImageFormat detect_image_format(std::string_view buf) {
  if (buf.size() >= 8 && starts_with(buf, std::string_view("\x89PNG\r\n\x1A\n", 8))) {
    return ImageFormat::PNG;
  }
  if (buf.size() >= 3 && starts_with(buf, std::string_view("\xFF\xD8\xFF", 3))) {
    return ImageFormat::JPEG;
  }
  if (buf.size() >= 12 && starts_with(buf, "RIFF") && starts_with_at(buf, 8, "WEBP")) {
    return ImageFormat::WEBP;
  }
  if (buf.size() >= 6 && (starts_with(buf, "GIF87a") || starts_with(buf, "GIF89a"))) {
    return ImageFormat::GIF;
  }
  if (buf.size() >= 12 && starts_with_at(buf, 4, "ftyp") &&
      (starts_with_at(buf, 8, "avif") || starts_with_at(buf, 8, "avis"))) {
    return ImageFormat::AVIF;
  }
  // SVG is text — sniff the first non-whitespace bytes.
  std::size_t i = 0;
  while (i < buf.size() && std::isspace(static_cast<unsigned char>(buf[i])))
    ++i;
  if (i + 4 <= buf.size() &&
      (starts_with_at(buf, i, "<svg") || starts_with_at(buf, i, "<?xml"))) {
    // Try harder for <?xml ... <svg
    auto rest = buf.substr(i);
    if (starts_with(rest, "<svg"))
      return ImageFormat::SVG;
    if (rest.find("<svg") != std::string_view::npos)
      return ImageFormat::SVG;
  }
  return ImageFormat::Unknown;
}

std::string_view image_extension(ImageFormat fmt) noexcept {
  switch (fmt) {
    case ImageFormat::PNG:
      return "png";
    case ImageFormat::JPEG:
      return "jpg";
    case ImageFormat::WEBP:
      return "webp";
    case ImageFormat::GIF:
      return "gif";
    case ImageFormat::SVG:
      return "svg";
    case ImageFormat::AVIF:
      return "avif";
    case ImageFormat::Unknown:
      return "";
  }
  return "";
}

std::string minify_css(std::string_view css) {
  std::string out;
  out.reserve(css.size());

  std::size_t i = 0;
  bool prev_was_space = false;
  while (i < css.size()) {
    char c = css[i];

    // Strip /* ... */ comments
    if (c == '/' && i + 1 < css.size() && css[i + 1] == '*') {
      i += 2;
      while (i + 1 < css.size() && !(css[i] == '*' && css[i + 1] == '/'))
        ++i;
      if (i + 1 < css.size())
        i += 2;
      continue;
    }

    // Collapse whitespace
    if (std::isspace(static_cast<unsigned char>(c))) {
      if (!out.empty()) {
        char last = out.back();
        // Drop whitespace adjacent to structural chars.
        const char structural[] = "{};:,>+~()";
        if (std::strchr(structural, last) == nullptr) {
          if (!prev_was_space) {
            out += ' ';
            prev_was_space = true;
          }
        }
      }
      ++i;
      continue;
    }

    // Drop whitespace before structural chars too.
    if (!out.empty() && out.back() == ' ') {
      const char structural_after[] = "{};:,>+~)";
      if (std::strchr(structural_after, c) != nullptr) {
        out.pop_back();
      }
    }
    out += c;
    prev_was_space = false;
    ++i;
  }

  // Trim trailing whitespace.
  while (!out.empty() && (out.back() == ' ' || out.back() == '\n')) {
    out.pop_back();
  }
  // Drop final ; before }
  for (std::size_t k = 0; k + 1 < out.size(); ++k) {
    if (out[k] == ';' && out[k + 1] == '}') {
      out.erase(k, 1);
      if (k > 0)
        --k;
    }
  }
  return out;
}

std::string minify_js(std::string_view js) {
  std::string out;
  out.reserve(js.size());

  std::size_t i = 0;
  bool prev_was_space = false;
  while (i < js.size()) {
    char c = js[i];

    // Block comment
    if (c == '/' && i + 1 < js.size() && js[i + 1] == '*') {
      i += 2;
      while (i + 1 < js.size() && !(js[i] == '*' && js[i + 1] == '/'))
        ++i;
      if (i + 1 < js.size())
        i += 2;
      continue;
    }
    // Line comment
    if (c == '/' && i + 1 < js.size() && js[i + 1] == '/') {
      while (i < js.size() && js[i] != '\n')
        ++i;
      continue;
    }
    // String literal — copy verbatim.
    if (c == '"' || c == '\'' || c == '`') {
      char quote = c;
      out += c;
      ++i;
      while (i < js.size() && js[i] != quote) {
        if (js[i] == '\\' && i + 1 < js.size()) {
          out += js[i];
          out += js[i + 1];
          i += 2;
        } else {
          out += js[i++];
        }
      }
      if (i < js.size()) {
        out += js[i++];
      }
      prev_was_space = false;
      continue;
    }

    if (std::isspace(static_cast<unsigned char>(c))) {
      if (!out.empty() && !prev_was_space) {
        out += ' ';
        prev_was_space = true;
      }
      ++i;
      continue;
    }
    out += c;
    prev_was_space = false;
    ++i;
  }
  while (!out.empty() && out.back() == ' ')
    out.pop_back();
  return out;
}

}  // namespace nift::server::assets
