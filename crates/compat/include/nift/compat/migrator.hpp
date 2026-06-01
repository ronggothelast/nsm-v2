/// @file migrator.hpp
/// @brief Migrate a Nift v1 (`nsm`) project layout to Nift v2.
///
/// v1 → v2 mapping:
///   - `siteInfo/.../nsm.config` → `nift.json` (translated keys)
///   - `siteInfo/tracked.json`   → `.nift/tracked.json`
///   - source `content/` and `template/` directories are kept in place
///     (v2 reads them by default).
///
/// The migrator is non-destructive: it never deletes v1 files. It writes
/// the v2 config alongside, and emits a report listing what it did and
/// any potential issues (unresolved keys, ambiguous paths).

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "nift/core/path.hpp"
#include "nift/core/types.hpp"
#include "nift/project/project.hpp"

namespace nift::compat {

struct MigrationReport {
  std::size_t files_examined = 0;
  bool config_written = false;
  std::string source_config_path;
  std::vector<std::string> notes;
};

/// @brief Run the migrator against a project root.
::nift::Expected<MigrationReport, ::nift::Error> migrate_project(
    const ::nift::core::Path& root);

/// @brief Translate a single v1 config key=value pair into a v2 key.
/// Returns the v2 key, or empty string if the key is unknown.
std::string translate_config_key(std::string_view v1_key);

/// @brief Parse a v1-style INI/key=value config file into ordered pairs.
/// Accepts `key value`, `key = value`, `key: value` and quoted values.
/// Comments start with `#`. Used for both migration and legacy-fallback
/// loading.
std::vector<std::pair<std::string, std::string>> parse_v1_config_text(
    std::string_view content);

/// @brief Build a ProjectConfig from a v1-format config file. Unknown
/// keys are recorded in `notes` (out param) but do not fail the load.
::nift::Expected<::nift::project::ProjectConfig, ::nift::Error> load_v1_config_file(
    const ::nift::core::Path& path, std::vector<std::string>* notes = nullptr);

}  // namespace nift::compat
