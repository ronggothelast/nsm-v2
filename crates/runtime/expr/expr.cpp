/// @file expr.cpp
/// @brief Expression evaluator — minimal recursive-descent.
///
/// Grammar (Phase 3, expanded in Phase 4):
///   expr     = or_expr
///   or_expr  = and_expr ("||" and_expr)*
///   and_expr = cmp_expr ("&&" cmp_expr)*
///   cmp_expr = add_expr (("==" | "!=" | "<" | ">" | "<=" | ">=") add_expr)?
///   add_expr = mul_expr (("+" | "-" | "..") mul_expr)*
///   mul_expr = unary (("*" | "/" | "%") unary)*
///   unary    = "!" unary | "-" unary | atom
///   atom     = NUMBER | STRING | IDENT | "(" expr ")"

#include "nift/runtime/expr.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>

namespace nift::runtime {

namespace {

struct Parser {
  std::string_view src;
  std::size_t pos = 0;
  const ExprContext* ctx = nullptr;
  bool ok = true;

  void skip_ws() {
    while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])))
      ++pos;
  }

  bool match(std::string_view tok) {
    skip_ws();
    if (pos + tok.size() > src.size())
      return false;
    if (src.compare(pos, tok.size(), tok) != 0)
      return false;
    pos += tok.size();
    return true;
  }

  // Returns serialized value as string; numeric ops normalize.
  std::string number_to_str(double d) {
    if (std::isnan(d) || std::isinf(d))
      return "0";
    if (d == static_cast<long long>(d)) {
      return std::to_string(static_cast<long long>(d));
    }
    std::ostringstream os;
    os << d;
    return os.str();
  }

  bool to_double(const std::string& s, double& out) {
    if (s.empty()) {
      out = 0;
      return false;
    }
    char* endp = nullptr;
    out = std::strtod(s.c_str(), &endp);
    return endp == s.c_str() + s.size();
  }

  bool truthy(const std::string& s) {
    if (s.empty())
      return false;
    if (s == "0" || s == "false" || s == "nil")
      return false;
    return true;
  }

  std::string parse_atom() {
    skip_ws();
    if (pos >= src.size()) {
      ok = false;
      return "";
    }

    char c = src[pos];

    // Parenthesized
    if (c == '(') {
      ++pos;
      auto v = parse_expr();
      skip_ws();
      if (pos < src.size() && src[pos] == ')')
        ++pos;
      else
        ok = false;
      return v;
    }

    // String literal: "..." or '...'
    if (c == '"' || c == '\'') {
      char quote = c;
      ++pos;
      std::string s;
      while (pos < src.size() && src[pos] != quote) {
        if (src[pos] == '\\' && pos + 1 < src.size()) {
          char nx = src[pos + 1];
          if (nx == 'n')
            s += '\n';
          else if (nx == 't')
            s += '\t';
          else
            s += nx;
          pos += 2;
        } else {
          s += src[pos++];
        }
      }
      if (pos < src.size())
        ++pos;  // closing quote
      else
        ok = false;
      return s;
    }

    // Number
    if (std::isdigit(static_cast<unsigned char>(c)) ||
        (c == '.' && pos + 1 < src.size() &&
         std::isdigit(static_cast<unsigned char>(src[pos + 1])))) {
      std::size_t start = pos;
      while (pos < src.size() &&
             (std::isdigit(static_cast<unsigned char>(src[pos])) || src[pos] == '.')) {
        ++pos;
      }
      return std::string(src.substr(start, pos - start));
    }

    // Identifier — variable lookup
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      std::size_t start = pos;
      while (pos < src.size() &&
             (std::isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_')) {
        ++pos;
      }
      std::string name(src.substr(start, pos - start));
      if (name == "true")
        return "true";
      if (name == "false")
        return "false";
      if (ctx) {
        auto it = ctx->find(name);
        if (it != ctx->end())
          return it->second;
      }
      return "";
    }

    ok = false;
    return "";
  }

  std::string parse_unary() {
    skip_ws();
    if (match("!")) {
      auto v = parse_unary();
      return truthy(v) ? "false" : "true";
    }
    if (pos < src.size() && src[pos] == '-' &&
        (pos + 1 >= src.size() ||
         !std::isspace(static_cast<unsigned char>(src[pos + 1])))) {
      // Negation only if followed directly by a number/ident
      if (pos + 1 < src.size() &&
          (std::isdigit(static_cast<unsigned char>(src[pos + 1])) ||
           std::isalpha(static_cast<unsigned char>(src[pos + 1])) ||
           src[pos + 1] == '(' || src[pos + 1] == '_')) {
        ++pos;
        auto v = parse_unary();
        double d = 0;
        if (to_double(v, d))
          return number_to_str(-d);
        return v;
      }
    }
    return parse_atom();
  }

  std::string parse_mul() {
    auto lhs = parse_unary();
    while (true) {
      skip_ws();
      if (match("*") || match("/") || match("%")) {
        char op = src[pos - 1];
        auto rhs = parse_unary();
        double a = 0, b = 0;
        if (!to_double(lhs, a) || !to_double(rhs, b)) {
          ok = false;
          return "";
        }
        if (op == '*')
          lhs = number_to_str(a * b);
        else if (op == '/') {
          if (b == 0) {
            ok = false;
            return "";
          }
          lhs = number_to_str(a / b);
        } else {
          if (b == 0) {
            ok = false;
            return "";
          }
          lhs = number_to_str(std::fmod(a, b));
        }
      } else
        break;
    }
    return lhs;
  }

  std::string parse_add() {
    auto lhs = parse_mul();
    while (true) {
      skip_ws();
      if (match("..")) {
        auto rhs = parse_mul();
        lhs = lhs + rhs;
      } else if (match("+") || match("-")) {
        char op = src[pos - 1];
        auto rhs = parse_mul();
        double a = 0, b = 0;
        if (!to_double(lhs, a) || !to_double(rhs, b)) {
          ok = false;
          return "";
        }
        lhs = (op == '+') ? number_to_str(a + b) : number_to_str(a - b);
      } else
        break;
    }
    return lhs;
  }

  std::string parse_cmp() {
    auto lhs = parse_add();
    skip_ws();
    std::string op_str;
    if (match("=="))
      op_str = "==";
    else if (match("!="))
      op_str = "!=";
    else if (match("<="))
      op_str = "<=";
    else if (match(">="))
      op_str = ">=";
    else if (match("<"))
      op_str = "<";
    else if (match(">"))
      op_str = ">";
    else
      return lhs;

    auto rhs = parse_add();
    double a = 0, b = 0;
    bool numeric = to_double(lhs, a) && to_double(rhs, b);
    bool result = false;
    if (numeric) {
      if (op_str == "==")
        result = a == b;
      else if (op_str == "!=")
        result = a != b;
      else if (op_str == "<")
        result = a < b;
      else if (op_str == ">")
        result = a > b;
      else if (op_str == "<=")
        result = a <= b;
      else if (op_str == ">=")
        result = a >= b;
    } else {
      if (op_str == "==")
        result = lhs == rhs;
      else if (op_str == "!=")
        result = lhs != rhs;
      else if (op_str == "<")
        result = lhs < rhs;
      else if (op_str == ">")
        result = lhs > rhs;
      else if (op_str == "<=")
        result = lhs <= rhs;
      else if (op_str == ">=")
        result = lhs >= rhs;
    }
    return result ? "true" : "false";
  }

  std::string parse_and() {
    auto lhs = parse_cmp();
    while (true) {
      skip_ws();
      if (match("&&")) {
        auto rhs = parse_cmp();
        lhs = (truthy(lhs) && truthy(rhs)) ? "true" : "false";
      } else
        break;
    }
    return lhs;
  }

  std::string parse_or() {
    auto lhs = parse_and();
    while (true) {
      skip_ws();
      if (match("||")) {
        auto rhs = parse_and();
        lhs = (truthy(lhs) || truthy(rhs)) ? "true" : "false";
      } else
        break;
    }
    return lhs;
  }

  std::string parse_expr() { return parse_or(); }
};

}  // namespace

::nift::Expected<std::string, ::nift::Error> eval_expr(std::string_view expr,
                                                       const ExprContext& ctx) {
  Parser p{expr, 0, &ctx, true};
  auto result = p.parse_expr();
  if (!p.ok) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::parse_error);
  }
  return result;
}

::nift::Expected<bool, ::nift::Error> eval_bool(std::string_view expr,
                                                const ExprContext& ctx) {
  auto r = eval_expr(expr, ctx);
  if (!r)
    return ::nift::unexpected<::nift::Error>(r.error());
  const auto& s = *r;
  if (s.empty() || s == "0" || s == "false" || s == "nil")
    return false;
  return true;
}

}  // namespace nift::runtime
