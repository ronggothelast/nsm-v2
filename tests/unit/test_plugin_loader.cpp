/// @file test_plugin_loader.cpp

#include <catch2/catch_test_macros.hpp>

#include "nift/plugin/plugin_loader.hpp"

using namespace nift::plugin;

TEST_CASE("PluginRegistry: empty has no directives", "[plugin][loader]") {
  PluginRegistry reg;
  CHECK(reg.size() == 0);
  CHECK_FALSE(reg.has_directive("anything"));
}

TEST_CASE("PluginRegistry: load missing file fails", "[plugin][loader]") {
  PluginRegistry reg;
  auto r = reg.load(::nift::core::Path("/tmp/__definitely_not_a_plugin__.so"));
  CHECK_FALSE(r);
}

TEST_CASE("PluginRegistry: render unknown directive fails", "[plugin][loader]") {
  PluginRegistry reg;
  auto r = reg.render("nope", "");
  CHECK_FALSE(r);
}

TEST_CASE("PluginRegistry: names empty when no plugins", "[plugin][loader]") {
  PluginRegistry reg;
  CHECK(reg.names().empty());
}
