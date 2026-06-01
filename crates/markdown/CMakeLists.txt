# ─── nift::markdown library ─────────────────────────────────────────
# Markdown parsing via cmark-gfm, front matter extraction, code highlighting.
# Depends on: nift::core, cmark-gfm, nlohmann_json

add_library(nift_markdown STATIC
  src/markdown.cpp
  src/front_matter.cpp
  src/code_highlight.cpp
)
add_library(nift::markdown ALIAS nift_markdown)

target_include_directories(nift_markdown
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(nift_markdown
  PUBLIC
    nift::core
    libcmark-gfm-extensions_static libcmark-gfm_static
    nlohmann_json::nlohmann_json
    fmt::fmt
)

target_compile_features(nift_markdown PUBLIC cxx_std_20)
