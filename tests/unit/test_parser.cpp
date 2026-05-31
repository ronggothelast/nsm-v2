/// @file test_parser.cpp
/// @brief Tests for the Nift template parser.

#include <catch2/catch_test_macros.hpp>

#include "nift/parser/parser.hpp"

using namespace nift::parser;

TEST_CASE("Parser: plain text", "[parser]") {
  auto ast = parse_template("Hello, world!");
  REQUIRE(ast.children.size() == 1);
  auto* text = std::get_if<TextNode>(ast.children[0].get());
  REQUIRE(text);
  CHECK(text->text == "Hello, world!");
}

TEST_CASE("Parser: simple directive", "[parser]") {
  auto ast = parse_template("@input(header.html)");
  REQUIRE(ast.children.size() == 1);
  auto* dir = std::get_if<DirectiveNode>(ast.children[0].get());
  REQUIRE(dir);
  CHECK(dir->name == "input");
  REQUIRE(dir->params.size() == 1);
  CHECK(dir->params[0] == "header.html");
}

TEST_CASE("Parser: directive with options", "[parser]") {
  auto ast = parse_template("@func{opt1, opt2}(a, b)");
  REQUIRE(ast.children.size() == 1);
  auto* dir = std::get_if<DirectiveNode>(ast.children[0].get());
  REQUIRE(dir);
  CHECK(dir->name == "func");
  REQUIRE(dir->options.size() == 2);
  CHECK(dir->options[0] == "opt1");
  CHECK(dir->options[1] == "opt2");
  REQUIRE(dir->params.size() == 2);
}

TEST_CASE("Parser: variable simple", "[parser]") {
  auto ast = parse_template("Hello $name!");
  REQUIRE(ast.children.size() == 3); // Text + VarSimple + Text
  CHECK(std::holds_alternative<TextNode>(*ast.children[0]));
  CHECK(std::holds_alternative<VarNode>(*ast.children[1]));
  CHECK(std::holds_alternative<TextNode>(*ast.children[2]));
}

TEST_CASE("Parser: variable bracket", "[parser]") {
  auto ast = parse_template("$[title]");
  REQUIRE(ast.children.size() == 1);
  auto* var = std::get_if<VarNode>(ast.children[0].get());
  REQUIRE(var);
  CHECK(var->kind == VarKind::Bracket);
  CHECK(var->name == "title");
}

TEST_CASE("Parser: variable expression", "[parser]") {
  auto ast = parse_template("$`x + 1`");
  REQUIRE(ast.children.size() == 1);
  auto* var = std::get_if<VarNode>(ast.children[0].get());
  REQUIRE(var);
  CHECK(var->kind == VarKind::Expr);
  CHECK(var->name == "x + 1");
}

TEST_CASE("Parser: block directive", "[parser]") {
  auto ast = parse_template("@if(show) {\nHello\n}");
  REQUIRE(ast.children.size() == 1);
  auto* block = std::get_if<BlockNode>(ast.children[0].get());
  REQUIRE(block);
  CHECK(block->name == "if");
  REQUIRE(block->params.size() == 1);
  CHECK(block->params[0] == "show");
  REQUIRE(block->body.size() >= 1);
}

TEST_CASE("Parser: block with elif", "[parser]") {
  auto ast = parse_template("@if(x) {\nA\n} elif(y) {\nB\n}");
  REQUIRE(ast.children.size() == 1);
  auto* block = std::get_if<BlockNode>(ast.children[0].get());
  REQUIRE(block);
  CHECK(block->name == "if");
  REQUIRE(block->elif_branches.size() == 1);
}

TEST_CASE("Parser: block with else", "[parser]") {
  auto ast = parse_template("@if(x) {\nA\n} else {\nB\n}");
  REQUIRE(ast.children.size() == 1);
  auto* block = std::get_if<BlockNode>(ast.children[0].get());
  REQUIRE(block);
  CHECK(block->has_else == true);
  REQUIRE(block->else_body.size() >= 1);
}

TEST_CASE("Parser: nested blocks", "[parser]") {
  auto ast = parse_template("@if(a) {\n@for(i; 0; 3) {\nItem\n}\n}");
  REQUIRE(ast.children.size() == 1);
  auto* outer = std::get_if<BlockNode>(ast.children[0].get());
  REQUIRE(outer);
  CHECK(outer->name == "if");
  REQUIRE(outer->body.size() >= 1);
}

TEST_CASE("Parser: comment preserved", "[parser]") {
  auto ast = parse_template("@# a comment\nreal content");
  REQUIRE(ast.children.size() == 2);
  CHECK(std::holds_alternative<CommentNode>(*ast.children[0]));
  CHECK(std::holds_alternative<TextNode>(*ast.children[1]));
}

TEST_CASE("Parser: empty input", "[parser]") {
  auto ast = parse_template("");
  CHECK(ast.children.empty());
}

TEST_CASE("Parser: multiple directives", "[parser]") {
  auto ast = parse_template("@def(x, 1)\n@def(y, 2)\n@write($x $y)");
  REQUIRE(ast.children.size() >= 3);
}

TEST_CASE("Parser: directive no params", "[parser]") {
  auto ast = parse_template("@pwd");
  REQUIRE(ast.children.size() == 1);
  auto* dir = std::get_if<DirectiveNode>(ast.children[0].get());
  REQUIRE(dir);
  CHECK(dir->name == "pwd");
  CHECK(dir->params.empty());
}

TEST_CASE("Parser: escape in text", "[parser]") {
  auto ast = parse_template("before \\@after");
  REQUIRE(ast.children.size() >= 1);
  // Both parts are text
  for (auto& child : ast.children) {
    CHECK(std::holds_alternative<TextNode>(*child));
  }
}

TEST_CASE("Parser: complex template", "[parser]") {
  std::string src = R"(@#-- Page template --#
<html>
<head><title>$title</title></head>
<body>
@input(nav.html)
@if(show_content) {
  @content
} else {
  <p>No content</p>
}
@input(footer.html)
</body>
</html>)";
  auto ast = parse_template(src);
  REQUIRE(ast.children.size() >= 5);
}
