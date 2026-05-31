/// @file test_expr.cpp

#include <catch2/catch_test_macros.hpp>

#include "nift/runtime/expr.hpp"

using namespace nift::runtime;

TEST_CASE("expr: integer literal", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("42", ctx);
  REQUIRE(r);
  CHECK(*r == "42");
}

TEST_CASE("expr: addition", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("1 + 2", ctx);
  REQUIRE(r);
  CHECK(*r == "3");
}

TEST_CASE("expr: precedence", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("2 + 3 * 4", ctx);
  REQUIRE(r);
  CHECK(*r == "14");
}

TEST_CASE("expr: parentheses", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("(2 + 3) * 4", ctx);
  REQUIRE(r);
  CHECK(*r == "20");
}

TEST_CASE("expr: division", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("10 / 4", ctx);
  REQUIRE(r);
  CHECK(*r == "2.5");
}

TEST_CASE("expr: modulo", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("10 % 3", ctx);
  REQUIRE(r);
  CHECK(*r == "1");
}

TEST_CASE("expr: string concat", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("\"hello\" .. \" \" .. \"world\"", ctx);
  REQUIRE(r);
  CHECK(*r == "hello world");
}

TEST_CASE("expr: equality numeric", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("3 == 3", ctx);
  REQUIRE(r);
  CHECK(*r == "true");
}

TEST_CASE("expr: inequality", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("3 != 4", ctx);
  REQUIRE(r);
  CHECK(*r == "true");
}

TEST_CASE("expr: less than", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("3 < 5", ctx);
  REQUIRE(r);
  CHECK(*r == "true");
}

TEST_CASE("expr: greater equal", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("5 >= 5", ctx);
  REQUIRE(r);
  CHECK(*r == "true");
}

TEST_CASE("expr: logical and", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("true && false", ctx);
  REQUIRE(r);
  CHECK(*r == "false");
}

TEST_CASE("expr: logical or", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("true || false", ctx);
  REQUIRE(r);
  CHECK(*r == "true");
}

TEST_CASE("expr: logical not", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("!false", ctx);
  REQUIRE(r);
  CHECK(*r == "true");
}

TEST_CASE("expr: variable lookup", "[runtime][expr]") {
  ExprContext ctx{{"x", "10"}, {"y", "5"}};
  auto r = eval_expr("x + y", ctx);
  REQUIRE(r);
  CHECK(*r == "15");
}

TEST_CASE("expr: string variable", "[runtime][expr]") {
  ExprContext ctx{{"name", "nift"}};
  auto r = eval_expr("name .. \"!\"", ctx);
  REQUIRE(r);
  CHECK(*r == "nift!");
}

TEST_CASE("expr: missing variable returns empty string", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_expr("missing", ctx);
  REQUIRE(r);
  CHECK(*r == "");
}

TEST_CASE("expr: eval_bool truthy non-empty", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_bool("\"yes\"", ctx);
  REQUIRE(r);
  CHECK(*r == true);
}

TEST_CASE("expr: eval_bool falsy zero", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_bool("0", ctx);
  REQUIRE(r);
  CHECK(*r == false);
}

TEST_CASE("expr: eval_bool falsy empty", "[runtime][expr]") {
  ExprContext ctx;
  auto r = eval_bool("\"\"", ctx);
  REQUIRE(r);
  CHECK(*r == false);
}

TEST_CASE("expr: chained comparison and logic", "[runtime][expr]") {
  ExprContext ctx{{"age", "25"}};
  auto r = eval_expr("age >= 18 && age < 65", ctx);
  REQUIRE(r);
  CHECK(*r == "true");
}
