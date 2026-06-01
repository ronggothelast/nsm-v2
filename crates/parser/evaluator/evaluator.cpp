/// @file evaluator.cpp
/// @brief Evaluator implementation — walks AST, produces output.

#include "nift/parser/evaluator.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <iostream>
#include <sstream>

namespace nift::parser {

// ─── EvalContext ──────────────────────────────────────────────────────

void EvalContext::set_var(std::string name, std::string value) {
  variables[std::move(name)] = std::move(value);
}

std::string_view EvalContext::get_var(std::string_view name) const {
  auto it = variables.find(std::string(name));
  if (it != variables.end())
    return it->second;
  return {};
}

bool EvalContext::has_var(std::string_view name) const {
  return variables.count(std::string(name)) > 0;
}

// Default directive handlers
static EvalResult handle_input(EvalContext& ctx, const DirectiveNode& node) {
  if (node.params.empty()) {
    // @content with no path — Phase 2 placeholder
    ctx.output += "[content]";
    return EvalResult::Ok;
  }
  // Phase 2: placeholder — actual file I/O in Phase 3
  ctx.output += "[input:" + node.params[0] + "]";
  return EvalResult::Ok;
}

static EvalResult handle_write(EvalContext& ctx, const DirectiveNode& node) {
  for (const auto& p : node.params) {
    ctx.output += p;
  }
  return EvalResult::Ok;
}

static EvalResult handle_echo(EvalContext& ctx, const DirectiveNode& node) {
  for (size_t i = 0; i < node.params.size(); ++i) {
    if (i > 0)
      ctx.output += " ";
    ctx.output += node.params[i];
  }
  return EvalResult::Ok;
}

static EvalResult handle_set(EvalContext& ctx, const DirectiveNode& node) {
  if (node.params.size() >= 2) {
    ctx.set_var(node.params[0], node.params[1]);
  }
  return EvalResult::Ok;
}

static EvalResult handle_def(EvalContext& ctx, const DirectiveNode& node) {
  if (node.params.size() >= 2) {
    ctx.set_var(node.params[0], node.params[1]);
  } else if (node.params.size() == 1) {
    ctx.set_var(node.params[0], "");
  }
  return EvalResult::Ok;
}

static EvalResult handle_exit(EvalContext& /*ctx*/, const DirectiveNode& /*node*/) {
  return EvalResult::Return;
}

static EvalResult handle_continue(EvalContext& /*ctx*/, const DirectiveNode& /*node*/) {
  return EvalResult::Skip;
}

static EvalResult handle_break(EvalContext& /*ctx*/, const DirectiveNode& /*node*/) {
  return EvalResult::Break;
}

// Block handlers
static EvalResult handle_if(EvalContext& ctx, const BlockNode& node) {
  // Evaluate condition
  if (node.params.empty())
    return EvalResult::Error;

  bool condition = false;
  std::string cond_str = node.params[0];
  if (ctx.has_var(cond_str)) {
    auto val = ctx.get_var(cond_str);
    condition = !val.empty() && val != "0" && val != "false";
  } else {
    // Try numeric
    try {
      condition = std::stod(cond_str) != 0.0;
    } catch (...) {
      condition = !cond_str.empty() && cond_str != "0" && cond_str != "false";
    }
  }

  if (condition) {
    Evaluator eval(ctx);
    for (const auto& child : node.body) {
      auto result = accept(eval, *child);
      if (result != EvalResult::Ok)
        return result;
    }
    return EvalResult::Ok;
  }

  // Check elif branches
  for (const auto& elif : node.elif_branches) {
    if (!elif.conditions.empty()) {
      bool elif_cond = false;
      std::string elif_str = elif.conditions[0];
      if (ctx.has_var(elif_str)) {
        auto val = ctx.get_var(elif_str);
        elif_cond = !val.empty() && val != "0" && val != "false";
      } else {
        try {
          elif_cond = std::stod(elif_str) != 0.0;
        } catch (...) {
          elif_cond = !elif_str.empty() && elif_str != "0" && elif_str != "false";
        }
      }
      if (elif_cond) {
        Evaluator eval(ctx);
        for (const auto& child : elif.body) {
          auto result = accept(eval, *child);
          if (result != EvalResult::Ok)
            return result;
        }
        return EvalResult::Ok;
      }
    }
  }

  // Else
  if (node.has_else) {
    Evaluator eval(ctx);
    for (const auto& child : node.else_body) {
      auto result = accept(eval, *child);
      if (result != EvalResult::Ok)
        return result;
    }
  }

  return EvalResult::Ok;
}

static EvalResult handle_for(EvalContext& ctx, const BlockNode& node) {
  // @for(i = 0; i < 10; i++)
  // Simplified: @for(var; start; end) iterates var from start to end
  if (node.params.size() < 3)
    return EvalResult::Error;

  std::string var_name = node.params[0];
  int start = 0, end = 0;
  try {
    start = std::stoi(node.params[1]);
  } catch (...) {
    return EvalResult::Error;
  }
  try {
    end = std::stoi(node.params[2]);
  } catch (...) {
    return EvalResult::Error;
  }

  for (int i = start; i < end; ++i) {
    ctx.set_var(var_name, std::to_string(i));
    Evaluator eval(ctx);
    for (const auto& child : node.body) {
      auto result = accept(eval, *child);
      if (result == EvalResult::Break)
        return EvalResult::Ok;
      if (result == EvalResult::Skip)
        continue;
      if (result != EvalResult::Ok)
        return result;
    }
  }
  return EvalResult::Ok;
}

static EvalResult handle_while(EvalContext& ctx, const BlockNode& node) {
  if (node.params.empty())
    return EvalResult::Error;

  int max_iterations = 100000;  // safety limit
  while (max_iterations-- > 0) {
    std::string cond_str = node.params[0];
    bool condition = false;
    if (ctx.has_var(cond_str)) {
      auto val = ctx.get_var(cond_str);
      condition = !val.empty() && val != "0" && val != "false";
    } else {
      try {
        condition = std::stod(cond_str) != 0.0;
      } catch (...) {
        condition = !cond_str.empty() && cond_str != "0" && cond_str != "false";
      }
    }
    if (!condition)
      break;

    Evaluator eval(ctx);
    for (const auto& child : node.body) {
      auto result = accept(eval, *child);
      if (result == EvalResult::Break)
        return EvalResult::Ok;
      if (result == EvalResult::Skip)
        continue;
      if (result != EvalResult::Ok)
        return result;
    }
  }
  return EvalResult::Ok;
}

static EvalResult handle_function(EvalContext& ctx, const BlockNode& node) {
  // Register the function as a callable block
  // For Phase 2: store the body and re-evaluate on call
  // Simplified: just store as a named block handler
  std::string fn_name = node.params.empty() ? node.name : node.params[0];
  ctx.block_handlers[fn_name] = [&node](EvalContext& inner_ctx,
                                        const BlockNode& /*call*/) {
    Evaluator eval(inner_ctx);
    for (const auto& child : node.body) {
      auto result = accept(eval, *child);
      if (result != EvalResult::Ok)
        return result;
    }
    return EvalResult::Ok;
  };
  return EvalResult::Ok;
}

void EvalContext::register_defaults() {
  directive_handlers["input"] = handle_input;
  directive_handlers["inject"] = handle_input;
  directive_handlers["include"] = handle_input;
  directive_handlers["content"] = handle_input;
  directive_handlers["write"] = handle_write;
  directive_handlers["echo"] = handle_echo;
  directive_handlers["set"] = handle_set;
  directive_handlers["def"] = handle_def;
  directive_handlers["exit"] = handle_exit;
  directive_handlers["quit"] = handle_exit;
  directive_handlers["continue"] = handle_continue;
  directive_handlers["break"] = handle_break;

  block_handlers["if"] = handle_if;
  block_handlers["for"] = handle_for;
  block_handlers["while"] = handle_while;
  block_handlers["do-while"] = handle_while;
  block_handlers["function"] = handle_function;
  block_handlers["fn"] = handle_function;
}

// ─── Evaluator ───────────────────────────────────────────────────────

EvalResult Evaluator::visit(const TextNode& node) {
  // Process escape sequences
  std::string text = node.text;
  size_t pos = 0;
  while ((pos = text.find('\\', pos)) != std::string::npos) {
    if (pos + 1 < text.size()) {
      char next = text[pos + 1];
      if (next == '@') {
        text.replace(pos, 2, "@");
        pos += 1;
      } else if (next == '\\') {
        text.replace(pos, 2, "\\");
        pos += 1;
      } else {
        pos += 1;
      }
    } else {
      break;
    }
  }
  ctx_.output += text;
  ctx_.current_line = node.line;
  return EvalResult::Ok;
}

EvalResult Evaluator::visit(const VarNode& node) {
  switch (node.kind) {
    case VarKind::Simple:
    case VarKind::Bracket: {
      auto val = ctx_.get_var(node.name);
      if (!val.empty()) {
        ctx_.output += val;
      }
      break;
    }
    case VarKind::Expr: {
      // Phase 2: simple expression evaluation
      // Try variable lookup first
      auto val = ctx_.get_var(node.name);
      if (!val.empty()) {
        ctx_.output += val;
      }
      break;
    }
  }
  return EvalResult::Ok;
}

EvalResult Evaluator::visit(const DirectiveNode& node) {
  ctx_.current_line = node.line;

  // Resolve $var references in params
  std::vector<std::string> resolved_params;
  for (const auto& p : node.params) {
    if (p.size() > 1 && p[0] == '$') {
      resolved_params.push_back(std::string(ctx_.get_var(p.substr(1))));
    } else {
      resolved_params.push_back(p);
    }
  }

  DirectiveNode resolved = node;
  resolved.params = std::move(resolved_params);

  // Check registered handlers
  auto it = ctx_.directive_handlers.find(node.name);
  if (it != ctx_.directive_handlers.end()) {
    return it->second(ctx_, resolved);
  }

  // Check block handlers (might be a function call without body)
  auto bit = ctx_.block_handlers.find(node.name);
  if (bit != ctx_.block_handlers.end()) {
    BlockNode empty_block{node.name, node.options, node.params, {},
                          {},        {},           false,       node.line};
    return bit->second(ctx_, empty_block);
  }

  // Unknown directive — pass through as-is with warning
  ctx_.output += "@" + node.name;
  if (!node.params.empty()) {
    ctx_.output += "(";
    for (size_t i = 0; i < node.params.size(); ++i) {
      if (i > 0)
        ctx_.output += ", ";
      ctx_.output += node.params[i];
    }
    ctx_.output += ")";
  }
  return EvalResult::Ok;
}

EvalResult Evaluator::visit(const BlockNode& node) {
  ctx_.current_line = node.line;

  // Check registered block handlers
  auto it = ctx_.block_handlers.find(node.name);
  if (it != ctx_.block_handlers.end()) {
    return it->second(ctx_, node);
  }

  // Unknown block — evaluate body as-is
  for (const auto& child : node.body) {
    auto result = accept(*this, *child);
    if (result != EvalResult::Ok)
      return result;
  }
  return EvalResult::Ok;
}

EvalResult Evaluator::visit(const CommentNode& /*node*/) {
  // Comments are dropped during evaluation
  return EvalResult::Ok;
}

EvalResult Evaluator::visit(const ProgramNode& node) {
  for (const auto& child : node.children) {
    auto result = accept(*this, *child);
    if (result == EvalResult::Return)
      break;
    if (result == EvalResult::Error)
      return result;
  }
  return EvalResult::Ok;
}

std::string Evaluator::eval_string(std::string_view /*template_str*/) {
  // Phase 2: placeholder for string interpolation
  return std::string(/*template_str*/);
}

bool Evaluator::eval_condition(std::string_view condition) {
  if (ctx_.has_var(condition)) {
    auto val = ctx_.get_var(condition);
    return !val.empty() && val != "0" && val != "false";
  }
  try {
    return std::stod(std::string(condition)) != 0.0;
  } catch (...) {
    return !condition.empty() && condition != "0" && condition != "false";
  }
}

void Evaluator::error(std::string_view message, std::size_t line) {
  if (ctx_.error_stream) {
    *ctx_.error_stream << "Error at line " << line << ": " << message << "\n";
  }
}

}  // namespace nift::parser
