/// @file argparse.hpp
/// @brief Minimal argument parser for the nift CLI.
///
/// Supports:
///   - Positional arguments (subcommand + extras)
///   - Long flags: --name=value, --name value, --bool
///   - Short flags: -n value, -b
///   - Bundled short flags: -abc → -a -b -c (booleans only)
///
/// We roll our own to avoid a CLI11 / argparse dep — the surface is
/// small and the dep tree is already large enough.

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nift::cli {

struct ParsedArgs {
  std::string subcommand;
  std::vector<std::string> positional;
  std::unordered_map<std::string, std::string> flags;  // string flags
  std::unordered_map<std::string, bool> bools;         // bool flags

  /// Returns the string value of `name` if present, otherwise `def`.
  std::string get(std::string_view name, std::string_view def = "") const;

  /// Returns the bool value of `name`, defaulting to `def`.
  bool get_bool(std::string_view name, bool def = false) const;

  /// True if a flag (string or bool) was provided.
  bool has(std::string_view name) const;
};

/// @brief Parse argv into a ParsedArgs. argv[0] is the program name (skipped).
///        argv[1] is treated as the subcommand if it doesn't start with `-`.
///
/// `bool_flags` is a list of flag names that should be treated as booleans
/// (so `--foo` without a value sets bools[foo]=true rather than consuming
/// the next argument). All other `--name value` are treated as string flags.
ParsedArgs parse(int argc, char** argv,
                 const std::vector<std::string>& bool_flags = {});

}  // namespace nift::cli
