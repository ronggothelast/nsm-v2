/// @file ast.cpp
/// @brief AST utilities — pretty-print.

#include "nift/parser/ast.hpp"

#include <sstream>

namespace nift::parser {

namespace {

std::string indent_str(int level) {
  return std::string(static_cast<size_t>(level) * 2, ' ');
}

void node_to_string(std::ostringstream& os, const Node& node, int depth);

void children_to_string(std::ostringstream& os,
                        const std::vector<NodePtr>& children,
                        int depth) {
  for (const auto& child : children) {
    node_to_string(os, *child, depth);
  }
}

void node_to_string(std::ostringstream& os, const Node& node, int depth) {
  std::string pad = indent_str(depth);
  std::visit([&](const auto& n) {
    using T = std::decay_t<decltype(n)>;

    if constexpr (std::is_same_v<T, TextNode>) {
      os << pad << "Text(\"" << n.text << "\") @L" << n.line << "\n";
    }
    else if constexpr (std::is_same_v<T, VarNode>) {
      const char* kind = n.kind == VarKind::Simple ? "Simple"
                       : n.kind == VarKind::Bracket ? "Bracket"
                       : "Expr";
      os << pad << "Var." << kind << "(\"" << n.name << "\") @L" << n.line << "\n";
    }
    else if constexpr (std::is_same_v<T, DirectiveNode>) {
      os << pad << "Directive @" << n.name;
      if (!n.options.empty()) {
        os << "{";
        for (size_t i = 0; i < n.options.size(); ++i) {
          if (i) os << ", ";
          os << n.options[i];
        }
        os << "}";
      }
      if (!n.params.empty()) {
        os << "(";
        for (size_t i = 0; i < n.params.size(); ++i) {
          if (i) os << ", ";
          os << n.params[i];
        }
        os << ")";
      }
      os << " @L" << n.line << "\n";
    }
    else if constexpr (std::is_same_v<T, BlockNode>) {
      os << pad << "Block @" << n.name;
      if (!n.options.empty()) {
        os << "{";
        for (size_t i = 0; i < n.options.size(); ++i) {
          if (i) os << ", ";
          os << n.options[i];
        }
        os << "}";
      }
      if (!n.params.empty()) {
        os << "(";
        for (size_t i = 0; i < n.params.size(); ++i) {
          if (i) os << ", ";
          os << n.params[i];
        }
        os << ")";
      }
      os << " @L" << n.line << "\n";
      os << pad << "  body:\n";
      children_to_string(os, n.body, depth + 2);
      for (const auto& elif : n.elif_branches) {
        os << pad << "  elif(";
        for (size_t i = 0; i < elif.conditions.size(); ++i) {
          if (i) os << "; ";
          os << elif.conditions[i];
        }
        os << ") @L" << elif.line << ":\n";
        children_to_string(os, elif.body, depth + 2);
      }
      if (n.has_else) {
        os << pad << "  else:\n";
        children_to_string(os, n.else_body, depth + 2);
      }
    }
    else if constexpr (std::is_same_v<T, CommentNode>) {
      os << pad << "Comment(\"" << n.text << "\") @L" << n.line << "\n";
    }
    else if constexpr (std::is_same_v<T, ProgramNode>) {
      os << pad << "Program:\n";
      children_to_string(os, n.children, depth + 1);
    }
  }, node);
}

} // anonymous namespace

std::string ast_to_string(const Node& node, int indent) {
  std::ostringstream os;
  node_to_string(os, node, indent);
  return os.str();
}

} // namespace nift::parser
