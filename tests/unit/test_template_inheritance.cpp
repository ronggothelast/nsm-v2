/// @file test_template_inheritance.cpp

#include <catch2/catch_test_macros.hpp>

#include "nift/parser/ast.hpp"
#include "nift/parser/evaluator.hpp"
#include "nift/parser/parser.hpp"

using namespace nift::parser;

// Helper: find first SectionNode in AST children
static const SectionNode* find_section(const ProgramNode& ast,
                                       const std::string& name) {
  for (const auto& child : ast.children) {
    auto* sec = std::get_if<SectionNode>(child.get());
    if (sec && sec->name == name)
      return sec;
  }
  return nullptr;
}

TEST_CASE("parse @extends directive", "[parser][inheritance]") {
  auto ast = parse_template("@extends(\"layout.html\")", "test.html");
  REQUIRE(ast.children.size() == 1);
  auto* ext = std::get_if<ExtendsNode>(ast.children[0].get());
  REQUIRE(ext != nullptr);
  CHECK(ext->layout_path == "layout.html");
}

TEST_CASE("parse @yield directive", "[parser][inheritance]") {
  auto ast = parse_template("@yield(\"content\")", "layout.html");
  REQUIRE(ast.children.size() == 1);
  auto* yield = std::get_if<YieldNode>(ast.children[0].get());
  REQUIRE(yield != nullptr);
  CHECK(yield->name == "content");
}

TEST_CASE("parse @parent directive", "[parser][inheritance]") {
  auto ast = parse_template("@parent", "child.html");
  REQUIRE(ast.children.size() == 1);
  auto* parent = std::get_if<ParentNode>(ast.children[0].get());
  REQUIRE(parent != nullptr);
}

TEST_CASE("parse @section block with braces", "[parser][inheritance]") {
  auto ast = parse_template(
      "@section(\"content\") {\n"
      "  <h1>Hello</h1>\n"
      "}",
      "child.html");
  auto* sec = find_section(ast, "content");
  REQUIRE(sec != nullptr);
  CHECK_FALSE(sec->body.empty());
}

TEST_CASE("parse @section with @endsection", "[parser][inheritance]") {
  auto ast = parse_template(
      "@section(\"content\") {\n"
      "  <p>Body</p>\n"
      "@endsection",
      "child.html");
  auto* sec = find_section(ast, "content");
  REQUIRE(sec != nullptr);
  CHECK(sec->name == "content");
}

TEST_CASE("evaluate @extends sets __extends variable", "[evaluator][inheritance]") {
  auto ast = parse_template("@extends(\"layout.html\")", "test.html");
  EvalContext ctx;
  ctx.register_defaults();
  Evaluator eval(ctx);
  accept(eval, *ast.children[0]);
  CHECK(ctx.get_var("__extends") == "layout.html");
}

TEST_CASE("evaluate @yield renders section content", "[evaluator][inheritance]") {
  EvalContext ctx;
  ctx.register_defaults();
  ctx.set_var("__section_content", "<h1>Hello World</h1>");

  auto ast = parse_template("@yield(\"content\")", "layout.html");
  Evaluator eval(ctx);
  accept(eval, *ast.children[0]);
  CHECK(ctx.output == "<h1>Hello World</h1>");
}

TEST_CASE("evaluate @yield with empty section", "[evaluator][inheritance]") {
  EvalContext ctx;
  ctx.register_defaults();

  auto ast = parse_template("@yield(\"missing\")", "layout.html");
  Evaluator eval(ctx);
  accept(eval, *ast.children[0]);
  CHECK(ctx.output.empty());
}

TEST_CASE("evaluate @section stores content", "[evaluator][inheritance]") {
  auto ast = parse_template(
      "@section(\"header\") {\n"
      "  <nav>Navigation</nav>\n"
      "}",
      "child.html");

  EvalContext ctx;
  ctx.register_defaults();
  Evaluator eval(ctx);
  for (const auto& child : ast.children) {
    accept(eval, *child);
  }
  auto val = ctx.get_var("__section_header");
  CHECK(val.find("Navigation") != std::string::npos);
}

TEST_CASE("evaluate @section after @extends", "[evaluator][inheritance]") {
  auto ast = parse_template(
      "@extends(\"layout.html\")\n"
      "@section(\"content\") {\n"
      "  <h1>Child Content</h1>\n"
      "}",
      "child.html");

  // Verify SectionNode exists
  auto* sec = find_section(ast, "content");
  REQUIRE(sec != nullptr);
  REQUIRE(sec->body.size() > 0);

  // Evaluate and check section was stored
  EvalContext ctx;
  ctx.register_defaults();
  Evaluator eval(ctx);
  for (const auto& child : ast.children) {
    accept(eval, *child);
  }
  auto section_val = ctx.get_var("__section_content");
  CHECK_FALSE(section_val.empty());
  CHECK(section_val.find("Child Content") != std::string::npos);
}

TEST_CASE("full inheritance flow: child extends layout", "[evaluator][inheritance]") {
  auto child_ast = parse_template(
      "@extends(\"layout.html\")\n"
      "@section(\"content\") {\n"
      "  <h1>Child Content</h1>\n"
      "}",
      "child.html");

  EvalContext child_ctx;
  child_ctx.register_defaults();
  Evaluator child_eval(child_ctx);
  for (const auto& child : child_ast.children) {
    accept(child_eval, *child);
  }

  auto section_val = child_ctx.get_var("__section_content");
  REQUIRE_FALSE(section_val.empty());

  auto layout_ast = parse_template(
      "<html>\n"
      "@yield(\"content\")\n"
      "</html>",
      "layout.html");

  EvalContext layout_ctx;
  layout_ctx.register_defaults();
  layout_ctx.set_var("__section_content", std::string(section_val));

  Evaluator layout_eval(layout_ctx);
  for (const auto& child : layout_ast.children) {
    accept(layout_eval, *child);
  }

  CHECK(layout_ctx.output.find("<h1>Child Content</h1>") != std::string::npos);
  CHECK(layout_ctx.output.find("<html>") != std::string::npos);
  CHECK(layout_ctx.output.find("</html>") != std::string::npos);
}

TEST_CASE("@extends with mixed content", "[parser][inheritance]") {
  auto ast = parse_template(
      "@extends(\"base.html\")\n"
      "@section(\"title\") {\n"
      "  My Page\n"
      "}\n"
      "@section(\"body\") {\n"
      "  <p>Hello</p>\n"
      "}",
      "page.html");

  // Check we have ExtendsNode
  bool has_extends = false;
  for (const auto& child : ast.children) {
    if (std::get_if<ExtendsNode>(child.get()))
      has_extends = true;
  }
  CHECK(has_extends);
  CHECK(find_section(ast, "title") != nullptr);
  CHECK(find_section(ast, "body") != nullptr);
}

TEST_CASE("@parent in section", "[parser][inheritance]") {
  auto ast = parse_template(
      "@section(\"footer\") {\n"
      "  @parent\n"
      "  <p>Extra footer</p>\n"
      "}",
      "child.html");

  auto* sec = find_section(ast, "footer");
  REQUIRE(sec != nullptr);
  bool has_parent = false;
  for (const auto& node : sec->body) {
    if (std::get_if<ParentNode>(node.get())) {
      has_parent = true;
    }
  }
  CHECK(has_parent);
}
