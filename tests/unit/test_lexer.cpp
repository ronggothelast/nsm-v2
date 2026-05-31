/// @file test_lexer.cpp
/// @brief Tests for the Nift template lexer.

#include <catch2/catch_test_macros.hpp>

#include "nift/parser/lexer.hpp"

using namespace nift::parser;

TEST_CASE("Lexer: plain text", "[lexer]") {
  // Text-only input: @ and $ are the only text-breakers
  auto tokens = Lexer("Hello, world!").tokenize();
  REQUIRE(tokens.size() == 2); // Text + Eof
  CHECK(tokens[0].type == TokenType::Text);
  CHECK(tokens[0].value == "Hello, world!");
}

TEST_CASE("Lexer: simple directive", "[lexer]") {
  auto tokens = Lexer("@input").tokenize();
  REQUIRE(tokens.size() == 2); // Directive + Eof
  CHECK(tokens[0].type == TokenType::Directive);
  CHECK(tokens[0].value == "input");
}

TEST_CASE("Lexer: directive with options", "[lexer]") {
  auto tokens = Lexer("@func{opt1, opt2}").tokenize();
  REQUIRE(tokens.size() == 3); // Directive + Text("{opt1, opt2}") + Eof
  CHECK(tokens[0].type == TokenType::Directive);
  CHECK(tokens[0].value == "func");
  CHECK(tokens[1].type == TokenType::Text);
  // The {opt1, opt2} is parsed by the parser, not the lexer
  CHECK(tokens[1].value.find('{') != std::string::npos);
}

TEST_CASE("Lexer: variable simple", "[lexer]") {
  auto tokens = Lexer("Hello $name!").tokenize();
  REQUIRE(tokens.size() == 4); // Text + VarSimple + Text + Eof
  CHECK(tokens[0].type == TokenType::Text);
  CHECK(tokens[0].value == "Hello ");
  CHECK(tokens[1].type == TokenType::VarSimple);
  CHECK(tokens[1].value == "name");
  CHECK(tokens[2].type == TokenType::Text);
  CHECK(tokens[2].value == "!");
}

TEST_CASE("Lexer: variable bracket", "[lexer]") {
  auto tokens = Lexer("$[title]").tokenize();
  REQUIRE(tokens.size() == 2); // VarBracket + Eof
  CHECK(tokens[0].type == TokenType::VarBracket);
  CHECK(tokens[0].value == "title");
}

TEST_CASE("Lexer: variable expression", "[lexer]") {
  auto tokens = Lexer("$`x + 1`").tokenize();
  REQUIRE(tokens.size() == 2); // VarExpr + Eof
  CHECK(tokens[0].type == TokenType::VarExpr);
  CHECK(tokens[0].value == "x + 1");
}

TEST_CASE("Lexer: empty input", "[lexer]") {
  auto tokens = Lexer("").tokenize();
  REQUIRE(tokens.size() == 1);
  CHECK(tokens[0].type == TokenType::Eof);
}

TEST_CASE("Lexer: line comment", "[lexer]") {
  auto tokens = Lexer("@# this is a comment\nreal content").tokenize();
  REQUIRE(tokens.size() == 3); // LineComment + Text + Eof
  CHECK(tokens[0].type == TokenType::LineComment);
  CHECK(tokens[0].value == " this is a comment");
  CHECK(tokens[1].type == TokenType::Text);
  CHECK(tokens[1].value == "\nreal content");
}

TEST_CASE("Lexer: block comment", "[lexer]") {
  auto tokens = Lexer("@#-- this is a block comment --#visible").tokenize();
  REQUIRE(tokens.size() == 3); // BlockComment + Text + Eof
  CHECK(tokens[0].type == TokenType::BlockComment);
  CHECK(tokens[0].value == " this is a block comment ");
  CHECK(tokens[1].type == TokenType::Text);
  CHECK(tokens[1].value == "visible");
}

TEST_CASE("Lexer: escape sequences", "[lexer]") {
  auto tokens = Lexer("before \\@after").tokenize();
  REQUIRE(tokens.size() == 2); // Text + Eof (\@ included in text)
  CHECK(tokens[0].type == TokenType::Text);
  CHECK(tokens[0].value == "before \\@after");
}

TEST_CASE("Lexer: single @", "[lexer]") {
  auto tokens = Lexer("@").tokenize();
  REQUIRE(tokens.size() == 2); // Text + Eof
  CHECK(tokens[0].type == TokenType::Text);
  CHECK(tokens[0].value == "@");
}

TEST_CASE("Lexer: structural tokens", "[lexer]") {
  // Commas and parens inside directive args are parsed by the parser, not the lexer
  // So a bare "(a, b)" is just text
  auto tokens = Lexer("(a, b)").tokenize();
  REQUIRE(tokens.size() == 2);
  CHECK(tokens[0].type == TokenType::Text);
  CHECK(tokens[0].value == "(a, b)");
}

TEST_CASE("Lexer: mixed text and directives", "[lexer]") {
  auto tokens = Lexer("<title>$title</title>").tokenize();
  REQUIRE(tokens.size() == 4); // Text + VarSimple + Text + Eof
  CHECK(tokens[0].type == TokenType::Text);
  CHECK(tokens[0].value == "<title>");
  CHECK(tokens[1].type == TokenType::VarSimple);
  CHECK(tokens[1].value == "title");
  CHECK(tokens[2].type == TokenType::Text);
  CHECK(tokens[2].value == "</title>");
}

TEST_CASE("Lexer: operators", "[lexer]") {
  // Operators outside directives are text
  auto tokens = Lexer("x + y = z").tokenize();
  REQUIRE(tokens.size() == 2);
  CHECK(tokens[0].type == TokenType::Text);
  CHECK(tokens[0].value == "x + y = z");
}

TEST_CASE("Lexer: @// comment", "[lexer]") {
  auto tokens = Lexer("@// comment\n").tokenize();
  REQUIRE(tokens.size() == 3);
  CHECK(tokens[0].type == TokenType::LineComment);
  CHECK(tokens[0].value == " comment");
  CHECK(tokens[1].type == TokenType::Text);
  CHECK(tokens[1].value == "\n");
}

TEST_CASE("Lexer: line tracking", "[lexer]") {
  auto tokens = Lexer("line1\nline2\nline3").tokenize();
  CHECK(tokens[0].line == 1);
}

TEST_CASE("Lexer: text preserves newlines", "[lexer]") {
  auto tokens = Lexer("line1\nline2\nline3").tokenize();
  REQUIRE(tokens.size() == 2);
  CHECK(tokens[0].type == TokenType::Text);
  CHECK(tokens[0].value == "line1\nline2\nline3");
}
