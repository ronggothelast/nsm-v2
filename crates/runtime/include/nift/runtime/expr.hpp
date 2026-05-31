/// @file expr.hpp
/// @brief Expression evaluator — minimal arithmetic + string ops.

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "nift/core/types.hpp"

namespace nift::runtime {

/// Variable context for expression evaluation.
using ExprContext = std::unordered_map<std::string, std::string>;

/// Evaluate an expression. Returns the result as a string.
/// Numbers serialize without trailing zeros; booleans as "true"/"false".
::nift::Expected<std::string, ::nift::Error> eval_expr(
    std::string_view expr,
    const ExprContext& ctx);

/// Evaluate as boolean. Truthy: non-empty string, non-zero number, "true".
::nift::Expected<bool, ::nift::Error> eval_bool(
    std::string_view expr,
    const ExprContext& ctx);

}  // namespace nift::runtime
