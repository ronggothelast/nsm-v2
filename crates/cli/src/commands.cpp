/// @file commands.cpp

#include "nift/cli/commands.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

#include "nift/build/pipeline.hpp"
#include "nift/compat/migrator.hpp"
#include "nift/core/filesystem.hpp"
#include "nift/project/build_cache.hpp"
#include "nift/project/project.hpp"
#include "nift/server/file_watcher.hpp"
#include "nift/server/http_server.hpp"
#include "nift/tailwind/tailwind.hpp"

namespace nift::cli {

namespace fs = std::filesystem;

std::string version_string() {
  return "nift 2.0.0-dev";
}

std::string help_text() {
  return R"(nift — modern C++ static site generator

USAGE:
  nift <command> [options]

COMMANDS:
  init [path]        Scaffold a new nift project at `path` (default: .)
  build [path]       Build the site at `path` (default: .)
  serve [path]       Build + serve with hot reload (default: .)
  migrate [path]     Migrate a v1 nsm project to v2 layout
  clean [path]       Remove build outputs and cache
  version            Print version
  help               Show this help

GLOBAL OPTIONS:
  --quiet            Suppress non-error output
  --no-cache         Disable incremental build cache
  --threads <N>      Worker thread count (default: hardware_concurrency)

SERVE OPTIONS:
  --host <addr>      Bind address (default: 127.0.0.1)
  --port <n>         Port (default: 8080, 0=ephemeral)
  --no-watch         Don't watch for file changes

EXAMPLES:
  nift init my-blog          Create a new project
  nift build                 Build the project in current dir
  nift serve --port 3000     Dev server on port 3000

See https://github.com/ronggothelast/nsm-v2 for full documentation.
)";
}

int cmd_help(const ParsedArgs&) {
  std::cout << help_text();
  return 0;
}

int cmd_version(const ParsedArgs&) {
  std::cout << version_string() << "\n";
  return 0;
}

namespace {

::nift::core::Path target_dir(const ParsedArgs& args) {
  return ::nift::core::Path(args.positional.empty() ? "." : args.positional[0]);
}

bool quiet(const ParsedArgs& args) {
  return args.get_bool("quiet");
}

::nift::project::ProjectConfig load_or_default(const ::nift::core::Path& root) {
  // Prefer canonical v2 nift.json. Fall back to legacy v1 INI/key=value
  // files (nift.config, nsm.config, siteInfo/nsm.config) so existing
  // projects keep building without an explicit `nift migrate` step.
  auto cfg_path = ::nift::core::Path(root.str() + "/nift.json");
  ::nift::project::ProjectConfig out;

  bool loaded = false;
  if (::nift::core::file_exists(cfg_path)) {
    auto cfg = ::nift::project::ProjectConfig::load(cfg_path);
    if (cfg) {
      out = *cfg;
      loaded = true;
    }
  }
  if (!loaded) {
    static const std::vector<std::string> kLegacyCandidates = {
        "/nift.config",
        "/nsm.config",
        "/siteInfo/nsm.config",
        "/.nsm/nsm.config",
    };
    for (const auto& rel : kLegacyCandidates) {
      ::nift::core::Path p(root.str() + rel);
      if (!::nift::core::file_exists(p))
        continue;
      auto cfg = ::nift::compat::load_v1_config_file(p, nullptr);
      if (cfg) {
        out = *cfg;
        loaded = true;
        break;
      }
    }
  }
  // If nothing was found, `out` stays at struct defaults.

  // Re-root any relative paths against project root.
  auto rebase = [&](::nift::core::Path& p) {
    if (!p.str().empty() && p.str()[0] != '/') {
      p = ::nift::core::Path(root.str() + "/" + p.str());
    }
  };
  rebase(out.content_dir);
  rebase(out.output_dir);
  rebase(out.template_dir);
  rebase(out.cache_dir);
  rebase(out.tracked_path);
  return out;
}

std::vector<::nift::core::Path> discover_sources(const ::nift::core::Path& dir) {
  std::vector<::nift::core::Path> out;
  std::error_code ec;
  if (!fs::exists(dir.str(), ec))
    return out;
  fs::recursive_directory_iterator it(dir.str(), ec);
  if (ec)
    return out;
  fs::recursive_directory_iterator end;
  for (; it != end; it.increment(ec)) {
    if (ec) {
      ec.clear();
      continue;
    }
    if (it->is_regular_file(ec)) {
      out.emplace_back(it->path().string());
    }
  }
  return out;
}

}  // namespace

int cmd_init(const ParsedArgs& args) {
  auto root = target_dir(args);
  std::error_code ec;
  fs::create_directories(root.str(), ec);
  fs::create_directories(root.str() + "/content", ec);
  fs::create_directories(root.str() + "/templates", ec);
  fs::create_directories(root.str() + "/public", ec);
  fs::create_directories(root.str() + "/.nift/cache", ec);

  ::nift::project::ProjectConfig cfg;
  cfg.content_dir = ::nift::core::Path("content");
  cfg.output_dir = ::nift::core::Path("public");
  cfg.template_dir = ::nift::core::Path("templates");
  cfg.cache_dir = ::nift::core::Path(".nift/cache");
  cfg.tracked_path = ::nift::core::Path(".nift/tracked.json");
  auto json = cfg.to_json();
  if (!json) {
    std::cerr << "init: failed to serialize config\n";
    return 2;
  }
  auto cfg_path = ::nift::core::Path(root.str() + "/nift.json");
  if (!::nift::core::write_file(cfg_path, *json)) {
    std::cerr << "init: failed to write nift.json\n";
    return 2;
  }

  // Sample content + template.
  ::nift::core::Path index_md(root.str() + "/content/index.html");
  if (!::nift::core::file_exists(index_md)) {
    (void)::nift::core::write_file(
        index_md,
        "<!doctype html><html><head><title>Nift Site</title></head>"
        "<body><h1>Welcome to Nift v2</h1>"
        "<p>Edit <code>content/index.html</code> to get started.</p>"
        "</body></html>\n");
  }

  if (!quiet(args)) {
    std::cout << "Initialized nift project at " << root.str() << "\n";
  }
  return 0;
}

int cmd_build(const ParsedArgs& args) {
  auto root = target_dir(args);
  auto cfg = load_or_default(root);

  // Optional --threads override.
  std::size_t pool_size = 0;
  if (args.has("threads")) {
    try {
      pool_size = std::stoul(args.get("threads", "0"));
    } catch (...) {}
  }

  ::nift::project::BuildCache cache(cfg.cache_dir);
  bool use_cache = !args.get_bool("no-cache");
  if (use_cache)
    (void)cache.load();

  auto sources = discover_sources(cfg.content_dir);
  if (sources.empty()) {
    if (!quiet(args)) {
      std::cout << "No content found in " << cfg.content_dir.str() << "\n";
    }
    return 0;
  }

  ::nift::build::Pipeline pipe(cfg, use_cache ? &cache : nullptr, pool_size);
  auto report = pipe.build(sources);

  if (use_cache)
    (void)cache.save();

  if (!quiet(args)) {
    std::cout << "Built " << report.built << " · cached " << report.cached
              << " · failed " << report.failed << " in " << report.elapsed.count()
              << " ms\n";
  }
  return report.failed == 0 ? 0 : 1;
}

int cmd_serve(const ParsedArgs& args) {
  // Initial build.
  int rc = cmd_build(args);
  if (rc != 0)
    return rc;

  auto root = target_dir(args);
  auto cfg = load_or_default(root);

  ::nift::server::ServerConfig sc;
  sc.host = args.get("host", "127.0.0.1");
  try {
    sc.port = static_cast<std::uint16_t>(std::stoi(args.get("port", "8080")));
  } catch (...) {
    sc.port = 8080;
  }
  sc.root = cfg.output_dir;

  ::nift::server::HttpServer server(sc);
  if (!server.start()) {
    std::cerr << "serve: failed to start HTTP server\n";
    return 2;
  }
  std::cout << "Serving " << cfg.output_dir.str() << " at http://" << sc.host << ":"
            << server.bound_port() << "\n";

  std::unique_ptr<::nift::server::FileWatcher> watcher;
  if (!args.get_bool("no-watch")) {
    ::nift::server::WatcherConfig wc;
    wc.root = cfg.content_dir;
    watcher = std::make_unique<::nift::server::FileWatcher>(wc);
    auto status = watcher->start([&](const std::vector<::nift::server::ChangeEvent>&) {
      // Re-run build on change.
      ParsedArgs rebuild_args = args;
      rebuild_args.bools["quiet"] = true;
      (void)cmd_build(rebuild_args);
      server.notify_rebuild();
      std::cout << "Rebuilt — generation " << server.generation() << "\n";
    });
    if (!status) {
      std::cerr << "serve: file watcher failed to start (continuing without)\n";
      watcher.reset();
    } else {
      std::cout << "Watching " << cfg.content_dir.str() << " for changes\n";
    }
  }

  // Start Tailwind watch mode if available and configured.
  std::unique_ptr<::nift::tailwind::TailwindWatcher> tw_watcher;
  if (!args.get_bool("no-watch") && ::nift::tailwind::is_available()) {
    ::nift::tailwind::TailwindConfig tw_cfg;
    tw_cfg.working_dir = root.native();
    tw_cfg.content_dirs.push_back(cfg.content_dir.str());
    if (!cfg.output_dir.str().empty())
      tw_cfg.output_css = cfg.output_dir.join("style.css").str();

    tw_watcher = std::make_unique<::nift::tailwind::TailwindWatcher>();
    if (tw_watcher->start(tw_cfg)) {
      std::cout << "Tailwind watch mode active\n";
    } else {
      std::cerr << "serve: Tailwind watch failed to start (continuing without)\n";
      tw_watcher.reset();
    }
  }

  std::cout << "Press Ctrl-C to stop\n";
  while (server.is_running()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // Stop Tailwind watcher before exit.
  if (tw_watcher) {
    tw_watcher->stop();
  }

  return 0;
}

int cmd_migrate(const ParsedArgs& args) {
  auto root = target_dir(args);
  auto report = ::nift::compat::migrate_project(root);
  if (!report) {
    std::cerr << "migrate: failed\n";
    return 1;
  }
  if (!quiet(args)) {
    std::cout << "Migrated " << report->files_examined << " files\n";
    if (report->config_written) {
      std::cout << "Wrote nift.json (was: " << report->source_config_path << ")\n";
    }
    for (const auto& note : report->notes) {
      std::cout << "  - " << note << "\n";
    }
  }
  return 0;
}

int cmd_clean(const ParsedArgs& args) {
  auto root = target_dir(args);
  auto cfg = load_or_default(root);
  std::error_code ec;
  fs::remove_all(cfg.output_dir.str(), ec);
  fs::remove_all(cfg.cache_dir.str(), ec);
  if (!quiet(args)) {
    std::cout << "Cleaned " << cfg.output_dir.str() << " and " << cfg.cache_dir.str()
              << "\n";
  }
  return 0;
}

int dispatch(const ParsedArgs& args) {
  if (args.subcommand.empty() || args.subcommand == "help" || args.get_bool("help") ||
      args.get_bool("h")) {
    return cmd_help(args);
  }
  if (args.subcommand == "version" || args.get_bool("version") || args.get_bool("V")) {
    return cmd_version(args);
  }
  if (args.subcommand == "init")
    return cmd_init(args);
  if (args.subcommand == "build")
    return cmd_build(args);
  if (args.subcommand == "serve")
    return cmd_serve(args);
  if (args.subcommand == "migrate")
    return cmd_migrate(args);
  if (args.subcommand == "clean")
    return cmd_clean(args);

  std::cerr << "Unknown command: " << args.subcommand << "\n";
  std::cerr << "Run `nift help` for usage.\n";
  return 2;
}

}  // namespace nift::cli
