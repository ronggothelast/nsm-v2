# crates/build/CMakeLists.txt
# nift::build — work-stealing executor, mmap reader, SIMD scanner, pipeline.

add_library(nift_build STATIC
  executor/executor.cpp
  mmap/mmap_reader.cpp
  scanner/simd_scanner.cpp
  pipeline/pipeline.cpp
)

add_library(nift::build ALIAS nift_build)

target_include_directories(nift_build PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(nift_build
  PUBLIC
    nift::core
    nift::project
  PRIVATE
    mio::mio
    xsimd
    Threads::Threads
)

# pthread on Linux/macOS
find_package(Threads REQUIRED)

set_target_properties(nift_build PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
