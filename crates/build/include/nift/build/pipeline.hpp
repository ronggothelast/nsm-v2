/// @file pipeline.hpp
/// @brief High-level build pipeline orchestrator.
///
/// Wires together: ProjectConfig → file discovery → cache check → parser →
/// evaluator → output write, all driven by the work-stealing pool.
///
/// Phase 4 stub: the orchestration layer exists with a clean interface
/// but the parse+evaluate steps are intentionally narrow (single-template
/// pass-through). Phase 6 (`nift build` CLI) will wire it to real templates.

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "nift/build/executor.hpp"
#include "nift/core/path.hpp"
#include "nift/core/types.hpp"
#include "nift/project/build_cache.hpp"
#include "nift/project/project.hpp"

namespace nift::build {

/// @brief Per-file build result.
enum class FileStatus : std::uint8_t {
  Built,   ///< Successfully rendered.
  Cached,  ///< Skipped — cache hit.
  Failed,  ///< Build error.
};

struct FileResult {
  ::nift::core::Path source;
  FileStatus status = FileStatus::Failed;
  std::string error_message;
};

/// @brief Aggregate build report.
struct BuildReport {
  std::size_t built = 0;
  std::size_t cached = 0;
  std::size_t failed = 0;
  std::chrono::milliseconds elapsed{0};
  std::vector<FileResult> file_results;
};

/// @brief High-level build pipeline.
class Pipeline {
 public:
  /// Construct with project config and optional cache.
  /// `pool_size = 0` means hardware_concurrency().
  Pipeline(::nift::project::ProjectConfig config, ::nift::project::BuildCache* cache,
           std::size_t pool_size = 0);

  /// Build all source files in the project.
  /// `inputs` is a list of paths relative to `config.content_dir` or absolute.
  BuildReport build(const std::vector<::nift::core::Path>& inputs);

  /// Number of worker threads in the pool.
  std::size_t pool_size() const noexcept { return pool_.size(); }

 private:
  ::nift::project::ProjectConfig config_;
  ::nift::project::BuildCache* cache_;
  WorkStealingPool pool_;

  FileResult build_one(const ::nift::core::Path& source);
};

}  // namespace nift::build
