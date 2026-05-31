/// @file test_migrator.cpp

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include "nift/compat/migrator.hpp"

using namespace nift::compat;
namespace fs = std::filesystem;

namespace {
fs::path tmp_root(const std::string& tag) {
  auto p = fs::temp_directory_path() /
           ("nift_compat_" + tag + "_" + std::to_string(std::rand()));
  fs::create_directories(p);
  return p;
}
void write(const fs::path& p, const std::string& body) {
  fs::create_directories(p.parent_path());
  std::ofstream out(p, std::ios::binary);
  out.write(body.data(), static_cast<std::streamsize>(body.size()));
}
}  // namespace

TEST_CASE("translate_config_key: known v1 keys", "[compat][migrator]") {
  CHECK(translate_config_key("contentDir") == "content_dir");
  CHECK(translate_config_key("siteDir") == "output_dir");
  CHECK(translate_config_key("templateDir") == "template_dir");
  CHECK(translate_config_key("defaultTemplate") == "default_template");
  CHECK(translate_config_key("siteName") == "name");
  CHECK(translate_config_key("buildDir") == "cache_dir");
}

TEST_CASE("translate_config_key: unknown returns empty",
          "[compat][migrator]") {
  CHECK(translate_config_key("madeUpKey").empty());
}

TEST_CASE("migrate_project: no v1 config writes default v2",
          "[compat][migrator]") {
  auto root = tmp_root("nodefault");
  auto r = migrate_project(::nift::core::Path(root.string()));
  REQUIRE(r);
  CHECK(r->config_written);
  CHECK(fs::exists(root / "nift.json"));
  fs::remove_all(root);
}

TEST_CASE("migrate_project: parses v1 nsm.config", "[compat][migrator]") {
  auto root = tmp_root("v1cfg");
  write(root / "siteInfo" / "nsm.config",
        "siteName = my-old-site\n"
        "contentDir = src\n"
        "siteDir = dist\n"
        "templateDir = layouts\n"
        "# this is a comment\n"
        "weirdKey = ignored\n");

  auto r = migrate_project(::nift::core::Path(root.string()));
  REQUIRE(r);
  CHECK(r->config_written);
  CHECK(r->source_config_path.find("nsm.config") != std::string::npos);

  // Notes should mention the unknown key.
  bool saw_unknown = false;
  for (const auto& n : r->notes) {
    if (n.find("weirdKey") != std::string::npos) saw_unknown = true;
  }
  CHECK(saw_unknown);

  // v2 config file written.
  REQUIRE(fs::exists(root / "nift.json"));
  std::ifstream f(root / "nift.json");
  std::string body((std::istreambuf_iterator<char>(f)),
                   std::istreambuf_iterator<char>());
  CHECK(body.find("my-old-site") != std::string::npos);
  CHECK(body.find("src") != std::string::npos);
  CHECK(body.find("dist") != std::string::npos);
  fs::remove_all(root);
}

TEST_CASE("migrate_project: parses key:value separator", "[compat][migrator]") {
  auto root = tmp_root("colon");
  write(root / "nsm.config", "siteName: test\ncontentDir: c\n");
  auto r = migrate_project(::nift::core::Path(root.string()));
  REQUIRE(r);
  CHECK(r->config_written);
  fs::remove_all(root);
}

TEST_CASE("migrate_project: migrates tracked.json", "[compat][migrator]") {
  auto root = tmp_root("tracked");
  write(root / "siteInfo" / "nsm.config", "siteName = x\n");
  write(root / "siteInfo" / "tracked.json", "{\"files\":[]}");
  auto r = migrate_project(::nift::core::Path(root.string()));
  REQUIRE(r);
  CHECK(fs::exists(root / ".nift" / "tracked.json"));
  fs::remove_all(root);
}

TEST_CASE("migrate_project: counts files examined", "[compat][migrator]") {
  auto root = tmp_root("count");
  for (int i = 0; i < 5; ++i) {
    write(root / ("f" + std::to_string(i) + ".txt"), "x");
  }
  auto r = migrate_project(::nift::core::Path(root.string()));
  REQUIRE(r);
  CHECK(r->files_examined >= 5);
  fs::remove_all(root);
}
