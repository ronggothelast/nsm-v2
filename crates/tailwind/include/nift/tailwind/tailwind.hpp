#pragma once
// ─── nift/tailwind/tailwind.hpp ──────────────────────────────────────
// Tailwind CSS integration via subprocess (Tailwind CLI).
// Manages: config generation, content scanning, CSS compilation.

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <optional>

namespace nift::tailwind {

namespace fs = std::filesystem;

// Configuration for Tailwind CSS integration.
struct TailwindConfig {
    // Path to tailwind CLI binary. Auto-detected if empty.
    std::string cli_path;

    // Path to tailwind.config.js. Generated if empty.
    std::string config_path;

    // Input CSS file path.
    std::string input_css;

    // Output CSS file path.
    std::string output_css;

    // Content directories to scan for class usage.
    std::vector<std::string> content_dirs;

    // Additional content patterns (globs).
    std::vector<std::string> content_patterns;

    // Whether to minify output.
    bool minify = false;

    // Whether to enable sourcemaps.
    bool sourcemaps = false;

    // Custom CSS variables / theme overrides.
    nlohmann::json theme_overrides = nullptr;

    // Additional plugins to include.
    std::vector<std::string> plugins;

    // Working directory (project root).
    fs::path working_dir;
};

// Result of a Tailwind compilation.
struct TailwindResult {
    bool success = false;
    std::string output_path;     // Path to generated CSS
    std::string error_message;   // Error details if failed
    size_t output_size = 0;      // Size of generated CSS in bytes
    double duration_ms = 0.0;    // Compilation time
};

// Find the Tailwind CLI binary.
// Checks: npx tailwindcss, node_modules/.bin/tailwindcss, PATH.
// Returns empty string if not found.
[[nodiscard]] std::string find_cli();

// Check if Tailwind CLI is available.
[[nodiscard]] bool is_available();

// Generate a tailwind.config.js file from configuration.
// Returns the path to the generated config file.
[[nodiscard]] fs::path generate_config(const TailwindConfig& cfg);

// Generate a minimal input CSS file with @tailwind directives.
// Returns the path to the generated file.
[[nodiscard]] fs::path generate_input_css(const TailwindConfig& cfg,
                                            const std::string& custom_css = "");

// Run Tailwind CSS compilation.
// This is the main entry point: finds CLI, generates config, compiles CSS.
[[nodiscard]] TailwindResult compile(const TailwindConfig& cfg);

// Run Tailwind in watch mode (blocking).
// Returns only when the process is interrupted.
[[nodiscard]] TailwindResult watch(const TailwindConfig& cfg);

// Scan content files for Tailwind class usage.
// Returns a list of detected utility classes.
[[nodiscard]] std::vector<std::string> scan_classes(const fs::path& content_dir);

// Generate a purged/minified CSS from a full Tailwind build.
// Useful for production builds.
[[nodiscard]] TailwindResult purge(const TailwindConfig& cfg,
                                     const fs::path& css_input,
                                     const std::vector<fs::path>& content_files);

}  // namespace nift::tailwind
