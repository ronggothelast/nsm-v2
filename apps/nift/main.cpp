/// @file main.cpp
/// @brief nift CLI entry point.

#include "nift/cli/argparse.hpp"
#include "nift/cli/commands.hpp"

int main(int argc, char** argv) {
  std::vector<std::string> bool_flags = {
      "quiet", "no-cache", "no-watch", "help", "h", "version", "V",
  };
  auto args = nift::cli::parse(argc, argv, bool_flags);
  return nift::cli::dispatch(args);
}
