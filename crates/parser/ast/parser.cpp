/// @file parser.cpp
/// @brief Parser — tokens → AST.

#include "nift/parser/parser.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "nift/parser/evaluator.hpp"

namespace nift::parser {

// ─── Construction ─────────────────────────────────────────────────

Parser::Parser(std::vector<Token> tokens, std::string_view filename)
    : tokens_(std::move(tokens)), filename_(filename) {
}

// ─── Token navigation ─────────────────────────────────────────────

const Token& Parser::peek() const {
  if (pos_ >= tokens_.size()) {
    static Token eof_token{TokenType::Eof, "", 0};
    return eof_token;
  }
  return tokens_[pos_];
}

const Token& Parser::advance() {
  const Token& tok = peek();
  if (pos_ < tokens_.size())
    ++pos_;
  return tok;
}

bool Parser::check(TokenType type) const {
  return peek().type == type;
}

bool Parser::is_at_end() const {
  return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::Eof;
}

// ─── Argument parsing helpers ─────────────────────────────────────

// Split a string by commas, respecting parentheses nesting
static std::vector<std::string> split_args(const std::string& text) {
  std::vector<std::string> args;
  std::string current;
  int depth = 0;
  for (char c : text) {
    if (c == '(') {
      ++depth;
      current += c;
    } else if (c == ')') {
      --depth;
      current += c;
    } else if ((c == ',' || c == ';') && depth == 0) {
      // Trim
      size_t start = current.find_first_not_of(" \t\n\r");
      size_t end = current.find_last_not_of(" \t\n\r");
      if (start != std::string::npos) {
        args.push_back(current.substr(start, end - start + 1));
      } else {
        args.push_back("");
      }
      current.clear();
    } else {
      current += c;
    }
  }
  if (!current.empty()) {
    size_t start = current.find_first_not_of(" \t\n\r");
    size_t end = current.find_last_not_of(" \t\n\r");
    if (start != std::string::npos) {
      args.push_back(current.substr(start, end - start + 1));
    } else {
      args.push_back("");
    }
  }
  return args;
}

// Parse @name{options}(params) from a sequence of tokens
// After consuming the Directive token, look at next tokens for { and (
//
// `name` is reserved for future per-directive parsing rules; the current
// implementation extracts options/params positionally without needing it.
void Parser::parse_directive_args(std::string& /*name*/,
                                  std::vector<std::string>& options,
                                  std::vector<std::string>& params) {
  // Check for {options}
  if (!is_at_end() && check(TokenType::Text)) {
    // Snapshot fields before any tokens_.insert(): the insert can
    // reallocate the underlying vector and invalidate `peek()`.
    const std::size_t tok_line = peek().line;
    const std::string tok_value(peek().value);
    std::string_view sv(tok_value);
    // Strip leading whitespace
    size_t ws_end = sv.find_first_not_of(" \t\n\r");
    if (ws_end != std::string_view::npos && sv[ws_end] == '{') {
      // Parse options from this text token
      size_t close = sv.find('}', ws_end);
      if (close != std::string_view::npos) {
        std::string opt_str(sv.substr(ws_end + 1, close - ws_end - 1));
        options = split_args(opt_str);
        // Consume the rest of the text token after }
        std::string remainder(sv.substr(close + 1));
        advance();
        // If remainder has (, parse params from it
        size_t paren_pos = remainder.find('(');
        if (paren_pos != std::string::npos) {
          size_t close_paren = remainder.find(')', paren_pos);
          if (close_paren != std::string::npos) {
            std::string param_str(
                remainder.substr(paren_pos + 1, close_paren - paren_pos - 1));
            params = split_args(param_str);
            // Push back any remaining text after )
            std::string rest = remainder.substr(close_paren + 1);
            if (!rest.empty()) {
              tokens_.insert(tokens_.begin() + static_cast<ptrdiff_t>(pos_),
                             Token{TokenType::Text, rest, tok_line});
            }
          }
        } else if (!remainder.empty()) {
          // Push back remainder
          tokens_.insert(tokens_.begin() + static_cast<ptrdiff_t>(pos_),
                         Token{TokenType::Text, remainder, tok_line});
        }
      }
      return;
    }
    // Check for (params)
    if (ws_end != std::string_view::npos && sv[ws_end] == '(') {
      size_t close = sv.find(')', ws_end);
      if (close != std::string_view::npos) {
        // Simple case: ( and ) in same text token
        std::string param_str(sv.substr(ws_end + 1, close - ws_end - 1));
        params = split_args(param_str);
        std::string remainder(sv.substr(close + 1));
        advance();
        if (!remainder.empty()) {
          tokens_.insert(tokens_.begin() + static_cast<ptrdiff_t>(pos_),
                         Token{TokenType::Text, remainder, tok_line});
        }
      } else {
        // Multi-token: ( in this token, ) in a later token
        // Accumulate text across tokens until ) found
        std::string accum(sv.substr(ws_end + 1));  // text after (
        advance();                                 // consume the text token with (
        while (!is_at_end()) {
          if (check(TokenType::Text)) {
            // Snapshot ct fields too — see comment above.
            const std::size_t ct_line = peek().line;
            const std::string ct_value(peek().value);
            size_t cparen = ct_value.find(')');
            if (cparen != std::string::npos) {
              accum += ct_value.substr(0, cparen);
              std::string remainder = ct_value.substr(cparen + 1);
              advance();
              if (!remainder.empty()) {
                tokens_.insert(tokens_.begin() + static_cast<ptrdiff_t>(pos_),
                               Token{TokenType::Text, remainder, ct_line});
              }
              break;
            }
            accum += ct_value;
            advance();
          } else if (check(TokenType::VarSimple)) {
            accum += "$" + std::string(peek().value);
            advance();
          } else if (check(TokenType::VarBracket)) {
            accum += "${" + std::string(peek().value) + "}";
            advance();
          } else if (check(TokenType::VarExpr)) {
            accum += "$`" + std::string(peek().value) + "`";
            advance();
          } else {
            // Unexpected token, stop collecting
            break;
          }
        }
        params = split_args(accum);
      }
      return;
    }
  }
}

// Strip surrounding double or single quotes from a string.
static std::string strip_quotes(const std::string& s) {
  if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') ||
                        (s.front() == '\'' && s.back() == '\''))) {
    return s.substr(1, s.size() - 2);
  }
  return s;
}

// Check if a directive name is a block directive that uses { }
static bool is_block_directive(const std::string& name) {
  return name == "if" || name == "for" || name == "while" || name == "do-while" ||
         name == "f++" || name == "n++" || name == "f--" || name == "n--" ||
         name == "script" || name == "lua" || name == "exprtk" || name == "read" ||
         name == "line" || name == "getline" || name == "function" ||
         name == "console" || name == "section";
}

// Check for { at current position (possibly after whitespace text)
bool Parser::check_for_brace() {
  if (is_at_end())
    return false;
  if (check(TokenType::Text)) {
    const auto& tok = peek();
    std::string_view sv = tok.value;
    size_t ws = sv.find_first_not_of(" \t\n\r");
    if (ws != std::string_view::npos && sv[ws] == '{') {
      // Trim leading whitespace, leaving just the { (and rest)
      if (ws > 0) {
        // Replace current token with trimmed version
        const_cast<Token&>(tok).value = std::string(sv.substr(ws));
      }
      return true;
    }
  }
  return false;
}

// ─── Block body parsing ───────────────────────────────────────────

// Parse until matching } (handling nested braces)
std::vector<NodePtr> Parser::parse_block_body() {
  std::vector<NodePtr> body;

  while (!is_at_end()) {
    // Check for } (end of block) — scan anywhere in the text token
    if (check(TokenType::Text)) {
      // Snapshot fields before any allocation/insert that could
      // reallocate tokens_ and invalidate the peek() reference.
      const std::size_t tok_line = peek().line;
      const std::string tok_value(peek().value);
      std::string_view sv(tok_value);
      size_t brace_pos = sv.find('}');
      if (brace_pos != std::string_view::npos) {
        // Text before } becomes body content (if non-empty)
        std::string before(sv.substr(0, brace_pos));
        if (!before.empty()) {
          body.push_back(make_node<TextNode>(before, tok_line));
        }
        // Text after } gets pushed back for elif/else handling
        std::string after = std::string(sv.substr(brace_pos + 1));
        advance();
        if (!after.empty()) {
          tokens_.insert(tokens_.begin() + static_cast<ptrdiff_t>(pos_),
                         Token{TokenType::Text, after, tok_line});
        }
        break;
      }
    }

    // Check for @endsection (end of section block)
    if (check(TokenType::Directive)) {
      std::string_view dv = peek().value;
      if (dv == "endsection" || dv == "ends") {
        advance();
        break;
      }
    }

    // Check for elif/else (at top-level block only)
    if (check(TokenType::Text)) {
      const auto& tok = peek();
      std::string_view sv = tok.value;
      size_t ws = sv.find_first_not_of(" \t\n\r");
      if (ws != std::string_view::npos) {
        std::string_view trimmed = sv.substr(ws);
        if (trimmed.substr(0, 4) == "elif" &&
            (trimmed.size() == 4 || !Lexer::is_identifier_char(trimmed[4]))) {
          break;  // Handled by caller
        }
        if (trimmed.substr(0, 4) == "else" &&
            (trimmed.size() == 4 || !Lexer::is_identifier_char(trimmed[4]))) {
          break;  // Handled by caller
        }
      }
    }

    auto node = parse_node();
    if (node) {
      body.push_back(std::move(node));
    }
  }

  // Strip leading whitespace-only TextNodes from body
  while (!body.empty()) {
    auto* tn = std::get_if<TextNode>(body.front().get());
    if (tn && Lexer::is_whitespace_only(tn->text)) {
      body.erase(body.begin());
    } else {
      break;
    }
  }
  // Trim leading newline from first TextNode (after { delimiter)
  if (!body.empty()) {
    if (auto* tn = std::get_if<TextNode>(body.front().get())) {
      if (!tn->text.empty() && tn->text[0] == '\n') {
        tn->text.erase(0, 1);
      }
    }
  }

  return body;
}

// ─── Node parsing ─────────────────────────────────────────────────

NodePtr Parser::parse_node() {
  const auto& tok = peek();

  // Comment
  if (tok.type == TokenType::LineComment || tok.type == TokenType::BlockComment) {
    advance();
    auto comment = std::make_unique<Node>(CommentNode{tok.value, tok.line});
    return comment;
  }

  // Directive
  if (tok.type == TokenType::Directive) {
    return parse_directive();
  }

  // Variable
  if (tok.type == TokenType::VarSimple) {
    advance();
    return make_node<VarNode>(VarKind::Simple, tok.value, tok.line);
  }
  if (tok.type == TokenType::VarBracket) {
    advance();
    return make_node<VarNode>(VarKind::Bracket, tok.value, tok.line);
  }
  if (tok.type == TokenType::VarExpr) {
    advance();
    return make_node<VarNode>(VarKind::Expr, tok.value, tok.line);
  }

  // Text — might contain directive/variable references mixed in
  if (tok.type == TokenType::Text) {
    return parse_text();
  }

  // Unknown — skip
  advance();
  return nullptr;
}

NodePtr Parser::parse_text() {
  const auto& tok = advance();
  size_t line = tok.line;

  // Check if this is whitespace between } and elif/else
  if (Lexer::is_whitespace_only(tok.value)) {
    return make_node<TextNode>(tok.value, line);
  }

  // Check if this text starts with } (end of block marker)
  // Return it as text — the block body parser will handle it
  return make_node<TextNode>(tok.value, line);
}

NodePtr Parser::parse_directive() {
  const auto& tok = advance();
  std::string name = tok.value;
  size_t line = tok.line;

  std::vector<std::string> options;
  std::vector<std::string> params;
  parse_directive_args(name, options, params);

  // Template inheritance directives
  if (name == "extends" && !params.empty()) {
    return make_node<ExtendsNode>(strip_quotes(params[0]), line);
  }
  if (name == "yield" && !params.empty()) {
    return make_node<YieldNode>(strip_quotes(params[0]), line);
  }
  if (name == "parent") {
    return make_node<ParentNode>(line);
  }

  // Block directive?
  if (is_block_directive(name) && check_for_brace()) {
    // For @section, convert the BlockNode to a SectionNode
    if (name == "section") {
      auto block_node = parse_block(name, options, params, line);
      auto* block = std::get_if<BlockNode>(block_node.get());
      if (block && !block->params.empty()) {
        return make_node<SectionNode>(strip_quotes(block->params[0]),
                                      std::move(block->body), line);
      }
      return block_node;
    }
    return parse_block(name, options, params, line);
  }

  // Simple directive
  return make_node<DirectiveNode>(name, options, params, line);
}

NodePtr Parser::parse_block(const std::string& name, std::vector<std::string> options,
                            std::vector<std::string> params, size_t line) {
  // Consume the { token (inside the text)
  if (check(TokenType::Text)) {
    auto& tok = const_cast<Token&>(peek());
    std::string_view sv = tok.value;
    size_t ws = sv.find_first_not_of(" \t\n\r");
    if (ws != std::string_view::npos && sv[ws] == '{') {
      std::string remainder = std::string(sv.substr(ws + 1));
      advance();
      if (!remainder.empty()) {
        tokens_.insert(tokens_.begin() + static_cast<ptrdiff_t>(pos_),
                       Token{TokenType::Text, remainder, tok.line});
      }
    }
  }

  // Parse body until }
  auto body = parse_block_body();

  // Check for elif/else
  std::vector<ElseIfBranch> elif_branches;
  std::vector<NodePtr> else_body;
  bool has_else = false;

  while (!is_at_end() && check(TokenType::Text)) {
    // Snapshot fields we need from the current token BEFORE any
    // tokens_.insert() / parse_block_body() call. Both can reallocate
    // tokens_, which would invalidate references taken from peek().
    // ASan caught this as heap-use-after-free at branch.line = tok.line.
    const std::size_t tok_line = peek().line;
    const std::string tok_value(peek().value);
    std::string_view sv(tok_value);
    size_t ws = sv.find_first_not_of(" \t\n\r");
    if (ws == std::string_view::npos)
      break;
    std::string_view trimmed = sv.substr(ws);

    // elif(cond) { ... }
    if (trimmed.substr(0, 4) == "elif" && !Lexer::is_identifier_char(trimmed[4])) {
      // Parse elif condition
      std::string cond_text(trimmed.substr(4));
      advance();

      // Extract condition from elif(cond)
      std::vector<std::string> elif_conds;
      size_t open = cond_text.find('(');
      size_t close = cond_text.find(')', open);
      if (open != std::string::npos && close != std::string::npos) {
        std::string cond = cond_text.substr(open + 1, close - open - 1);
        // Trim
        size_t cs = cond.find_first_not_of(" \t\n\r");
        size_t ce = cond.find_last_not_of(" \t\n\r");
        if (cs != std::string::npos) {
          elif_conds.push_back(cond.substr(cs, ce - cs + 1));
        }
        // Skip to {
        std::string rest = cond_text.substr(close + 1);
        if (!rest.empty()) {
          tokens_.insert(tokens_.begin() + static_cast<ptrdiff_t>(pos_),
                         Token{TokenType::Text, rest, tok_line});
        }
      }

      if (check_for_brace()) {
        // Consume {
        if (check(TokenType::Text)) {
          // Snapshot btok fields first — same reallocation hazard.
          const std::size_t btok_line = peek().line;
          const std::string btok_value(peek().value);
          std::string_view bsv(btok_value);
          size_t bws = bsv.find_first_not_of(" \t\n\r");
          if (bws != std::string_view::npos && bsv[bws] == '{') {
            std::string brem = std::string(bsv.substr(bws + 1));
            advance();
            if (!brem.empty()) {
              tokens_.insert(tokens_.begin() + static_cast<ptrdiff_t>(pos_),
                             Token{TokenType::Text, brem, btok_line});
            }
          }
        }
        auto elif_body = parse_block_body();
        ElseIfBranch branch;
        branch.conditions = std::move(elif_conds);
        branch.body = std::move(elif_body);
        branch.line = tok_line;
        elif_branches.push_back(std::move(branch));
      }
      continue;
    }

    // else { ... }
    if (trimmed.substr(0, 4) == "else" &&
        (trimmed.size() == 4 || !Lexer::is_identifier_char(trimmed[4]))) {
      std::string rest(trimmed.substr(4));
      advance();
      if (!rest.empty()) {
        tokens_.insert(tokens_.begin() + static_cast<ptrdiff_t>(pos_),
                       Token{TokenType::Text, rest, tok_line});
      }

      if (check_for_brace()) {
        if (check(TokenType::Text)) {
          // Snapshot btok fields first — see elif branch above.
          const std::size_t btok_line = peek().line;
          const std::string btok_value(peek().value);
          std::string_view bsv(btok_value);
          size_t bws = bsv.find_first_not_of(" \t\n\r");
          if (bws != std::string_view::npos && bsv[bws] == '{') {
            std::string brem = std::string(bsv.substr(bws + 1));
            advance();
            if (!brem.empty()) {
              tokens_.insert(tokens_.begin() + static_cast<ptrdiff_t>(pos_),
                             Token{TokenType::Text, brem, btok_line});
            }
          }
        }
        else_body = parse_block_body();
        has_else = true;
      }
      continue;
    }

    break;
  }

  BlockNode block;
  block.name = std::move(name);
  block.options = std::move(options);
  block.params = std::move(params);
  block.body = std::move(body);
  block.elif_branches = std::move(elif_branches);
  block.else_body = std::move(else_body);
  block.has_else = has_else;
  block.line = line;

  return std::make_unique<Node>(std::move(block));
}

// ─── Top-level parse ──────────────────────────────────────────────

ProgramNode Parser::parse() {
  ProgramNode program;

  while (!is_at_end()) {
    auto node = parse_node();
    if (node) {
      program.children.push_back(std::move(node));
    }
  }

  return program;
}

// ─── Free functions ───────────────────────────────────────────────

ProgramNode parse_template(std::string_view source, std::string_view filename) {
  Lexer lexer(source, filename);
  auto tokens = lexer.tokenize();
  Parser parser(std::move(tokens), filename);
  return parser.parse();
}

std::string evaluate_template(
    std::string_view source,
    const std::unordered_map<std::string, std::string>& variables,
    std::string_view filename) {
  auto ast = parse_template(source, filename);

  EvalContext ctx;
  ctx.register_defaults();
  for (const auto& [k, v] : variables) {
    ctx.set_var(k, v);
  }

  Evaluator eval(ctx);
  Node root = std::move(ast);
  accept(eval, root);

  return ctx.output;
}

}  // namespace nift::parser
