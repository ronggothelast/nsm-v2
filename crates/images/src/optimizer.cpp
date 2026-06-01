// ─── nift/images/optimizer.cpp ───────────────────────────────────────
// Image optimization: resize + format conversion via external tools.

#include "nift/images/optimizer.hpp"

#include <fmt/format.h>

#include <array>
#include <chrono>
#include <fstream>
#include <regex>
#include <sstream>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace nift::images {

namespace {

std::string exec_cmd(const std::string& cmd) {
  std::string result;
  std::array<char, 4096> buffer;
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe)
    return result;
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
    result += buffer.data();
  }
  pclose(pipe);
  return result;
}

bool command_exists(const std::string& cmd) {
#ifdef _WIN32
  auto out = exec_cmd("where " + cmd + " 2>NUL");
#else
  auto out = exec_cmd("which " + cmd + " 2>/dev/null");
#endif
  return !out.empty();
}

bool file_exists(const fs::path& p) {
  return fs::exists(p) && fs::is_regular_file(p);
}

size_t get_file_size(const fs::path& p) {
  return fs::exists(p) ? static_cast<size_t>(fs::file_size(p)) : 0;
}

// Parse image dimensions from `identify` or `file` output.
// Returns {width, height} or {0, 0} on failure.
std::pair<int, int> get_dimensions(const fs::path& path) {
  // Try ImageMagick identify.
  if (command_exists("identify")) {
    auto out = exec_cmd(
        fmt::format("identify -format '%wx%h' '{}' 2>/dev/null", path.string()));
    // Parse "WIDTHxHEIGHT"
    std::regex dim_re(R"((\d+)x(\d+))");
    std::smatch m;
    if (std::regex_search(out, m, dim_re)) {
      return {std::stoi(m[1]), std::stoi(m[2])};
    }
  }

  // Try ffprobe for video/gif.
  if (command_exists("ffprobe")) {
    auto out =
        exec_cmd(fmt::format("ffprobe -v error -select_streams v:0 "
                             "-show_entries stream=width,height "
                             "-of csv=p=0 '{}' 2>/dev/null",
                             path.string()));
    std::regex dim_re(R"((\d+),(\d+))");
    std::smatch m;
    if (std::regex_search(out, m, dim_re)) {
      return {std::stoi(m[1]), std::stoi(m[2])};
    }
  }

  return {0, 0};
}

}  // namespace

std::vector<Breakpoint> default_breakpoints() {
  return {
      {320, "320w"},   {640, "640w"},   {768, "768w"},   {1024, "1024w"},
      {1280, "1280w"}, {1536, "1536w"}, {1920, "1920w"},
  };
}

std::string find_tool(const std::string& tool_name) {
  if (command_exists(tool_name))
    return tool_name;
  // Check common alternative locations.
  if (tool_name == "convert" && command_exists("magick"))
    return "magick";
  return "";
}

bool has_vipsthumbnail() {
  return command_exists("vipsthumbnail");
}
bool has_convert() {
  return command_exists("convert") || command_exists("magick");
}
bool has_cwebp() {
  return command_exists("cwebp");
}
bool has_avifenc() {
  return command_exists("avifenc");
}

std::string format_ext(Format fmt) {
  switch (fmt) {
    case Format::WebP:
      return ".webp";
    case Format::AVIF:
      return ".avif";
    case Format::Png:
      return ".png";
    case Format::Jpeg:
      return ".jpg";
    case Format::Original:
    default:
      return "";
  }
}

ImageResult resize(const fs::path& input, const fs::path& output,
                   const ResizeOptions& opts) {
  auto start = std::chrono::steady_clock::now();
  ImageResult result;

  if (!file_exists(input)) {
    result.error_message = "Input not found: " + input.string();
    return result;
  }

  result.input_size = get_file_size(input);
  fs::create_directories(output.parent_path());

  // Determine output format.
  auto out_path = output;
  if (opts.format != Format::Original) {
    out_path.replace_extension(format_ext(opts.format));
  }

  // Try vipsthumbnail first (fastest).
  if (has_vipsthumbnail()) {
    std::string size_str;
    if (opts.width > 0 && opts.height > 0) {
      size_str = fmt::format("{}x{}", opts.width, opts.height);
      if (opts.maintain_aspect)
        size_str += ">";
    } else if (opts.width > 0) {
      size_str = fmt::format("{}x", opts.width);
    } else if (opts.height > 0) {
      size_str = fmt::format("x{}", opts.height);
    } else {
      size_str = "100%";  // No resize, just convert.
    }

    std::string cmd =
        fmt::format("vipsthumbnail '{}' -s {} -o '{}' --Q={}", input.string(), size_str,
                    out_path.string(), opts.quality);

    if (opts.strip_metadata) {
      cmd += " --strip";
    }

    exec_cmd(cmd + " 2>&1");
  }
  // Try ImageMagick convert.
  else if (has_convert()) {
    std::string convert = find_tool("convert");
    std::string cmd = convert + " '" + input.string() + "'";

    if (opts.width > 0 || opts.height > 0) {
      cmd += fmt::format(" -resize {}x{}", opts.width, opts.height);
      if (opts.maintain_aspect)
        cmd += ">";
    }

    cmd += fmt::format(" -quality {}", opts.quality);

    if (opts.strip_metadata) {
      cmd += " -strip";
    }

    cmd += " '" + out_path.string() + "'";
    exec_cmd(cmd + " 2>&1");
  }
  // Fallback: copy file (no processing available).
  else {
    fs::copy_file(input, out_path, fs::copy_options::overwrite_existing);
  }

  auto end = std::chrono::steady_clock::now();
  result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

  if (file_exists(out_path)) {
    result.success = true;
    result.output_path = out_path.string();
    result.output_size = get_file_size(out_path);
    auto [w, h] = get_dimensions(out_path);
    result.width = w;
    result.height = h;
  } else {
    result.error_message = "Resize failed";
  }

  return result;
}

ImageResult to_webp(const fs::path& input, const fs::path& output, int quality) {
  auto start = std::chrono::steady_clock::now();
  ImageResult result;

  if (!file_exists(input)) {
    result.error_message = "Input not found: " + input.string();
    return result;
  }

  result.input_size = get_file_size(input);
  fs::create_directories(output.parent_path());

  auto out_path = output;
  out_path.replace_extension(".webp");

  if (has_cwebp()) {
    std::string cmd = fmt::format("cwebp -q {} '{}' -o '{}' 2>&1", quality,
                                  input.string(), out_path.string());
    exec_cmd(cmd);
  } else if (has_convert()) {
    std::string convert = find_tool("convert");
    std::string cmd = fmt::format("{} '{}' -quality {} '{}'", convert, input.string(),
                                  quality, out_path.string());
    exec_cmd(cmd);
  } else {
    result.error_message = "No WebP encoder found (install cwebp or ImageMagick)";
    return result;
  }

  auto end = std::chrono::steady_clock::now();
  result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

  if (file_exists(out_path)) {
    result.success = true;
    result.output_path = out_path.string();
    result.output_size = get_file_size(out_path);
    auto [w, h] = get_dimensions(out_path);
    result.width = w;
    result.height = h;
  } else {
    result.error_message = "WebP conversion failed";
  }

  return result;
}

ImageResult to_avif(const fs::path& input, const fs::path& output, int quality) {
  auto start = std::chrono::steady_clock::now();
  ImageResult result;

  if (!file_exists(input)) {
    result.error_message = "Input not found: " + input.string();
    return result;
  }

  result.input_size = get_file_size(input);
  fs::create_directories(output.parent_path());

  auto out_path = output;
  out_path.replace_extension(".avif");

  if (has_avifenc()) {
    // avifenc uses speed (0-10, lower = better quality) not quality directly.
    int speed = 10 - (quality / 10);  // Map 0-100 → 10-0
    speed = std::clamp(speed, 0, 10);

    std::string cmd = fmt::format("avifenc --speed {} '{}' '{}' 2>&1", speed,
                                  input.string(), out_path.string());
    exec_cmd(cmd);
  } else if (has_convert()) {
    std::string convert = find_tool("convert");
    std::string cmd = fmt::format("{} '{}' -quality {} '{}'", convert, input.string(),
                                  quality, out_path.string());
    exec_cmd(cmd);
  } else {
    result.error_message = "No AVIF encoder found (install avifenc or ImageMagick)";
    return result;
  }

  auto end = std::chrono::steady_clock::now();
  result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

  if (file_exists(out_path)) {
    result.success = true;
    result.output_path = out_path.string();
    result.output_size = get_file_size(out_path);
    auto [w, h] = get_dimensions(out_path);
    result.width = w;
    result.height = h;
  } else {
    result.error_message = "AVIF conversion failed";
  }

  return result;
}

ResponsiveResult generate_responsive(const fs::path& input, const fs::path& output_dir,
                                     const std::vector<Breakpoint>& breakpoints,
                                     Format format, int quality) {
  ResponsiveResult result;
  fs::create_directories(output_dir);

  auto stem = input.stem().string();
  auto ext = format_ext(format);
  if (ext.empty())
    ext = input.extension().string();

  std::vector<std::string> srcset_parts;

  for (const auto& bp : breakpoints) {
    auto out_name = fmt::format("{}-{}{}", stem, bp.suffix, ext);
    auto out_path = output_dir / out_name;

    ResizeOptions opts;
    opts.width = bp.width;
    opts.quality = quality;
    opts.format = format;

    auto img_result = resize(input, out_path, opts);

    if (img_result.success) {
      result.images.push_back(img_result);
      srcset_parts.push_back(fmt::format("{} {}w", out_name, bp.width));
    }
  }

  // Build srcset attribute.
  if (!srcset_parts.empty()) {
    for (size_t i = 0; i < srcset_parts.size(); ++i) {
      if (i > 0)
        result.srcset_attribute += ", ";
      result.srcset_attribute += srcset_parts[i];
    }
    result.sizes_attribute = "(max-width: 640px) 100vw, (max-width: 1024px) 50vw, 33vw";
  }

  return result;
}

std::vector<ImageResult> run_image_pipeline(const ImagePipelineConfig& cfg) {
  std::vector<ImageResult> results;

  if (!fs::exists(cfg.input_dir))
    return results;
  fs::create_directories(cfg.output_dir);

  for (auto& entry : fs::recursive_directory_iterator(cfg.input_dir)) {
    if (!entry.is_regular_file())
      continue;

    auto ext = entry.path().extension().string();
    // Convert to lowercase.
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    bool is_image = false;
    for (const auto& e : cfg.extensions) {
      if (ext == e) {
        is_image = true;
        break;
      }
    }
    if (!is_image)
      continue;

    auto rel_path = fs::relative(entry.path(), cfg.input_dir);
    auto out_base = cfg.output_dir / rel_path.parent_path() / entry.path().stem();

    // Resize.
    auto resize_result =
        resize(entry.path(), out_base.string() + entry.path().extension().string(),
               cfg.resize_opts);
    results.push_back(resize_result);

    // WebP conversion.
    if (cfg.generate_webp && has_cwebp()) {
      auto webp_result =
          to_webp(entry.path(), out_base.string() + ".webp", cfg.resize_opts.quality);
      results.push_back(webp_result);
    }

    // AVIF conversion.
    if (cfg.generate_avif && has_avifenc()) {
      auto avif_result =
          to_avif(entry.path(), out_base.string() + ".avif", cfg.resize_opts.quality);
      results.push_back(avif_result);
    }

    // Responsive images.
    if (cfg.generate_responsive) {
      auto resp_dir = cfg.output_dir / "responsive" / rel_path.parent_path();
      auto resp = generate_responsive(entry.path(), resp_dir, cfg.breakpoints,
                                      Format::WebP, cfg.resize_opts.quality);
      for (auto& r : resp.images) {
        results.push_back(std::move(r));
      }
    }
  }

  return results;
}

std::string picture_tag(const fs::path& image_path, const std::string& alt,
                        const std::vector<Breakpoint>& breakpoints) {
  auto stem = image_path.stem().string();
  auto dir = image_path.parent_path();
  auto orig_ext = image_path.extension().string();

  std::string html = "<picture>\n";

  // AVIF source (best compression).
  if (has_avifenc()) {
    html += "  <source type=\"image/avif\" srcset=\"";
    for (size_t i = 0; i < breakpoints.size(); ++i) {
      if (i > 0)
        html += ", ";
      html += fmt::format("{}/{}-{}.avif {}w", dir.string(), stem,
                          breakpoints[i].suffix, breakpoints[i].width);
    }
    html += "\">\n";
  }

  // WebP source (good compression, wide support).
  if (has_cwebp()) {
    html += "  <source type=\"image/webp\" srcset=\"";
    for (size_t i = 0; i < breakpoints.size(); ++i) {
      if (i > 0)
        html += ", ";
      html += fmt::format("{}/{}-{}.webp {}w", dir.string(), stem,
                          breakpoints[i].suffix, breakpoints[i].width);
    }
    html += "\">\n";
  }

  // Fallback img tag.
  html +=
      fmt::format("  <img src=\"{}\" alt=\"{}\" loading=\"lazy\" decoding=\"async\">\n",
                  image_path.string(), alt);
  html += "</picture>";

  return html;
}

}  // namespace nift::images
