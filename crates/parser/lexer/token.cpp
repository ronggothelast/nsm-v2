/// @file token.cpp
/// @brief Token utilities.

#include "nift/parser/token.hpp"

namespace nift::parser {

const char* token_type_name(TokenType t) noexcept {
  switch (t) {
    case TokenType::Text:         return "Text";
    case TokenType::Directive:    return "Directive";
    case TokenType::VarSimple:    return "VarSimple";
    case TokenType::VarBracket:   return "VarBracket";
    case TokenType::VarExpr:      return "VarExpr";
    case TokenType::LParen:       return "LParen";
    case TokenType::RParen:       return "RParen";
    case TokenType::LBrace:       return "LBrace";
    case TokenType::RBrace:       return "RBrace";
    case TokenType::Comma:        return "Comma";
    case TokenType::LineComment:  return "LineComment";
    case TokenType::BlockComment: return "BlockComment";
    case TokenType::Eof:          return "Eof";
  }
  return "Unknown";
}

} // namespace nift::parser
