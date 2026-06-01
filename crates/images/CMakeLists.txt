# ─── nift::images library ────────────────────────────────────────────
# Image optimization: resize, format conversion, responsive images.
# Uses external tools via subprocess (vipsthumbnail, cwebp, avifenc).

add_library(nift_images STATIC
  src/optimizer.cpp
)
add_library(nift::images ALIAS nift_images)

target_include_directories(nift_images
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(nift_images
  PUBLIC
    nift::core
    fmt::fmt
)

target_compile_features(nift_images PUBLIC cxx_std_20)
