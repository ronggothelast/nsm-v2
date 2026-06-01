#pragma once
// ─── nift/assets/pipeline.hpp ────────────────────────────────────────
// Asset pipeline: CSS minification, JS bundling, fingerprinting.
// Uses external tools (esbuild, lightningcss) via subprocess.

#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <optional>
#include <unordered_map>

namespace nift::assets {

namespace fs = std::filesystem;

// Asset processing result.
struct AssetResult {
    bool success = false;
    std::string output_path;
    std::string error_message;
    size_t input_size = 0;
    size_t output_size = 0;
    double duration_ms = 0.0;
    std::string content_hash;  // BLAKE3 hash for fingerprinting
};

// CSS processing options.
struct CssOptions {
    bool minify = true;
    bool sourcemap = false;
    bool autoprefixer = false;  // Requires lightningcss + browserslist
    std::string targets;        // Browserslist targets string
};

// JS bundling options.
struct JsOptions {
    bool minify = true;
    bool sourcemap = false;
    bool bundle = true;
    std::string format = "esm";  // esm, cjs, iife
    std::string target = "es2020";
    std::vector<std::string> external;  // External modules to exclude
    std::unordered_map<std::string, std::string> define;  // Compile-time defines
};

// Find esbuild binary.
[[nodiscard]] std::string find_esbuild();

// Find lightningcss binary.
[[nodiscard]] std::string find_lightningcss();

// Check tool availability.
[[nodiscard]] bool esbuild_available();
[[nodiscard]] bool lightningcss_available();

// Process CSS file: minify + optional sourcemap.
[[nodiscard]] AssetResult process_css(const fs::path& input,
                                        const fs::path& output_dir,
                                        const CssOptions& opts = {});

// Bundle JS file(s): bundle + minify + optional sourcemap.
[[nodiscard]] AssetResult bundle_js(const std::vector<fs::path>& inputs,
                                      const fs::path& output_dir,
                                      const JsOptions& opts = {});

// Fingerprint a file: compute BLAKE3 hash and rename.
// Returns the new filename with hash (e.g., "style.a1b2c3.css").
[[nodiscard]] AssetResult fingerprint(const fs::path& input,
                                        const fs::path& output_dir);

// Process all assets in a directory: find CSS/JS files, process them.
struct PipelineConfig {
    fs::path input_dir;      // Source assets directory
    fs::path output_dir;     // Output directory
    CssOptions css_opts;
    JsOptions js_opts;
    bool fingerprint = false;  // Add content hash to filenames
    std::vector<std::string> css_patterns = {"*.css"};
    std::vector<std::string> js_patterns = {"*.js", "*.ts"};
};
[[nodiscard]] std::vector<AssetResult> run_pipeline(const PipelineConfig& cfg);

// Generate a manifest.json mapping original → fingerprinted filenames.
[[nodiscard]] bool write_manifest(const std::vector<AssetResult>& results,
                                    const fs::path& output_path);

}  // namespace nift::assets
