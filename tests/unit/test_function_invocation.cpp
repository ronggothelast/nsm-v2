/// @file test_function_invocation.cpp
/// @brief Tests for @function/@fn definition and invocation.

#include <catch2/catch_test_macros.hpp>

#include "nift/parser/evaluator.hpp"
#include "nift/parser/parser.hpp"

using namespace nift::parser;

TEST_CASE("@fn alias works the same", "[evaluator][function]") {
  auto ast = parse_template(
      "@fn(title) {\n"
      "  My Site\n"
      "}\n"
      "@title",
      "test.html");

  EvalContext ctx;
  ctx.register_defaults();
  Evaluator eval(ctx);
  for (const auto& child : ast.children) {
    accept(eval, *child);
  }

  CHECK(ctx.output.find("My Site") != std::string::npos);
}

TEST_CASE("@function definition works", "[evaluator][function]") {
  // Define a function and invoke it (no params)
  auto ast = parse_template(
      "@function(heading) {\n"
      "  <h1>Welcome</h1>\n"
      "}\n"
      "@heading",
      "test.html");

  EvalContext ctx;
  ctx.register_defaults();
  Evaluator eval(ctx);
  for (const auto& child : ast.children) {
    accept(eval, *child);
  }

  CHECK(ctx.output.find("<h1>Welcome</h1>") != std::string::npos);
}

TEST_CASE("@function invocation after definition", "[evaluator][function]") {
  // Function can be called multiple times
  auto ast = parse_template(
      "@function(br) {\n"
      "  <br>\n"
      "}\n"
      "@br\n"
      "text\n"
      "@br",
      "test.html");

  EvalContext ctx;
  ctx.register_defaults();
  Evaluator eval(ctx);
  for (const auto& child : ast.children) {
    accept(eval, *child);
  }

  CHECK(ctx.output.find("<br>") != std::string::npos);
  CHECK(ctx.output.find("text") != std::string::npos);
}

TEST_CASE("@function body preserved correctly", "[evaluator][function]") {
  // Verify the body is captured by value (the dangling ref fix)
  auto ast = parse_template(
      "@function(separator) {\n"
      "  ---\n"
      "}\n"
      "@separator",
      "test.html");

  EvalContext ctx;
  ctx.register_defaults();
  Evaluator eval(ctx);
  for (const auto& child : ast.children) {
    accept(eval, *child);
  }

  CHECK(ctx.output.find("---") != std::string::npos);
}

TEST_CASE("@function not invoked does nothing", "[evaluator][function]") {
  // Define but don't call — body should not appear in output
  auto ast = parse_template(
      "@function(unused) {\n"
      "  HIDDEN\n"
      "}\n"
      "visible",
      "test.html");

  EvalContext ctx;
  ctx.register_defaults();
  Evaluator eval(ctx);
  for (const auto& child : ast.children) {
    accept(eval, *child);
  }

  CHECK(ctx.output.find("visible") != std::string::npos);
  CHECK(ctx.output.find("HIDDEN") == std::string::npos);
}
