/// @file migrator.cpp

#include "nift/compat/migrator.hpp"

#include <filesystem>
#include <sstream>
#include <unordered_map>

#include "nift/core/filesystem.hpp"
#include "nift/project/project.hpp"

namespace nift::compat {

namespace fs = std::filesystem;

std::string translate_config_key(std::string_view v1_key) {
  static const std::unordered_map<std::string, std::string> kMap = {
      // v1 nsm.config keys → v2 nift.json keys (camelCase variants).
      {"contentDir", "content_dir"},
      {"contentDirectory", "content_dir"},
      {"siteDir", "output_dir"},
      {"outputDir", "output_dir"},
      {"templateDir", "template_dir"},
      {"templateDirectory", "template_dir"},
      {"defaultTemplate", "default_template"},
      {"siteName", "name"},
      {"name", "name"},
      {"version", "version"},
      {"buildDir", "cache_dir"},
      // v1 alternative snake_case + lowercase variants found in user
      // projects and the chaos test corpus. Map to the v2 canonical keys.
      {"content_dir", "content_dir"},
      {"output_dir", "output_dir"},
      {"template_dir", "template_dir"},
      {"templates_dir", "template_dir"},
      // Legacy term "layouts" is treated as templates in v2.
      {"layouts_dir", "template_dir"},
      {"layoutDir", "template_dir"},
      {"layoutsDir", "template_dir"},
      {"default_template", "default_template"},
      {"cache_dir", "cache_dir"},
  };
  auto it = kMap.find(std::string(v1_key));
  if (it == kMap.end())
    return "";
  return it->second;
}

namespace {

::nift::core::Path find_v1_config(const ::nift::core::Path& root) {
  // Common v1 locations: siteInfo/nsm.config, .nsm/nsm.config, nsm.config.
  std::vector<std::string> candidates = {
      root.str() + "/siteInfo/nsm.config",
      root.str() + "/.nsm/nsm.config",
      root.str() + "/nsm.config",
      root.str() + "/.nsm/site.config",
  };
  for (const auto& c : candidates) {
    ::nift::core::Path p(c);
    if (::nift::core::file_exists(p))
      return p;
  }
  return ::nift::core::Path("");
}

void apply_v1_pairs_to_config(
    const std::vector<std::pair<std::string, std::string>>& pairs,
    ::nift::project::ProjectConfig& cfg, std::vector<std::string>* notes) {
  for (const auto& [k, v] : pairs) {
    auto v2 = translate_config_key(k);
    if (v2.empty()) {
      if (notes)
        notes->push_back("Unknown v1 key: " + k + " (skipped)");
      continue;
    }
    if (v2 == "name")
      cfg.name = v;
    else if (v2 == "version")
      cfg.version = v;
    else if (v2 == "content_dir")
      cfg.content_dir = ::nift::core::Path(v);
    else if (v2 == "output_dir")
      cfg.output_dir = ::nift::core::Path(v);
    else if (v2 == "template_dir")
      cfg.template_dir = ::nift::core::Path(v);
    else if (v2 == "default_template")
      cfg.default_template = v;
    else if (v2 == "cache_dir")
      cfg.cache_dir = ::nift::core::Path(v);
  }
}

}  // namespace

std::vector<std::pair<std::string, std::string>> parse_v1_config_text(
    std::string_view content) {
  std::vector<std::pair<std::string, std::string>> out;
  std::istringstream iss((std::string(content)));
  std::string line;
  while (std::getline(iss, line)) {
    // Strip comments + leading/trailing whitespace.
    auto h = line.find('#');
    if (h != std::string::npos)
      line.resize(h);
    auto first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
      continue;
    auto last = line.find_last_not_of(" \t\r\n");
    line = line.substr(first, last - first + 1);
    if (line.empty())
      continue;

    // v1 syntax variants:
    //   key value
    //   key = value
    //   key: value
    auto sep = line.find_first_of("=:\t ");
    if (sep == std::string::npos)
      continue;
    std::string key = line.substr(0, sep);
    std::string val = line.substr(sep + 1);
    auto vstart = val.find_first_not_of(" \t=:");
    if (vstart == std::string::npos)
      continue;
    val = val.substr(vstart);
    if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
      val = val.substr(1, val.size() - 2);
    }
    out.emplace_back(std::move(key), std::move(val));
  }
  return out;
}

::nift::Expected<::nift::project::ProjectConfig, ::nift::Error> load_v1_config_file(
    const ::nift::core::Path& path, std::vector<std::string>* notes) {
  if (!::nift::core::file_exists(path)) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::not_found);
  }
  auto src = ::nift::core::read_file(path);
  if (!src) {
    return ::nift::unexpected<::nift::Error>(src.error());
  }
  ::nift::project::ProjectConfig cfg;
  apply_v1_pairs_to_config(parse_v1_config_text(*src), cfg, notes);
  return cfg;
}

::nift::Expected<MigrationReport, ::nift::Error> migrate_project(
    const ::nift::core::Path& root) {
  MigrationReport report;

  // Count files for the report (best-effort).
  std::error_code ec;
  if (fs::exists(root.str(), ec)) {
    fs::recursive_directory_iterator it(root.str(), ec);
    fs::recursive_directory_iterator end;
    for (; it != end; it.increment(ec)) {
      if (ec) {
        ec.clear();
        continue;
      }
      if (it->is_regular_file(ec))
        ++report.files_examined;
    }
  }

  auto v1_cfg = find_v1_config(root);
  if (v1_cfg.str().empty()) {
    report.notes.push_back("No v1 nsm.config found. Wrote default v2 nift.json.");
    ::nift::project::ProjectConfig defaults;
    auto json = defaults.to_json();
    if (!json)
      return ::nift::unexpected<::nift::Error>(json.error());
    auto out_path = ::nift::core::Path(root.str() + "/nift.json");
    auto status = ::nift::core::write_file(out_path, *json);
    if (!status)
      return ::nift::unexpected<::nift::Error>(status.error());
    report.config_written = true;
    return report;
  }

  report.source_config_path = v1_cfg.str();
  auto src = ::nift::core::read_file(v1_cfg);
  if (!src) {
    return ::nift::unexpected<::nift::Error>(src.error());
  }

  ::nift::project::ProjectConfig cfg;
  apply_v1_pairs_to_config(parse_v1_config_text(*src), cfg, &report.notes);

  auto json = cfg.to_json();
  if (!json)
    return ::nift::unexpected<::nift::Error>(json.error());
  auto out_path = ::nift::core::Path(root.str() + "/nift.json");
  auto status = ::nift::core::write_file(out_path, *json);
  if (!status)
    return ::nift::unexpected<::nift::Error>(status.error());
  report.config_written = true;

  // Migrate tracked.json if present.
  auto v1_tracked = ::nift::core::Path(root.str() + "/siteInfo/tracked.json");
  if (::nift::core::file_exists(v1_tracked)) {
    fs::create_directories(root.str() + "/.nift", ec);
    auto out_tracked = ::nift::core::Path(root.str() + "/.nift/tracked.json");
    auto content = ::nift::core::read_file(v1_tracked);
    if (content) {
      (void)::nift::core::write_file(out_tracked, *content);
      report.notes.push_back("Migrated tracked.json → .nift/tracked.json");
    }
  }

  return report;
}

}  // namespace nift::compat
