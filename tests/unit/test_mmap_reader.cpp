/// @file test_mmap_reader.cpp

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include "nift/build/mmap_reader.hpp"

using namespace nift::build;
namespace fs = std::filesystem;

namespace {
::nift::core::Path write_tmp(const std::string& tag, const std::string& content) {
  auto p = fs::temp_directory_path() /
           ("nift_mmap_" + tag + "_" + std::to_string(std::rand()) + ".txt");
  {
    std::ofstream out(p, std::ios::binary);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
  }
  return ::nift::core::Path(p.string());
}
}  // namespace

TEST_CASE("MmapReader: small text file", "[build][mmap]") {
  auto path = write_tmp("small", "hello world");
  auto r = MmapReader::open(path);
  REQUIRE(r);
  CHECK(r->size() == 11);
  CHECK(r->data() == "hello world");
}

TEST_CASE("MmapReader: missing file → not_found", "[build][mmap]") {
  ::nift::core::Path missing("/tmp/__definitely_not_here_nift__");
  auto r = MmapReader::open(missing);
  REQUIRE_FALSE(r);
  CHECK(r.error() == ::nift::Error::not_found);
}

TEST_CASE("MmapReader: empty file falls back to buffered", "[build][mmap]") {
  auto path = write_tmp("empty", "");
  auto r = MmapReader::open(path);
  REQUIRE(r);
  CHECK(r->size() == 0);
  CHECK(r->is_mapped() == false);  // mmap rejects 0-byte → fallback
}

TEST_CASE("MmapReader: large file mmaps", "[build][mmap]") {
  std::string big(64 * 1024, 'X');
  auto path = write_tmp("big", big);
  auto r = MmapReader::open(path);
  REQUIRE(r);
  CHECK(r->size() == big.size());
  CHECK(r->is_mapped() == true);
  // Spot-check first/last bytes.
  CHECK(r->data().front() == 'X');
  CHECK(r->data().back() == 'X');
}

TEST_CASE("MmapReader: data view stable across moves", "[build][mmap]") {
  auto path = write_tmp("move", "stable bytes");
  auto r1 = MmapReader::open(path);
  REQUIRE(r1);
  auto r2 = std::move(*r1);
  CHECK(r2.data() == "stable bytes");
}
