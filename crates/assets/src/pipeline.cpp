// ─── nift/assets/pipeline.cpp ────────────────────────────────────────
// Asset pipeline: CSS/JS processing via esbuild + lightningcss + BLAKE3.

#include "nift/assets/pipeline.hpp"

#include <blake3.h>
#include <fmt/format.h>

#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace nift::assets {

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

// Compute BLAKE3 hash of a file, return hex string.
std::string blake3_file(const fs::path& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f.is_open())
    return "";

  blake3_hasher hasher;
  blake3_hasher_init(&hasher);

  char buf[8192];
  while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
    blake3_hasher_update(&hasher, buf, static_cast<size_t>(f.gcount()));
  }

  uint8_t hash[BLAKE3_OUT_LEN];
  blake3_hasher_finalize(&hasher, hash, BLAKE3_OUT_LEN);

  std::string hex;
  hex.reserve(BLAKE3_OUT_LEN * 2);
  for (int i = 0; i < BLAKE3_OUT_LEN; ++i) {
    char h[3];
    snprintf(h, sizeof(h), "%02x", hash[i]);
    hex += h;
  }
  return hex;
}

// Read entire file to string.
std::string read_file(const fs::path& p) {
  std::ifstream f(p, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
}

// Get file extension without dot.

}  // namespace

std::string find_esbuild() {
  if (command_exists("esbuild"))
    return "esbuild";
  if (command_exists("npx")) {
    auto ver = exec_cmd("npx esbuild --version 2>/dev/null");
    if (!ver.empty() && ver.find(".") != std::string::npos)
      return "npx esbuild";
  }
  if (file_exists("node_modules/.bin/esbuild"))
    return "./node_modules/.bin/esbuild";
  return "";
}

std::string find_lightningcss() {
  if (command_exists("lightningcss"))
    return "lightningcss";
  if (command_exists("npx")) {
    auto ver = exec_cmd("npx lightningcss --version 2>/dev/null");
    if (!ver.empty())
      return "npx lightningcss";
  }
  if (file_exists("node_modules/.bin/lightningcss"))
    return "./node_modules/.bin/lightningcss";
  return "";
}

bool esbuild_available() {
  return !find_esbuild().empty();
}
bool lightningcss_available() {
  return !find_lightningcss().empty();
}

AssetResult process_css(const fs::path& input, const fs::path& output_dir,
                        const CssOptions& opts) {
  auto start = std::chrono::steady_clock::now();
  AssetResult result;

  if (!file_exists(input)) {
    result.error_message = "Input file not found: " + input.string();
    return result;
  }

  result.input_size = fs::file_size(input);

  // Determine output path.
  auto out_name = input.stem().string();
  if (opts.minify)
    out_name += ".min";
  out_name += ".css";
  auto output = output_dir / out_name;
  fs::create_directories(output_dir);

  // Try lightningcss first.
  std::string cli = find_lightningcss();
  if (!cli.empty()) {
    std::string cmd = cli + " " + input.string();
    cmd += " -o " + output.string();
    if (opts.minify)
      cmd += " --minify";
    if (opts.sourcemap)
      cmd += " --source-map";
    if (!opts.targets.empty())
      cmd += " --targets '" + opts.targets + "'";

    exec_cmd(cmd + " 2>&1");
  } else {
    // Fallback: just copy with basic minification (strip comments + whitespace).
    std::string css = read_file(input);

    // Remove comments.
    std::regex comment_re(R"(//.*?$|/\*[\s\S]*?\*/)", std::regex::multiline);
    css = std::regex_replace(css, comment_re, "");

    if (opts.minify) {
      // Collapse whitespace.
      std::regex ws_re(R"(\s+)");
      css = std::regex_replace(css, ws_re, " ");
      // Remove spaces around selectors/braces.
      std::regex sel_re(R"(\s*([{}:;,>+~])\s*)");
      css = std::regex_replace(css, sel_re, "$1");
      // Trim.
      while (!css.empty() && css.front() == ' ')
        css.erase(0, 1);
      while (!css.empty() && css.back() == ' ')
        css.pop_back();
    }

    std::ofstream out(output);
    out << css;
    // Append sourceMappingURL if sourcemaps enabled.
    if (opts.sourcemap) {
      out << "\n/*# sourceMappingURL=" << output.filename().string() << ".map */";
      // Write a basic sourcemap (single mapping to original).
      auto map_path = fs::path(output.string() + ".map");
      nlohmann::json sm;
      sm["version"] = 3;
      sm["file"] = output.filename().string();
      sm["sources"] = {input.filename().string()};
      sm["mappings"] = "AAAA";
      std::ofstream mf(map_path);
      mf << sm.dump(2) << "\n";
    }
    out.close();
  }

  auto end = std::chrono::steady_clock::now();
  result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

  if (file_exists(output)) {
    result.success = true;
    result.output_path = output.string();
    result.output_size = fs::file_size(output);
    result.content_hash = blake3_file(output);

    // Detect sourcemap file.
    auto map_path = fs::path(output.string() + ".map");
    if (file_exists(map_path))
      result.sourcemap_path = map_path.string();
  } else {
    result.error_message = "CSS processing failed";
  }

  return result;
}

AssetResult bundle_js(const std::vector<fs::path>& inputs, const fs::path& output_dir,
                      const JsOptions& opts) {
  auto start = std::chrono::steady_clock::now();
  AssetResult result;

  if (inputs.empty()) {
    result.error_message = "No input files";
    return result;
  }

  // Check inputs exist.
  size_t total_size = 0;
  for (const auto& inp : inputs) {
    if (!file_exists(inp)) {
      result.error_message = "Input not found: " + inp.string();
      return result;
    }
    total_size += fs::file_size(inp);
  }
  result.input_size = total_size;

  // Find esbuild.
  std::string cli = find_esbuild();
  if (cli.empty()) {
    result.error_message = "esbuild not found. Install: npm install -D esbuild";
    return result;
  }

  // Determine output.
  auto out_name = inputs[0].stem().string();
  if (opts.minify)
    out_name += ".min";
  if (opts.format == "esm")
    out_name += ".mjs";
  else if (opts.format == "cjs")
    out_name += ".cjs";
  else
    out_name += ".js";
  auto output = output_dir / out_name;
  fs::create_directories(output_dir);

  // Build command.
  std::string cmd = cli;
  for (const auto& inp : inputs) {
    cmd += " " + inp.string();
  }
  cmd += " --outfile=" + output.string();
  cmd += " --format=" + opts.format;
  cmd += " --target=" + opts.target;
  if (opts.bundle)
    cmd += " --bundle";
  if (opts.minify)
    cmd += " --minify";
  if (opts.sourcemap)
    cmd += " --sourcemap";

  for (const auto& ext : opts.external) {
    cmd += " --external:" + ext;
  }
  for (const auto& [key, val] : opts.define) {
    cmd += " --define:" + key + "=" + val;
  }

  exec_cmd(cmd + " 2>&1");

  auto end = std::chrono::steady_clock::now();
  result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

  if (file_exists(output)) {
    result.success = true;
    result.output_path = output.string();
    result.output_size = fs::file_size(output);
    result.content_hash = blake3_file(output);

    // Detect sourcemap file.
    auto map_path = fs::path(output.string() + ".map");
    if (file_exists(map_path))
      result.sourcemap_path = map_path.string();
  } else {
    result.error_message = "JS bundling failed";
  }

  return result;
}

AssetResult fingerprint(const fs::path& input, const fs::path& output_dir) {
  AssetResult result;

  if (!file_exists(input)) {
    result.error_message = "Input not found: " + input.string();
    return result;
  }

  result.input_size = fs::file_size(input);

  // Compute hash.
  std::string hash = blake3_file(input);
  if (hash.empty()) {
    result.error_message = "Failed to compute hash";
    return result;
  }

  // Create fingerprinted filename: name.HASH.ext
  auto stem = input.stem().string();
  auto ext = input.extension().string();
  auto fname = stem + "." + hash.substr(0, 8) + ext;
  auto output = output_dir / fname;

  fs::create_directories(output_dir);
  fs::copy_file(input, output, fs::copy_options::overwrite_existing);

  // Copy sourcemap file if it exists, with updated name.
  auto map_input = fs::path(input.string() + ".map");
  if (file_exists(map_input)) {
    auto map_output = fs::path(output.string() + ".map");
    fs::copy_file(map_input, map_output, fs::copy_options::overwrite_existing);
    result.sourcemap_path = map_output.string();
  }

  result.success = true;
  result.output_path = output.string();
  result.output_size = fs::file_size(output);
  result.content_hash = hash;

  return result;
}

std::vector<AssetResult> run_pipeline(const PipelineConfig& cfg) {
  std::vector<AssetResult> results;

  if (!fs::exists(cfg.input_dir))
    return results;
  fs::create_directories(cfg.output_dir);

  // Process CSS files.
  for ([[maybe_unused]] const auto& _pattern : cfg.css_patterns) {
    for (auto& entry : fs::directory_iterator(cfg.input_dir)) {
      if (!entry.is_regular_file())
        continue;
      if (entry.path().extension() != ".css" && entry.path().extension() != ".scss")
        continue;

      auto result = process_css(entry.path(), cfg.output_dir, cfg.css_opts);
      if (cfg.fingerprint && result.success) {
        result = fingerprint(result.output_path, cfg.output_dir);
      }
      results.push_back(std::move(result));
    }
  }

  // Process JS files.
  for ([[maybe_unused]] const auto& _pattern : cfg.js_patterns) {
    std::vector<fs::path> js_files;
    for (auto& entry : fs::directory_iterator(cfg.input_dir)) {
      if (!entry.is_regular_file())
        continue;
      auto ext = entry.path().extension();
      if (ext == ".js" || ext == ".ts" || ext == ".mjs" || ext == ".mts") {
        js_files.push_back(entry.path());
      }
    }

    if (!js_files.empty()) {
      auto result = bundle_js(js_files, cfg.output_dir, cfg.js_opts);
      if (cfg.fingerprint && result.success) {
        result = fingerprint(result.output_path, cfg.output_dir);
      }
      results.push_back(std::move(result));
    }
  }

  return results;
}

bool write_manifest(const std::vector<AssetResult>& results,
                    const fs::path& output_path) {
  nlohmann::json manifest = nlohmann::json::object();

  for (const auto& r : results) {
    if (!r.success)
      continue;
    // Map original name → fingerprinted path.
    fs::path original(r.output_path);
    nlohmann::json entry = {
        {"path", r.output_path}, {"size", r.output_size}, {"hash", r.content_hash}};
    if (!r.sourcemap_path.empty())
      entry["sourcemap"] = r.sourcemap_path;
    manifest[original.filename().string()] = entry;
  }

  fs::create_directories(output_path.parent_path());
  std::ofstream f(output_path);
  if (!f.is_open())
    return false;
  f << manifest.dump(2) << "\n";
  return true;
}

}  // namespace nift::assets
