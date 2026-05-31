/// @file test_lua_runtime.cpp

#include <catch2/catch_test_macros.hpp>

#include "nift/runtime/lua_runtime.hpp"

using namespace nift::runtime;

TEST_CASE("LuaRuntime: basic arithmetic", "[runtime][lua]") {
  LuaRuntime lua;
  auto result = lua.exec("return 1 + 2 * 3");
  REQUIRE(result);
  CHECK(*result == "7");
}

TEST_CASE("LuaRuntime: print captures output", "[runtime][lua]") {
  LuaRuntime lua;
  auto result = lua.exec("print('hello')");
  REQUIRE(result);
  CHECK(*result == "hello\n");
}

TEST_CASE("LuaRuntime: string return", "[runtime][lua]") {
  LuaRuntime lua;
  auto result = lua.exec("return 'world'");
  REQUIRE(result);
  CHECK(*result == "world");
}

TEST_CASE("LuaRuntime: globals round-trip", "[runtime][lua]") {
  LuaRuntime lua;
  lua.set_global("name", "nift");
  auto result = lua.exec("return 'hi ' .. name");
  REQUIRE(result);
  CHECK(*result == "hi nift");
}

TEST_CASE("LuaRuntime: get_global numeric", "[runtime][lua]") {
  LuaRuntime lua;
  auto result = lua.exec("counter = 42");
  REQUIRE(result);
  CHECK(lua.get_global("counter") == "42");
}

TEST_CASE("LuaRuntime: syntax error returns Error", "[runtime][lua]") {
  LuaRuntime lua;
  auto result = lua.exec("this is not valid lua $$$");
  REQUIRE_FALSE(result);
}

TEST_CASE("LuaRuntime: io disabled by default (sandbox)", "[runtime][lua]") {
  LuaRuntime lua;
  auto result = lua.exec("return io ~= nil");
  REQUIRE(result);
  CHECK(*result == "false");
}

TEST_CASE("LuaRuntime: enable_unsafe loads io", "[runtime][lua]") {
  LuaRuntime lua;
  lua.enable_unsafe();
  auto result = lua.exec("return io ~= nil");
  REQUIRE(result);
  CHECK(*result == "true");
}

TEST_CASE("LuaRuntime: math available by default", "[runtime][lua]") {
  LuaRuntime lua;
  auto result = lua.exec("return math.floor(3.7)");
  REQUIRE(result);
  CHECK(*result == "3");
}

TEST_CASE("LuaRuntime: string lib available", "[runtime][lua]") {
  LuaRuntime lua;
  auto result = lua.exec("return string.upper('abc')");
  REQUIRE(result);
  CHECK(*result == "ABC");
}

TEST_CASE("LuaRuntime: table operations", "[runtime][lua]") {
  LuaRuntime lua;
  auto result = lua.exec("local t = {1, 2, 3}; return #t");
  REQUIRE(result);
  CHECK(*result == "3");
}

TEST_CASE("LuaRuntime: multi-line script", "[runtime][lua]") {
  LuaRuntime lua;
  auto result = lua.exec(R"(
    local sum = 0
    for i = 1, 5 do sum = sum + i end
    return sum
  )");
  REQUIRE(result);
  CHECK(*result == "15");
}

TEST_CASE("LuaRuntime: boolean return", "[runtime][lua]") {
  LuaRuntime lua;
  auto t = lua.exec("return true");
  REQUIRE(t);
  CHECK(*t == "true");

  auto f = lua.exec("return false");
  REQUIRE(f);
  CHECK(*f == "false");
}

TEST_CASE("LuaRuntime: independent state per instance", "[runtime][lua]") {
  LuaRuntime a;
  LuaRuntime b;
  a.set_global("x", "from_a");
  b.set_global("x", "from_b");
  CHECK(a.get_global("x") == "from_a");
  CHECK(b.get_global("x") == "from_b");
}
