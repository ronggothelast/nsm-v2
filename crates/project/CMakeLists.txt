# crates/project/CMakeLists.txt
# nift::project — ProjectInfo + BuildCache.

add_library(nift_project STATIC
  info/project.cpp
  cache/build_cache.cpp
)

add_library(nift::project ALIAS nift_project)

target_include_directories(nift_project PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(nift_project
  PUBLIC
    nift::core
    nlohmann_json::nlohmann_json
  PRIVATE
    BLAKE3::blake3
)

set_target_properties(nift_project PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
