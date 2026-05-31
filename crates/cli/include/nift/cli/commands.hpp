/// @file commands.hpp
/// @brief Subcommand entry points for the nift CLI.

#pragma once

#include <string>

#include "nift/cli/argparse.hpp"

namespace nift::cli {

/// Each command returns an exit code (0 = success, non-zero = failure).

int cmd_help(const ParsedArgs& args);
int cmd_version(const ParsedArgs& args);
int cmd_init(const ParsedArgs& args);
int cmd_build(const ParsedArgs& args);
int cmd_serve(const ParsedArgs& args);
int cmd_migrate(const ParsedArgs& args);
int cmd_clean(const ParsedArgs& args);

/// Top-level dispatch — picks the right handler based on subcommand.
int dispatch(const ParsedArgs& args);

/// CLI version string (e.g. "2.0.0-dev").
std::string version_string();

/// Help text for `nift help` and `nift --help`.
std::string help_text();

}  // namespace nift::cli
