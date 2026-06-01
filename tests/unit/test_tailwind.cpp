// ─── test_tailwind.cpp ───────────────────────────────────────────────
// Unit tests for nift::tailwind: config generation, class scanning, CLI detection.

#include <catch2/catch_test_macros.hpp>
#include <nift/tailwind/tailwind.hpp>

#include <fstream>
#include <filesystem>

using namespace nift::tailwind;
namespace fs = std::filesystem;

// Helper: create a temp directory for tests.
struct TmpDir {
    fs::path path;
    TmpDir() : path(fs::temp_directory_path() / ("nift_tw_test_" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()))) {
        fs::create_directories(path);
    }
    ~TmpDir() { fs::remove_all(path); }
};

// Helper: write a file.
static void write_file(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream f(p);
    f << content;
}

// ═══════════════════════════════════════════════════════════════════
// CLI detection
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("find_cli: returns non-empty or empty string", "[tailwind]") {
    auto cli = find_cli();
    // We can't guarantee tailwind is installed, so just check it returns.
    // If installed, it should return a non-empty string.
    INFO("Detected CLI: " << cli);
    // Always passes — we just verify it doesn't crash.
    CHECK(true);
}

TEST_CASE("is_available: consistent with find_cli", "[tailwind]") {
    auto cli = find_cli();
    auto avail = is_available();
    CHECK(avail == !cli.empty());
}

// ═══════════════════════════════════════════════════════════════════
// Config generation
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("generate_config: creates valid JS file", "[tailwind]") {
    TmpDir tmp;

    TailwindConfig cfg;
    cfg.working_dir = tmp.path;
    cfg.content_dirs = {"content", "layouts"};
    cfg.content_patterns = {"src/**/*.{js,ts}"};

    auto config_path = generate_config(cfg);
    CHECK(fs::exists(config_path));
    CHECK(config_path.filename() == "tailwind.config.js");

    // Read and verify content.
    std::ifstream f(config_path);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    CHECK(content.find("module.exports") != std::string::npos);
    CHECK(content.find("content") != std::string::npos);
    CHECK(content.find("content/") != std::string::npos);
    CHECK(content.find("layouts/") != std::string::npos);
    CHECK(content.find("src/**/*.{js,ts}") != std::string::npos);
    CHECK(content.find("theme") != std::string::npos);
    CHECK(content.find("extend") != std::string::npos);
}

TEST_CASE("generate_config: uses existing config if provided", "[tailwind]") {
    TmpDir tmp;

    // Create an existing config file.
    auto existing = tmp.path / "my-config.js";
    write_file(existing, "module.exports = { content: [] }");

    TailwindConfig cfg;
    cfg.working_dir = tmp.path;
    cfg.config_path = existing.string();

    auto result = generate_config(cfg);
    CHECK(result == existing);
}

TEST_CASE("generate_config: includes theme overrides", "[tailwind]") {
    TmpDir tmp;

    TailwindConfig cfg;
    cfg.working_dir = tmp.path;
    cfg.content_dirs = {"content"};
    cfg.theme_overrides = nlohmann::json{
        {"colors", {{"primary", "#ff0000"}}},
        {"fontFamily", {{"sans", {"Inter", "sans-serif"}}}}
    };

    auto config_path = generate_config(cfg);
    std::ifstream f(config_path);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    CHECK(content.find("colors") != std::string::npos);
    CHECK(content.find("#ff0000") != std::string::npos);
    CHECK(content.find("fontFamily") != std::string::npos);
    CHECK(content.find("Inter") != std::string::npos);
}

TEST_CASE("generate_config: includes plugins", "[tailwind]") {
    TmpDir tmp;

    TailwindConfig cfg;
    cfg.working_dir = tmp.path;
    cfg.content_dirs = {"content"};
    cfg.plugins = {"@tailwindcss/typography", "@tailwindcss/forms"};

    auto config_path = generate_config(cfg);
    std::ifstream f(config_path);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    CHECK(content.find("plugins") != std::string::npos);
    CHECK(content.find("@tailwindcss/typography") != std::string::npos);
    CHECK(content.find("@tailwindcss/forms") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Input CSS generation
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("generate_input_css: creates file with directives", "[tailwind]") {
    TmpDir tmp;

    TailwindConfig cfg;
    cfg.working_dir = tmp.path;

    auto input_path = generate_input_css(cfg);
    CHECK(fs::exists(input_path));

    std::ifstream f(input_path);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    CHECK(content.find("@tailwind base") != std::string::npos);
    CHECK(content.find("@tailwind components") != std::string::npos);
    CHECK(content.find("@tailwind utilities") != std::string::npos);
}

TEST_CASE("generate_input_css: includes custom CSS", "[tailwind]") {
    TmpDir tmp;

    TailwindConfig cfg;
    cfg.working_dir = tmp.path;

    auto input_path = generate_input_css(cfg, ":root { --color: red; }");
    std::ifstream f(input_path);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    CHECK(content.find("--color: red") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Class scanning
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("scan_classes: finds classes in HTML files", "[tailwind]") {
    TmpDir tmp;

    write_file(tmp.path / "index.html",
        "<div class=\"flex items-center p-4\">\n"
        "  <span className=\"text-lg font-bold\">Hello</span>\n"
        "</div>");

    auto classes = scan_classes(tmp.path);

    CHECK(std::find(classes.begin(), classes.end(), "flex") != classes.end());
    CHECK(std::find(classes.begin(), classes.end(), "items-center") != classes.end());
    CHECK(std::find(classes.begin(), classes.end(), "p-4") != classes.end());
    CHECK(std::find(classes.begin(), classes.end(), "text-lg") != classes.end());
    CHECK(std::find(classes.begin(), classes.end(), "font-bold") != classes.end());
}

TEST_CASE("scan_classes: finds classes in JS/TS files", "[tailwind]") {
    TmpDir tmp;

    write_file(tmp.path / "app.tsx",
        "const el = <div className=\"bg-blue-500 text-white p-2\">OK</div>;");

    auto classes = scan_classes(tmp.path);

    CHECK(std::find(classes.begin(), classes.end(), "bg-blue-500") != classes.end());
    CHECK(std::find(classes.begin(), classes.end(), "text-white") != classes.end());
}

TEST_CASE("scan_classes: deduplicates classes", "[tailwind]") {
    TmpDir tmp;

    write_file(tmp.path / "a.html", "<div class=\"p-4 flex\">A</div>");
    write_file(tmp.path / "b.html", "<div class=\"p-4 flex\">B</div>");

    auto classes = scan_classes(tmp.path);

    // Count occurrences of "p-4".
    int count = std::count(classes.begin(), classes.end(), "p-4");
    CHECK(count == 1);  // Deduplicated.
}

TEST_CASE("scan_classes: empty directory", "[tailwind]") {
    TmpDir tmp;
    fs::create_directories(tmp.path / "empty");

    auto classes = scan_classes(tmp.path / "empty");
    CHECK(classes.empty());
}

TEST_CASE("scan_classes: ignores non-content files", "[tailwind]") {
    TmpDir tmp;

    write_file(tmp.path / "style.css", ".flex { display: flex; }");
    write_file(tmp.path / "image.png", "binary");

    auto classes = scan_classes(tmp.path);
    // CSS and binary files should be ignored.
    CHECK(classes.empty());
}

// ═══════════════════════════════════════════════════════════════════
// Compile (only if Tailwind is available)
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("compile: fails gracefully when CLI not found", "[tailwind]") {
    TailwindConfig cfg;
    cfg.cli_path = "/nonexistent/tailwindcss";
    cfg.working_dir = fs::current_path();

    auto result = compile(cfg);
    CHECK(result.success == false);
    CHECK(!result.error_message.empty());
}

TEST_CASE("watch: fails gracefully when CLI not found", "[tailwind]") {
    TailwindConfig cfg;
    cfg.cli_path = "/nonexistent/tailwindcss";
    cfg.working_dir = fs::current_path();

    auto result = watch(cfg);
    CHECK(result.success == false);
    CHECK(!result.error_message.empty());
}
