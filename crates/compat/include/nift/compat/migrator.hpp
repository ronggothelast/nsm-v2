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
#include <vector>

#include "nift/core/path.hpp"
#include "nift/core/types.hpp"

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

}  // namespace nift::compat
