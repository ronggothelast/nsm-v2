// ─── nift/tailwind/tailwind.cpp ──────────────────────────────────────
// Tailwind CSS integration: subprocess wrapper + config generation.

#include "nift/tailwind/tailwind.hpp"

#include <fmt/format.h>

#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#endif

namespace nift::tailwind {

namespace {

// Execute a command and capture stdout.
std::string exec_cmd(const std::string& cmd) {
  std::string result;
  std::array<char, 4096> buffer;
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe)
    return result;
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
    result += buffer.data();
  }
  pclose(pipe);
  return result;
}

// Check if a file exists.
bool file_exists(const fs::path& p) {
  return fs::exists(p) && fs::is_regular_file(p);
}

// Check if a command exists in PATH.
bool command_exists(const std::string& cmd) {
#ifdef _WIN32
  auto output = exec_cmd("where " + cmd + " 2>NUL");
#else
  auto output = exec_cmd("which " + cmd + " 2>/dev/null");
#endif
  return !output.empty();
}

// Escape a string for JavaScript string literal.
std::string js_escape(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += c;
        break;
    }
  }
  return out;
}

}  // namespace

std::string find_cli() {
  // 1. Check npx tailwindcss.
  if (command_exists("npx")) {
    auto ver = exec_cmd("npx tailwindcss --help 2>/dev/null | head -1");
    if (!ver.empty() && ver.find("tailwind") != std::string::npos) {
      return "npx tailwindcss";
    }
  }

  // 2. Check node_modules/.bin/tailwindcss.
  if (file_exists("node_modules/.bin/tailwindcss")) {
    return "./node_modules/.bin/tailwindcss";
  }

  // 3. Check global tailwindcss.
  if (command_exists("tailwindcss")) {
    return "tailwindcss";
  }

  // 4. Check tailwindcss via bun.
  if (command_exists("bunx")) {
    return "bunx tailwindcss";
  }

  return "";
}

bool is_available() {
  return !find_cli().empty();
}

fs::path generate_config(const TailwindConfig& cfg) {
  // If config path is provided and exists, use it.
  if (!cfg.config_path.empty() && file_exists(cfg.config_path)) {
    return fs::path(cfg.config_path);
  }

  // Generate config file.
  fs::path config_path = cfg.working_dir / "tailwind.config.js";

  std::ofstream f(config_path);
  if (!f.is_open()) {
    throw std::runtime_error(
        fmt::format("Cannot write tailwind config: {}", config_path.string()));
  }

  f << "/** @type {import('tailwindcss').Config} */\n";
  f << "module.exports = {\n";

  // Content paths.
  f << "  content: [\n";
  for (const auto& dir : cfg.content_dirs) {
    f << "    \"" << js_escape(dir)
      << "/**/*.{html,js,ts,jsx,tsx,vue,svelte,astro,md,php}\"\n";
  }
  for (const auto& pattern : cfg.content_patterns) {
    f << "    \"" << js_escape(pattern) << "\"\n";
  }
  f << "  ],\n";

  // Theme.
  f << "  theme: {\n";
  f << "    extend: {\n";

  if (cfg.theme_overrides.is_object()) {
    for (auto& [key, val] : cfg.theme_overrides.items()) {
      f << "      " << key << ": ";
      f << val.dump(2);
      f << ",\n";
    }
  }

  f << "    },\n";
  f << "  },\n";

  // Plugins.
  if (!cfg.plugins.empty()) {
    f << "  plugins: [\n";
    for (const auto& plugin : cfg.plugins) {
      f << "    require('" << js_escape(plugin) << "'),\n";
    }
    f << "  ],\n";
  }

  f << "}\n";
  f.close();

  return config_path;
}

fs::path generate_input_css(const TailwindConfig& cfg, const std::string& custom_css) {
  fs::path input_path = cfg.working_dir / "assets" / "input.css";

  // Create directory if needed.
  fs::create_directories(input_path.parent_path());

  std::ofstream f(input_path);
  if (!f.is_open()) {
    throw std::runtime_error(
        fmt::format("Cannot write input CSS: {}", input_path.string()));
  }

  f << "@tailwind base;\n";
  f << "@tailwind components;\n";
  f << "@tailwind utilities;\n\n";

  if (!custom_css.empty()) {
    f << custom_css << "\n";
  }

  f.close();
  return input_path;
}

TailwindResult compile(const TailwindConfig& cfg) {
  auto start = std::chrono::steady_clock::now();
  TailwindResult result;

  // Find CLI.
  std::string cli = cfg.cli_path;
  if (cli.empty()) {
    cli = find_cli();
  }
  if (cli.empty()) {
    result.error_message =
        "Tailwind CLI not found. Install with: npm install -D tailwindcss";
    return result;
  }

  // Generate config if needed.
  fs::path config_path;
  try {
    config_path = generate_config(cfg);
  } catch (const std::exception& e) {
    result.error_message = fmt::format("Config generation failed: {}", e.what());
    return result;
  }

  // Determine input CSS.
  std::string input = cfg.input_css;
  if (input.empty()) {
    auto generated = generate_input_css(cfg);
    input = generated.string();
  }

  // Determine output CSS.
  std::string output = cfg.output_css;
  if (output.empty()) {
    output = (cfg.working_dir / "assets" / "style.css").string();
  }

  // Create output directory.
  fs::create_directories(fs::path(output).parent_path());

  // Build command.
  std::string cmd = cli;
  cmd += " --input " + input;
  cmd += " --output " + output;
  cmd += " --config " + config_path.string();

  if (cfg.minify) {
    cmd += " --minify";
  }
  if (cfg.sourcemaps) {
    cmd += " --source-map";
  }

  // Execute.
  auto stdout_output = exec_cmd(cmd + " 2>&1");
  auto end = std::chrono::steady_clock::now();
  result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

  // Check if output was created.
  if (file_exists(output)) {
    result.success = true;
    result.output_path = output;
    result.output_size = fs::file_size(output);
  } else {
    result.error_message = "Tailwind compilation failed. Output: " + stdout_output;
  }

  return result;
}

TailwindResult watch(const TailwindConfig& cfg) {
  TailwindResult result;

  std::string cli = cfg.cli_path;
  if (cli.empty())
    cli = find_cli();
  if (cli.empty()) {
    result.error_message = "Tailwind CLI not found";
    return result;
  }

  fs::path config_path;
  try {
    config_path = generate_config(cfg);
  } catch (const std::exception& e) {
    result.error_message = fmt::format("Config generation failed: {}", e.what());
    return result;
  }

  std::string input = cfg.input_css;
  if (input.empty()) {
    auto generated = generate_input_css(cfg);
    input = generated.string();
  }

  std::string output = cfg.output_css;
  if (output.empty()) {
    output = (cfg.working_dir / "assets" / "style.css").string();
  }

  fs::create_directories(fs::path(output).parent_path());

  std::string cmd = cli;
  cmd += " --input " + input;
  cmd += " --output " + output;
  cmd += " --config " + config_path.string();
  cmd += " --watch";

  if (cfg.minify)
    cmd += " --minify";
  if (cfg.sourcemaps)
    cmd += " --source-map";

  // This blocks until interrupted.
  int ret = system(cmd.c_str());
  result.success = (ret == 0);
  if (!result.success) {
    result.error_message = fmt::format("Tailwind watch exited with code {}", ret);
  }

  return result;
}

std::vector<std::string> scan_classes(const fs::path& content_dir) {
  std::vector<std::string> classes;
  std::regex class_regex(R"((?:class(?:Name)?|tw)\s*[=:]\s*["']([^"']+)["'])");

  for (auto& entry : fs::recursive_directory_iterator(content_dir)) {
    if (!entry.is_regular_file())
      continue;

    auto ext = entry.path().extension().string();
    if (ext != ".html" && ext != ".htm" && ext != ".js" && ext != ".ts" &&
        ext != ".jsx" && ext != ".tsx" && ext != ".vue" && ext != ".svelte" &&
        ext != ".astro" && ext != ".md" && ext != ".php" && ext != ".nift") {
      continue;
    }

    std::ifstream f(entry.path());
    if (!f.is_open())
      continue;

    std::string content((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());

    std::sregex_iterator it(content.begin(), content.end(), class_regex);
    std::sregex_iterator end;

    for (; it != end; ++it) {
      std::string class_str = (*it)[1].str();
      // Split by whitespace.
      std::istringstream ss(class_str);
      std::string cls;
      while (ss >> cls) {
        // Remove leading/trailing quotes.
        if (!cls.empty()) {
          classes.push_back(cls);
        }
      }
    }
  }

  // Deduplicate.
  std::sort(classes.begin(), classes.end());
  classes.erase(std::unique(classes.begin(), classes.end()), classes.end());

  return classes;
}

TailwindResult purge(const TailwindConfig& cfg, const fs::path& css_input,
                     const std::vector<fs::path>& content_files) {
  TailwindResult result;

  // For purge, we run Tailwind with the content list.
  TailwindConfig purge_cfg = cfg;
  purge_cfg.input_css = css_input.string();
  purge_cfg.minify = true;

  // Add content files as patterns.
  for (const auto& f : content_files) {
    purge_cfg.content_patterns.push_back(f.string());
  }

  return compile(purge_cfg);
}

}  // namespace nift::tailwind

// ─── TailwindWatcher implementation ──────────────────────────────────

#ifndef _WIN32
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace nift::tailwind {

struct TailwindWatcher::Impl {
  std::atomic<bool> running{false};
  std::thread thread;
  std::string cmd;  // Full command to run.

#ifndef _WIN32
  pid_t child_pid = -1;
#endif
};

TailwindWatcher::TailwindWatcher() : impl_(std::make_unique<Impl>()) {}

TailwindWatcher::~TailwindWatcher() { stop(); }

bool TailwindWatcher::start(const TailwindConfig& cfg) {
  if (impl_->running)
    return true;

  std::string cli = cfg.cli_path;
  if (cli.empty())
    cli = find_cli();
  if (cli.empty())
    return false;

  fs::path config_path;
  try {
    config_path = generate_config(cfg);
  } catch (...) {
    return false;
  }

  std::string input = cfg.input_css;
  if (input.empty()) {
    auto generated = generate_input_css(cfg);
    input = generated.string();
  }

  std::string output = cfg.output_css;
  if (output.empty())
    output = (cfg.working_dir / "assets" / "style.css").string();

  fs::create_directories(fs::path(output).parent_path());

  std::string cmd = cli;
  cmd += " --input " + input;
  cmd += " --output " + output;
  cmd += " --config " + config_path.string();
  cmd += " --watch";
  if (cfg.minify)
    cmd += " --minify";
  if (cfg.sourcemaps)
    cmd += " --source-map";

  impl_->cmd = cmd;
  impl_->running = true;

#ifndef _WIN32
  impl_->thread = std::thread([this]() {
    pid_t pid = fork();
    if (pid == 0) {
      // Child process.
      execlp("sh", "sh", "-c", impl_->cmd.c_str(), nullptr);
      _exit(127);
    } else if (pid > 0) {
      impl_->child_pid = pid;
      int status = 0;
      waitpid(pid, &status, 0);
      impl_->child_pid = -1;
    }
    impl_->running = false;
  });
#else
  impl_->thread = std::thread([this]() {
    int ret = system(impl_->cmd.c_str());
    (void)ret;
    impl_->running = false;
  });
#endif

  return true;
}

void TailwindWatcher::stop() {
  if (!impl_->running)
    return;
  impl_->running = false;

#ifndef _WIN32
  if (impl_->child_pid > 0) {
    kill(impl_->child_pid, SIGTERM);
    // Give it 2s to exit, then SIGKILL.
    for (int i = 0; i < 20; ++i) {
      if (kill(impl_->child_pid, 0) != 0)
        break;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (kill(impl_->child_pid, 0) == 0)
      kill(impl_->child_pid, SIGKILL);
    impl_->child_pid = -1;
  }
#endif

  if (impl_->thread.joinable())
    impl_->thread.join();
}

bool TailwindWatcher::is_running() const noexcept {
  return impl_->running;
}

}  // namespace nift::tailwind
