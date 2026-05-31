/// @file test_project.cpp

#include <catch2/catch_test_macros.hpp>

#include "nift/project/project.hpp"

using namespace nift::project;

TEST_CASE("ProjectConfig: default values", "[project][config]") {
  ProjectConfig c;
  CHECK(c.name == "nift-site");
  CHECK(c.content_dir.str() == "content");
  CHECK(c.output_dir.str() == "public");
  CHECK(c.template_dir.str() == "templates");
}

TEST_CASE("ProjectConfig: round-trip JSON", "[project][config]") {
  ProjectConfig orig;
  orig.name = "my-blog";
  orig.version = "1.2.3";
  orig.content_dir = ::nift::core::Path("src");
  orig.output_dir = ::nift::core::Path("dist");

  auto serialized = orig.to_json();
  REQUIRE(serialized);

  auto parsed = ProjectConfig::from_json(*serialized);
  REQUIRE(parsed);

  CHECK(parsed->name == "my-blog");
  CHECK(parsed->version == "1.2.3");
  CHECK(parsed->content_dir.str() == "src");
  CHECK(parsed->output_dir.str() == "dist");
}

TEST_CASE("ProjectConfig: from_json invalid returns Error", "[project][config]") {
  auto r = ProjectConfig::from_json("not valid json {{{");
  REQUIRE_FALSE(r);
}

TEST_CASE("ProjectConfig: from_json partial keeps defaults", "[project][config]") {
  auto r = ProjectConfig::from_json(R"({"name": "custom"})");
  REQUIRE(r);
  CHECK(r->name == "custom");
  // unspecified fields get default values
  CHECK(r->content_dir.str() == "content");
  CHECK(r->version == "0.0.1");
}

TEST_CASE("ProjectConfig: ignored_paths round-trip", "[project][config]") {
  ProjectConfig c;
  c.ignored_paths = {".cache", "build", "target"};
  auto j = c.to_json();
  REQUIRE(j);
  auto p = ProjectConfig::from_json(*j);
  REQUIRE(p);
  REQUIRE(p->ignored_paths.size() == 3);
  CHECK(p->ignored_paths[0] == ".cache");
  CHECK(p->ignored_paths[2] == "target");
}
