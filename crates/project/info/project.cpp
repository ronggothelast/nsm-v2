/// @file project.cpp
/// @brief ProjectConfig load/save via JSON.

#include "nift/project/project.hpp"

#include <nlohmann/json.hpp>

#include "nift/core/filesystem.hpp"

namespace nift::project {

using json = nlohmann::json;

namespace {

void to_json(json& j, const ProjectConfig& c) {
  j = json{
      {"name", c.name},
      {"version", c.version},
      {"content_dir", c.content_dir.str()},
      {"output_dir", c.output_dir.str()},
      {"template_dir", c.template_dir.str()},
      {"cache_dir", c.cache_dir.str()},
      {"tracked_path", c.tracked_path.str()},
      {"default_template", c.default_template},
      {"ignored_paths", c.ignored_paths},
  };
}

void from_json(const json& j, ProjectConfig& c) {
  if (j.contains("name"))
    c.name = j.at("name").get<std::string>();
  if (j.contains("version"))
    c.version = j.at("version").get<std::string>();
  if (j.contains("content_dir"))
    c.content_dir = ::nift::core::Path(j.at("content_dir").get<std::string>());
  if (j.contains("output_dir"))
    c.output_dir = ::nift::core::Path(j.at("output_dir").get<std::string>());
  if (j.contains("template_dir"))
    c.template_dir = ::nift::core::Path(j.at("template_dir").get<std::string>());
  if (j.contains("cache_dir"))
    c.cache_dir = ::nift::core::Path(j.at("cache_dir").get<std::string>());
  if (j.contains("tracked_path"))
    c.tracked_path = ::nift::core::Path(j.at("tracked_path").get<std::string>());
  if (j.contains("default_template"))
    c.default_template = j.at("default_template").get<std::string>();
  if (j.contains("ignored_paths"))
    c.ignored_paths = j.at("ignored_paths").get<std::vector<std::string>>();
}

}  // namespace

::nift::Expected<ProjectConfig, ::nift::Error> ProjectConfig::load(
    const ::nift::core::Path& config_path) {
  if (!::nift::core::file_exists(config_path)) {
    // No config file → return defaults.
    return ProjectConfig{};
  }
  auto content = ::nift::core::read_file(config_path);
  if (!content) {
    return ::nift::unexpected<::nift::Error>(content.error());
  }
  return from_json(*content);
}

::nift::Expected<std::string, ::nift::Error> ProjectConfig::to_json() const {
  json j;
  ::nift::project::to_json(j, *this);
  return j.dump(2);
}

::nift::Expected<ProjectConfig, ::nift::Error> ProjectConfig::from_json(
    std::string_view jstr) {
  try {
    auto parsed = json::parse(jstr);
    ProjectConfig c;
    ::nift::project::from_json(parsed, c);
    return c;
  } catch (const std::exception&) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::parse_error);
  }
}

}  // namespace nift::project
