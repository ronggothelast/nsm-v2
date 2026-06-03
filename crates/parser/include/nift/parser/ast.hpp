#pragma once
/// @file ast.hpp
/// @brief Abstract Syntax Tree for Nift templates.
///
/// The AST is a tree of Nodes produced by parsing the token stream.
/// Each node type represents a different template construct.
///
/// Node types:
///   - TextNode:       raw literal text
///   - VarNode:        $var, $[var], $`expr`
///   - DirectiveNode:  @name{opts}(args)
///   - BlockNode:      @name{opts}(args) { body } [elif/else]
///   - CommentNode:    @#-- block --# or @# line
///   - ProgramNode:    root of the tree (sequence of nodes)
///
/// Memory: AST nodes use a bump arena allocator (Phase 4).
/// For now, shared_ptr for simplicity during development.

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace nift::parser {

// Forward declarations
struct TextNode;
struct VarNode;
struct DirectiveNode;
struct BlockNode;
struct CommentNode;
struct ProgramNode;
struct ExtendsNode;
struct SectionNode;
struct YieldNode;
struct ParentNode;

/// Node variant — all possible AST node types.
using Node = std::variant<TextNode, VarNode, DirectiveNode, BlockNode, CommentNode,
                          ProgramNode, ExtendsNode, SectionNode, YieldNode, ParentNode>;

/// Unique pointer to a node (for tree ownership).
using NodePtr = std::shared_ptr<Node>;

/// Convenience: make a NodePtr from any node type.
template <typename T, typename... Args>
NodePtr make_node(Args&&... args) {
  return std::make_shared<Node>(T{std::forward<Args>(args)...});
}

// ─── Node types ──────────────────────────────────────────────────────

/// Raw text — not a directive or variable.
struct TextNode {
  std::string text;
  std::size_t line = 0;
};

/// Variable interpolation: $var, $[var], $`expr`
enum class VarKind { Simple, Bracket, Expr };

struct VarNode {
  VarKind kind;
  std::string name;  ///< Variable name or expression string
  std::size_t line = 0;
};

/// Directive invocation: @name{options}(params)
/// Params can be nested expressions (recursive).
struct DirectiveNode {
  std::string name;
  std::vector<std::string> options;
  std::vector<std::string> params;  ///< String params (pre-evaluation)
  std::size_t line = 0;
};

/// Block directive: @name{options}(params) { body } [elif(...) { }]* [else { }]
struct ElseIfBranch {
  std::vector<std::string> conditions;
  std::vector<NodePtr> body;
  std::size_t line = 0;
};

struct BlockNode {
  std::string name;
  std::vector<std::string> options;
  std::vector<std::string> params;
  std::vector<NodePtr> body;

  // For @if: elif/else branches
  std::vector<ElseIfBranch> elif_branches;
  std::vector<NodePtr> else_body;
  bool has_else = false;

  std::size_t line = 0;
};

/// Comment (preserved for source maps, dropped during evaluation).
struct CommentNode {
  std::string text;
  std::size_t line = 0;
};

/// Root node — sequence of child nodes.
struct ProgramNode {
  std::vector<NodePtr> children;
};

/// Template inheritance: @extends("layout.html")
struct ExtendsNode {
  std::string layout_path;
  std::size_t line = 0;
};

/// Named section: @section("name")...@endsection
struct SectionNode {
  std::string name;
  std::vector<NodePtr> body;
  std::size_t line = 0;
};

/// Yield point in layout: @yield("name")
struct YieldNode {
  std::string name;
  std::size_t line = 0;
};

/// Parent reference in section: @parent
struct ParentNode {
  std::size_t line = 0;
};

// ─── Visitor pattern ─────────────────────────────────────────────────

/// Visitor interface — implement to walk the AST.
/// Each visit method handles one node type.
template <typename ReturnType = void>
struct Visitor {
  virtual ~Visitor() = default;
  virtual ReturnType visit(const TextNode&) = 0;
  virtual ReturnType visit(const VarNode&) = 0;
  virtual ReturnType visit(const DirectiveNode&) = 0;
  virtual ReturnType visit(const BlockNode&) = 0;
  virtual ReturnType visit(const CommentNode&) = 0;
  virtual ReturnType visit(const ProgramNode&) = 0;
  virtual ReturnType visit(const ExtendsNode&) = 0;
  virtual ReturnType visit(const SectionNode&) = 0;
  virtual ReturnType visit(const YieldNode&) = 0;
  virtual ReturnType visit(const ParentNode&) = 0;
};

/// Accept visitor — dispatches to the correct visit() overload.
template <typename R>
R accept(Visitor<R>& visitor, const Node& node) {
  return std::visit([&visitor](const auto& n) -> R { return visitor.visit(n); }, node);
}

// ─── AST utilities ───────────────────────────────────────────────────

/// Pretty-print an AST node to string (for debugging).
std::string ast_to_string(const Node& node, int indent = 0);

}  // namespace nift::parser
