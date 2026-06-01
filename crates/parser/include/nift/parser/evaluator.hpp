#pragma once
/// @file evaluator.hpp
/// @brief Evaluator — walks the AST and produces output string.
///
/// The evaluator implements the Visitor pattern. It:
///   - Resolves variables from a VariableStore
///   - Dispatches directives to registered handlers
///   - Handles control flow (if/for/while)
///   - Produces the final output string

#include <functional>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "nift/parser/ast.hpp"

namespace nift::parser {

/// Forward-declared context for directive handlers.
struct EvalContext;

/// Result of evaluating a directive.
enum class EvalResult {
  Ok,      ///< Success
  Skip,    ///< Skip (e.g., @continue)
  Break,   ///< Break out of loop
  Return,  ///< Return from function
  Error,   ///< Error occurred
};

/// Signature for a directive handler function.
/// Takes context + directive node → modifies context's output.
using DirectiveHandler = std::function<EvalResult(EvalContext&, const DirectiveNode&)>;
using BlockHandler = std::function<EvalResult(EvalContext&, const BlockNode&)>;

/// Evaluation context — mutable state during AST traversal.
struct EvalContext {
  std::string output;  ///< Accumulated output
  std::unordered_map<std::string, std::string> variables;
  std::unordered_map<std::string, DirectiveHandler> directive_handlers;
  std::unordered_map<std::string, BlockHandler> block_handlers;
  std::ostream* error_stream = nullptr;  ///< Error output
  std::string current_file;              ///< For diagnostics
  int current_line = 0;

  /// Set a variable.
  void set_var(std::string name, std::string value);

  /// Get a variable. Returns empty string if not found.
  [[nodiscard]] std::string_view get_var(std::string_view name) const;

  /// Check if variable exists.
  [[nodiscard]] bool has_var(std::string_view name) const;

  /// Register default directive handlers (@input, @include, @if, @for, etc.)
  void register_defaults();
};

/// Evaluator — visitor that walks AST and produces output.
class Evaluator : public Visitor<EvalResult> {
 public:
  explicit Evaluator(EvalContext& ctx) : ctx_(ctx) {}

  // ─── Visitor interface ─────────────────────────────────────────
  EvalResult visit(const TextNode&) override;
  EvalResult visit(const VarNode&) override;
  EvalResult visit(const DirectiveNode&) override;
  EvalResult visit(const BlockNode&) override;
  EvalResult visit(const CommentNode&) override;
  EvalResult visit(const ProgramNode&) override;

 private:
  EvalContext& ctx_;

  /// Evaluate a string expression (simple variable interpolation).
  std::string eval_string(std::string_view template_str);

  /// Evaluate a boolean condition string.
  bool eval_condition(std::string_view condition);

  /// Log an error.
  void error(std::string_view message, int line = 0);
};

}  // namespace nift::parser
