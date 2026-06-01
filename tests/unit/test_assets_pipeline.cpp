// ─── test_assets_pipeline.cpp ────────────────────────────────────────
// Unit tests for nift::assets: CSS processing, JS bundling, fingerprinting, manifest.

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <nift/assets/pipeline.hpp>

#include <fstream>
#include <filesystem>

using namespace nift::assets;
namespace fs = std::filesystem;

// Helper: create a temp directory for tests.
struct AssetTmpDir {
    fs::path path;
    AssetTmpDir() : path(fs::temp_directory_path() / ("nift_asset_test_" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()))) {
        fs::create_directories(path);
    }
    ~AssetTmpDir() { fs::remove_all(path); }
};

static void write_test_file(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream f(p);
    f << content;
}

// ═══════════════════════════════════════════════════════════════════
// CLI detection
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("find_esbuild: returns string", "[assets]") {
    auto cli = find_esbuild();
    INFO("esbuild: " << cli);
    CHECK(true);  // Just verify no crash.
}

TEST_CASE("find_lightningcss: returns string", "[assets]") {
    auto cli = find_lightningcss();
    INFO("lightningcss: " << cli);
    CHECK(true);
}

// ═══════════════════════════════════════════════════════════════════
// CSS processing (fallback mode — no lightningcss needed)
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("process_css: basic minification (fallback)", "[assets]") {
    AssetTmpDir tmp;

    std::string css = R"(
        /* Comment */
        body {
            margin: 0;
            padding: 0;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
    )";

    auto input = tmp.path / "style.css";
    write_test_file(input, css);

    auto output_dir = tmp.path / "out";
    CssOptions opts;
    opts.minify = true;
    opts.sourcemap = false;

    auto result = process_css(input, output_dir, opts);

    CHECK(result.success);
    CHECK(!result.output_path.empty());
    CHECK(result.output_size > 0);
    CHECK(result.output_size < result.input_size);  // Minified should be smaller.
    CHECK(!result.content_hash.empty());
    CHECK(result.content_hash.size() == 64);  // BLAKE3 hex = 64 chars.
}

TEST_CASE("process_css: non-minified copy", "[assets]") {
    AssetTmpDir tmp;

    std::string css = "body { color: red; }";
    auto input = tmp.path / "style.css";
    write_test_file(input, css);

    auto output_dir = tmp.path / "out";
    CssOptions opts;
    opts.minify = false;

    auto result = process_css(input, output_dir, opts);

    CHECK(result.success);
    CHECK(result.output_size > 0);
}

TEST_CASE("process_css: missing input file", "[assets]") {
    AssetTmpDir tmp;

    auto result = process_css(tmp.path / "nonexistent.css", tmp.path / "out");
    CHECK(result.success == false);
    CHECK(!result.error_message.empty());
}

// ═══════════════════════════════════════════════════════════════════
// JS bundling (requires esbuild)
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("bundle_js: fails gracefully without esbuild", "[assets]") {
    AssetTmpDir tmp;

    auto input = tmp.path / "app.js";
    write_test_file(input, "console.log('hello');");

    JsOptions opts;
    auto result = bundle_js({input}, tmp.path / "out", opts);

    // esbuild may or may not be installed. If not, should fail gracefully.
    if (!esbuild_available()) {
        CHECK(result.success == false);
        CHECK(!result.error_message.empty());
    }
    // If esbuild IS available, it should succeed.
    if (esbuild_available()) {
        CHECK(result.success);
    }
}

TEST_CASE("bundle_js: empty inputs", "[assets]") {
    AssetTmpDir tmp;
    JsOptions opts;

    auto result = bundle_js({}, tmp.path / "out", opts);
    CHECK(result.success == false);
}

TEST_CASE("bundle_js: missing input", "[assets]") {
    AssetTmpDir tmp;
    JsOptions opts;

    auto result = bundle_js({tmp.path / "nonexistent.js"}, tmp.path / "out", opts);
    CHECK(result.success == false);
}

// ═══════════════════════════════════════════════════════════════════
// Fingerprinting
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("fingerprint: adds hash to filename", "[assets]") {
    AssetTmpDir tmp;

    auto input = tmp.path / "style.css";
    write_test_file(input, "body { margin: 0; }");

    auto output_dir = tmp.path / "fingerprinted";
    auto result = fingerprint(input, output_dir);

    CHECK(result.success);
    CHECK(!result.content_hash.empty());

    // Output filename should contain hash.
    fs::path out_path(result.output_path);
    auto fname = out_path.filename().string();
    CHECK(fname.find(result.content_hash.substr(0, 8)) != std::string::npos);
    CHECK(fname.find(".css") != std::string::npos);

    // File should exist.
    CHECK(fs::exists(out_path));
}

TEST_CASE("fingerprint: same content = same hash", "[assets]") {
    AssetTmpDir tmp;

    std::string content = "body { color: blue; }";
    auto input1 = tmp.path / "a.css";
    auto input2 = tmp.path / "b.css";
    write_test_file(input1, content);
    write_test_file(input2, content);

    auto out1 = fingerprint(input1, tmp.path / "out1");
    auto out2 = fingerprint(input2, tmp.path / "out2");

    CHECK(out1.success);
    CHECK(out2.success);
    CHECK(out1.content_hash == out2.content_hash);  // Same content = same hash.
}

TEST_CASE("fingerprint: different content = different hash", "[assets]") {
    AssetTmpDir tmp;

    auto input1 = tmp.path / "a.css";
    auto input2 = tmp.path / "b.css";
    write_test_file(input1, "body { color: red; }");
    write_test_file(input2, "body { color: blue; }");

    auto out1 = fingerprint(input1, tmp.path / "out1");
    auto out2 = fingerprint(input2, tmp.path / "out2");

    CHECK(out1.success);
    CHECK(out2.success);
    CHECK(out1.content_hash != out2.content_hash);
}

TEST_CASE("fingerprint: missing input", "[assets]") {
    AssetTmpDir tmp;

    auto result = fingerprint(tmp.path / "nope.css", tmp.path / "out");
    CHECK(result.success == false);
}

// ═══════════════════════════════════════════════════════════════════
// Pipeline
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("run_pipeline: processes CSS files in directory", "[assets]") {
    AssetTmpDir tmp;

    auto input_dir = tmp.path / "assets";
    write_test_file(input_dir / "main.css", "body { margin: 0; }");
    write_test_file(input_dir / "theme.css", ".dark { background: #000; }");

    PipelineConfig cfg;
    cfg.input_dir = input_dir;
    cfg.output_dir = tmp.path / "dist";
    cfg.css_opts.minify = true;

    auto results = run_pipeline(cfg);

    // Should process at least the CSS files.
    CHECK(results.size() >= 2);
    for (const auto& r : results) {
        CHECK(r.success);
    }
}

TEST_CASE("run_pipeline: with fingerprinting", "[assets]") {
    AssetTmpDir tmp;

    auto input_dir = tmp.path / "assets";
    write_test_file(input_dir / "style.css", "body { color: green; }");

    PipelineConfig cfg;
    cfg.input_dir = input_dir;
    cfg.output_dir = tmp.path / "dist";
    cfg.fingerprint = true;

    auto results = run_pipeline(cfg);

    CHECK(results.size() >= 1);
    if (!results.empty()) {
        CHECK(!results[0].content_hash.empty());
    }
}

TEST_CASE("run_pipeline: empty input directory", "[assets]") {
    AssetTmpDir tmp;

    fs::create_directories(tmp.path / "empty");

    PipelineConfig cfg;
    cfg.input_dir = tmp.path / "empty";
    cfg.output_dir = tmp.path / "dist";

    auto results = run_pipeline(cfg);
    CHECK(results.empty());
}

TEST_CASE("run_pipeline: missing input directory", "[assets]") {
    AssetTmpDir tmp;

    PipelineConfig cfg;
    cfg.input_dir = tmp.path / "nonexistent";
    cfg.output_dir = tmp.path / "dist";

    auto results = run_pipeline(cfg);
    CHECK(results.empty());
}

// ═══════════════════════════════════════════════════════════════════
// Manifest
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("write_manifest: creates valid JSON", "[assets]") {
    AssetTmpDir tmp;

    // Create some results.
    AssetResult r1;
    r1.success = true;
    r1.output_path = (tmp.path / "style.min.css").string();
    r1.output_size = 1024;
    r1.content_hash = "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890";

    AssetResult r2;
    r2.success = true;
    r2.output_path = (tmp.path / "app.min.mjs").string();
    r2.output_size = 2048;
    r2.content_hash = "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef";

    AssetResult r3;
    r3.success = false;  // Failed — should be skipped.

    auto manifest_path = tmp.path / "dist" / "manifest.json";
    bool ok = write_manifest({r1, r2, r3}, manifest_path);

    CHECK(ok);
    CHECK(fs::exists(manifest_path));

    // Read and verify JSON.
    std::ifstream f(manifest_path);
    nlohmann::json manifest = nlohmann::json::parse(f);

    CHECK(manifest.size() == 2);  // Only successful results.
    CHECK(manifest.contains("style.min.css"));
    CHECK(manifest.contains("app.min.mjs"));
    CHECK(manifest["style.min.css"]["size"] == 1024);
    CHECK(manifest["app.min.mjs"]["size"] == 2048);
}

TEST_CASE("write_manifest: empty results", "[assets]") {
    AssetTmpDir tmp;

    auto manifest_path = tmp.path / "manifest.json";
    bool ok = write_manifest({}, manifest_path);

    CHECK(ok);
    std::ifstream f(manifest_path);
    nlohmann::json manifest = nlohmann::json::parse(f);
    CHECK(manifest.empty());
}
