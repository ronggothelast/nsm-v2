/// @file test_evaluator.cpp
/// @brief Tests for the Nift template evaluator.

#include <catch2/catch_test_macros.hpp>

#include "nift/parser/evaluator.hpp"
#include "nift/parser/parser.hpp"

using namespace nift::parser;

TEST_CASE("Evaluator: plain text pass-through", "[evaluator]") {
  auto result = evaluate_template("Hello, world!");
  CHECK(result == "Hello, world!");
}

TEST_CASE("Evaluator: variable substitution", "[evaluator]") {
  auto result = evaluate_template("Hello $name!", {{"name", "World"}});
  CHECK(result == "Hello World!");
}

TEST_CASE("Evaluator: bracket variable", "[evaluator]") {
  auto result = evaluate_template("$[title]", {{"title", "My Page"}});
  CHECK(result == "My Page");
}

TEST_CASE("Evaluator: missing variable", "[evaluator]") {
  auto result = evaluate_template("Hello $missing!");
  CHECK(result == "Hello !");
}

TEST_CASE("Evaluator: multiple variables", "[evaluator]") {
  auto result = evaluate_template("$greeting $name!", {{"greeting", "Hi"}, {"name", "Alice"}});
  CHECK(result == "Hi Alice!");
}

TEST_CASE("Evaluator: if true", "[evaluator]") {
  auto result = evaluate_template("@if(show) {\nvisible\n}", {{"show", "1"}});
  CHECK(result.find("visible") != std::string::npos);
}

TEST_CASE("Evaluator: if false", "[evaluator]") {
  auto result = evaluate_template("@if(show) {\nvisible\n} else {\nhidden\n}", {{"show", "0"}});
  CHECK(result.find("hidden") != std::string::npos);
}

TEST_CASE("Evaluator: if with elif", "[evaluator]") {
  auto result = evaluate_template(
      "@if(a) {\nA\n} elif(b) {\nB\n} else {\nC\n}",
      {{"a", "0"}, {"b", "1"}});
  CHECK(result.find("B") != std::string::npos);
}

TEST_CASE("Evaluator: if all false → else", "[evaluator]") {
  auto result = evaluate_template(
      "@if(a) {\nA\n} elif(b) {\nB\n} else {\nC\n}",
      {{"a", "0"}, {"b", "0"}});
  CHECK(result.find("C") != std::string::npos);
}

TEST_CASE("Evaluator: for loop", "[evaluator]") {
  auto result = evaluate_template("@for(i; 0; 3) {\nItem $i\n}");
  CHECK(result == "Item 0\nItem 1\nItem 2\n");
}

TEST_CASE("Evaluator: while loop", "[evaluator]") {
  auto result = evaluate_template(
      "@def(counter, 3)\n@while(counter) {\nTick\n@def(counter, 0)\n}");
  CHECK(result.find("Tick") != std::string::npos);
}

TEST_CASE("Evaluator: comment dropped", "[evaluator]") {
  auto result = evaluate_template("@# comment\nvisible");
  CHECK(result.find("visible") != std::string::npos);
}

TEST_CASE("Evaluator: block comment dropped", "[evaluator]") {
  auto result = evaluate_template("@#-- block --#visible");
  CHECK(result == "visible");
}

TEST_CASE("Evaluator: escape sequences", "[evaluator]") {
  auto result = evaluate_template("\\@literal");
  CHECK(result == "@literal");
}

TEST_CASE("Evaluator: @write directive", "[evaluator]") {
  auto result = evaluate_template("@write(hello world)");
  CHECK(result == "hello world");
}

TEST_CASE("Evaluator: @set then use", "[evaluator]") {
  auto result = evaluate_template("@set(x, hello)\n@write($x)");
  // @set with first param "x" and second "hello" → sets x = "hello"
  // Then newline + @write($x) outputs "hello"
  CHECK(result.find("hello") != std::string::npos);
}

TEST_CASE("Evaluator: @def then use", "[evaluator]") {
  auto result = evaluate_template("@def(count, 42)\nThe count is $count");
  CHECK(result.find("The count is") != std::string::npos);
  CHECK(result.find("42") != std::string::npos);
}

TEST_CASE("Evaluator: nested blocks", "[evaluator]") {
  auto result = evaluate_template(
      "@if(show) {\n@for(i; 0; 2) {\nItem $i\n}\n}",
      {{"show", "1"}});
  CHECK(result.find("Item 0") != std::string::npos);
  CHECK(result.find("Item 1") != std::string::npos);
}

TEST_CASE("Evaluator: @input placeholder", "[evaluator]") {
  auto result = evaluate_template("@input(header.html)");
  CHECK(result == "[input:header.html]");
}

TEST_CASE("Evaluator: @include placeholder", "[evaluator]") {
  auto result = evaluate_template("@include(nav.html)");
  CHECK(result == "[input:nav.html]");
}

TEST_CASE("Evaluator: @content placeholder", "[evaluator]") {
  auto result = evaluate_template("@content");
  CHECK(result.find("content") != std::string::npos); // handler output
}

TEST_CASE("Evaluator: mixed content", "[evaluator]") {
  auto result = evaluate_template(
      "<html>\n<head><title>$title</title></head>\n<body>\n@if(show) {\n<p>Hello</p>\n}\n</body>\n</html>",
      {{"title", "Test"}, {"show", "1"}});
  CHECK(result.find("<title>Test</title>") != std::string::npos);
  CHECK(result.find("<p>Hello</p>") != std::string::npos);
}

TEST_CASE("Evaluator: empty template", "[evaluator]") {
  auto result = evaluate_template("");
  CHECK(result.empty());
}

TEST_CASE("Evaluator: no variables", "[evaluator]") {
  auto result = evaluate_template("static content only");
  CHECK(result == "static content only");
}

TEST_CASE("Evaluator: @exit stops evaluation", "[evaluator]") {
  auto result = evaluate_template("before @exit after");
  CHECK(result == "before ");
}

TEST_CASE("Evaluator: unknown directive passthrough", "[evaluator]") {
  auto result = evaluate_template("@unknown(args)");
  CHECK(result.find("unknown") != std::string::npos);
}

TEST_CASE("Evaluator: context variables persist across directives", "[evaluator]") {
  EvalContext ctx;
  ctx.register_defaults();
  ctx.set_var("x", "10");

  auto ast = parse_template("@set(y, $x)\nResult: $y");
  Evaluator eval(ctx);
  accept(eval, ast);

  CHECK(ctx.variables.count("y") == 1);
}

TEST_CASE("Evaluator: visitor pattern works", "[evaluator]") {
  auto ast = parse_template("Hello $name!");
  EvalContext ctx;
  ctx.register_defaults();
  ctx.set_var("name", "World");

  Evaluator eval(ctx);
  auto result = accept(eval, ast);
  CHECK(result == EvalResult::Ok);
  CHECK(ctx.output == "Hello World!");
}

TEST_CASE("Evaluator: 50 fixture — complete page template", "[evaluator][snapshot]") {
  std::string template_src = R"(@#-- Page Template --#
<!DOCTYPE html>
<html>
<head>
  <title>$title</title>
  <meta name="description" content="$description">
</head>
<body>
  <header>@input(header.html)</header>
  <main>
    @if(has_content) {
      @content
    } else {
      <p>No content available.</p>
    }
  </main>
  <footer>@input(footer.html)</footer>
</body>
</html>)";

  std::unordered_map<std::string, std::string> vars = {
    {"title", "My Page"},
    {"description", "A test page"},
    {"has_content", "1"}
  };

  auto result = evaluate_template(template_src, vars);
  CHECK(result.find("<title>My Page</title>") != std::string::npos);
  CHECK(result.find("A test page") != std::string::npos);
  CHECK(result.find("[input:header.html]") != std::string::npos);
  CHECK(result.find("[input:footer.html]") != std::string::npos);
  CHECK(result.find("<p>No content available.</p>") == std::string::npos);
}
