/// @file argparse.cpp

#include "nift/cli/argparse.hpp"

#include <algorithm>
#include <unordered_set>

namespace nift::cli {

std::string ParsedArgs::get(std::string_view name, std::string_view def) const {
  auto it = flags.find(std::string(name));
  if (it == flags.end())
    return std::string(def);
  return it->second;
}

bool ParsedArgs::get_bool(std::string_view name, bool def) const {
  auto it = bools.find(std::string(name));
  if (it != bools.end())
    return it->second;
  // Allow string flag like --quiet=true / --quiet=1
  auto fit = flags.find(std::string(name));
  if (fit == flags.end())
    return def;
  const auto& v = fit->second;
  return v == "true" || v == "1" || v == "yes" || v == "on";
}

bool ParsedArgs::has(std::string_view name) const {
  return flags.count(std::string(name)) > 0 || bools.count(std::string(name)) > 0;
}

ParsedArgs parse(int argc, char** argv, const std::vector<std::string>& bool_flags) {
  ParsedArgs out;
  std::unordered_set<std::string> bool_set(bool_flags.begin(), bool_flags.end());

  bool subcommand_set = false;
  bool stop_flags = false;  // After "--", everything is positional.

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (stop_flags) {
      out.positional.push_back(std::move(arg));
      continue;
    }
    if (arg == "--") {
      stop_flags = true;
      continue;
    }

    // Long flag: --name=value or --name (bool / next arg)
    if (arg.size() >= 2 && arg.substr(0, 2) == "--") {
      std::string body = arg.substr(2);
      auto eq = body.find('=');
      if (eq != std::string::npos) {
        out.flags[body.substr(0, eq)] = body.substr(eq + 1);
        continue;
      }
      // Pure --name
      if (bool_set.count(body)) {
        out.bools[body] = true;
        continue;
      }
      // Treat as string flag, consume next arg if present and not a flag.
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        out.flags[body] = argv[++i];
      } else {
        out.bools[body] = true;
      }
      continue;
    }

    // Short flag: -n value, or -abc bundled bools
    if (arg.size() >= 2 && arg[0] == '-' && arg[1] != '-') {
      std::string body = arg.substr(1);
      // Bundled bools: each char is its own flag, only valid if all are bool flags.
      if (body.size() > 1) {
        bool all_bool = std::all_of(body.begin(), body.end(), [&](char c) {
          std::string s(1, c);
          return bool_set.count(s) > 0;
        });
        if (all_bool) {
          for (char c : body)
            out.bools[std::string(1, c)] = true;
          continue;
        }
        // Otherwise treat as -n=value form: -nvalue
        if (bool_set.count(std::string(1, body[0])) == 0) {
          out.flags[std::string(1, body[0])] = body.substr(1);
          continue;
        }
      }
      // Single char
      if (bool_set.count(body)) {
        out.bools[body] = true;
        continue;
      }
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        out.flags[body] = argv[++i];
      } else {
        out.bools[body] = true;
      }
      continue;
    }

    // Positional. First is the subcommand.
    if (!subcommand_set) {
      out.subcommand = std::move(arg);
      subcommand_set = true;
    } else {
      out.positional.push_back(std::move(arg));
    }
  }
  return out;
}

}  // namespace nift::cli
