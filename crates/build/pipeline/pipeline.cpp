/// @file pipeline.cpp

#include "nift/build/pipeline.hpp"

#include <future>
#include <utility>

#include "nift/build/mmap_reader.hpp"
#include "nift/core/filesystem.hpp"
#include "nift/project/build_cache.hpp"

namespace nift::build {

Pipeline::Pipeline(::nift::project::ProjectConfig config,
                   ::nift::project::BuildCache* cache, std::size_t pool_size)
    : config_(std::move(config)), cache_(cache), pool_(pool_size) {
}

FileResult Pipeline::build_one(const ::nift::core::Path& source) {
  FileResult r;
  r.source = source;

  // Read source via mmap (zero-copy).
  auto reader = MmapReader::open(source);
  if (!reader) {
    r.status = FileStatus::Failed;
    r.error_message = "open failed";
    return r;
  }

  std::string_view src_view = reader->data();

  // Compute content hash.
  std::string content_hash = ::nift::project::hash_content(src_view);

  // Cache check.
  if (cache_) {
    if (!cache_->is_dirty(source, content_hash)) {
      r.status = FileStatus::Cached;
      return r;
    }
  }

  // Output path: same name under config_.output_dir.
  ::nift::core::Path output(config_.output_dir.str() + "/" +
                            source.native().filename().string());

  // Make sure output dir exists.
  (void)::nift::core::create_directories(config_.output_dir);

  // Phase 4 placeholder: copy source bytes to output. Phase 6 wires to
  // the parser+evaluator from the parser crate.
  auto write_status = ::nift::core::write_file(output, src_view);
  if (!write_status) {
    r.status = FileStatus::Failed;
    r.error_message = "write failed";
    return r;
  }

  // Update cache.
  if (cache_) {
    ::nift::project::TrackedFile tf;
    tf.source = source;
    tf.output = output;
    tf.content_hash = content_hash;
    tf.deps_hash = content_hash;  // no deps yet
    cache_->put(tf);
  }

  r.status = FileStatus::Built;
  return r;
}

BuildReport Pipeline::build(const std::vector<::nift::core::Path>& inputs) {
  BuildReport report;
  auto t0 = std::chrono::steady_clock::now();

  // Submit all tasks.
  std::vector<std::future<FileResult>> futures;
  futures.reserve(inputs.size());
  for (const auto& src : inputs) {
    futures.push_back(pool_.submit([this, src] { return build_one(src); }));
  }

  // Collect.
  report.file_results.reserve(inputs.size());
  for (auto& f : futures) {
    FileResult r = f.get();
    switch (r.status) {
      case FileStatus::Built:
        ++report.built;
        break;
      case FileStatus::Cached:
        ++report.cached;
        break;
      case FileStatus::Failed:
        ++report.failed;
        break;
    }
    report.file_results.push_back(std::move(r));
  }

  auto t1 = std::chrono::steady_clock::now();
  report.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
  return report;
}

}  // namespace nift::build
