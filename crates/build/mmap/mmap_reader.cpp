/// @file mmap_reader.cpp

#include "nift/build/mmap_reader.hpp"

#include <fstream>
#include <mio/mmap.hpp>
#include <sstream>
#include <system_error>
#include <utility>

#include "nift/core/filesystem.hpp"

namespace nift::build {

struct MmapReader::Impl {
  mio::mmap_source map;  // primary path
  std::string buffered;  // fallback path (small files / errors)
  bool mapped = false;
  std::size_t bytes = 0;
};

MmapReader::MmapReader() : impl_(std::make_unique<Impl>()) {
}
MmapReader::~MmapReader() = default;
MmapReader::MmapReader(MmapReader&&) noexcept = default;
MmapReader& MmapReader::operator=(MmapReader&&) noexcept = default;

::nift::Expected<MmapReader, ::nift::Error> MmapReader::open(
    const ::nift::core::Path& path) {
  if (!::nift::core::file_exists(path)) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::not_found);
  }

  MmapReader r;

  // Try mmap first.
  std::error_code ec;
  r.impl_->map = mio::make_mmap_source(path.str(), 0, mio::map_entire_file, ec);
  if (!ec && r.impl_->map.is_open() && r.impl_->map.size() > 0) {
    r.impl_->mapped = true;
    r.impl_->bytes = r.impl_->map.size();
    return r;
  }

  // Fallback: heap read (handles 0-byte files mmap rejects, plus
  // platforms / FS that don't support mmap — e.g. some Docker overlays).
  auto content = ::nift::core::read_file(path);
  if (!content) {
    return ::nift::unexpected<::nift::Error>(content.error());
  }
  r.impl_->buffered = std::move(*content);
  r.impl_->mapped = false;
  r.impl_->bytes = r.impl_->buffered.size();
  return r;
}

std::string_view MmapReader::data() const noexcept {
  if (impl_->mapped) {
    return std::string_view(impl_->map.data(), impl_->map.size());
  }
  return std::string_view(impl_->buffered.data(), impl_->buffered.size());
}

std::size_t MmapReader::size() const noexcept {
  return impl_->bytes;
}

bool MmapReader::is_mapped() const noexcept {
  return impl_->mapped;
}

}  // namespace nift::build
