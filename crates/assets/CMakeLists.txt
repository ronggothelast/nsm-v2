# ─── nift::assets library ────────────────────────────────────────────
# Asset pipeline: CSS minification, JS bundling, fingerprinting.
# Uses esbuild + lightningcss via subprocess.

add_library(nift_assets STATIC
  src/pipeline.cpp
)
add_library(nift::assets ALIAS nift_assets)

target_include_directories(nift_assets
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(nift_assets
  PUBLIC
    nift::core
    nlohmann_json::nlohmann_json
    BLAKE3::blake3
    fmt::fmt
)

target_compile_features(nift_assets PUBLIC cxx_std_20)
