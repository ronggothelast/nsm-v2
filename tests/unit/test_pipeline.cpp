/// @file test_pipeline.cpp

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <vector>

#include "nift/build/pipeline.hpp"

using namespace nift::build;
namespace fs = std::filesystem;

namespace {

struct TempProject {
  fs::path root;
  ::nift::project::ProjectConfig config;

  TempProject() {
    root = fs::temp_directory_path() /
           ("nift_pipeline_" + std::to_string(std::rand()));
    fs::create_directories(root / "content");
    fs::create_directories(root / "public");
    fs::create_directories(root / ".nift" / "cache");
    config.content_dir = ::nift::core::Path((root / "content").string());
    config.output_dir = ::nift::core::Path((root / "public").string());
    config.cache_dir =
        ::nift::core::Path((root / ".nift" / "cache").string());
  }
  ~TempProject() {
    std::error_code ec;
    fs::remove_all(root, ec);
  }
  ::nift::core::Path write(const std::string& name, const std::string& body) {
    auto p = root / "content" / name;
    std::ofstream out(p, std::ios::binary);
    out.write(body.data(), static_cast<std::streamsize>(body.size()));
    return ::nift::core::Path(p.string());
  }
};

}  // namespace

TEST_CASE("Pipeline: builds single file", "[build][pipeline]") {
  TempProject t;
  auto src = t.write("a.html", "<h1>hello</h1>");
  Pipeline pipe(t.config, nullptr, 2);
  auto report = pipe.build({src});
  CHECK(report.built == 1);
  CHECK(report.cached == 0);
  CHECK(report.failed == 0);
}

TEST_CASE("Pipeline: builds many files in parallel", "[build][pipeline]") {
  TempProject t;
  std::vector<::nift::core::Path> srcs;
  for (int i = 0; i < 50; ++i) {
    srcs.push_back(t.write("p" + std::to_string(i) + ".html",
                           "<p>page " + std::to_string(i) + "</p>"));
  }
  Pipeline pipe(t.config, nullptr, 4);
  auto report = pipe.build(srcs);
  CHECK(report.built == 50);
  CHECK(report.failed == 0);
}

TEST_CASE("Pipeline: missing source fails gracefully", "[build][pipeline]") {
  TempProject t;
  auto bad = ::nift::core::Path("/tmp/__not_here_nift__.html");
  Pipeline pipe(t.config, nullptr, 2);
  auto report = pipe.build({bad});
  CHECK(report.failed == 1);
}

TEST_CASE("Pipeline: cache hit on second build", "[build][pipeline]") {
  TempProject t;
  auto src = t.write("c.html", "stable content");

  ::nift::project::BuildCache cache(t.config.cache_dir);
  Pipeline pipe(t.config, &cache, 2);

  auto first = pipe.build({src});
  CHECK(first.built == 1);
  CHECK(first.cached == 0);

  auto second = pipe.build({src});
  CHECK(second.built == 0);
  CHECK(second.cached == 1);
}

TEST_CASE("Pipeline: cache miss after content change", "[build][pipeline]") {
  TempProject t;
  auto src = t.write("d.html", "v1");

  ::nift::project::BuildCache cache(t.config.cache_dir);
  Pipeline pipe(t.config, &cache, 2);
  pipe.build({src});

  // Change content.
  t.write("d.html", "v2 different bytes");
  auto report = pipe.build({src});
  CHECK(report.built == 1);
  CHECK(report.cached == 0);
}

TEST_CASE("Pipeline: report includes elapsed time", "[build][pipeline]") {
  TempProject t;
  auto src = t.write("t.html", "x");
  Pipeline pipe(t.config, nullptr, 2);
  auto report = pipe.build({src});
  CHECK(report.elapsed.count() >= 0);
  CHECK(report.file_results.size() == 1);
}
