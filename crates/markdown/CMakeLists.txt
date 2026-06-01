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

# Link cmark-gfm — use imported targets when available (Linux),
# fall back to find_library for Windows where target names differ.
if(TARGET libcmark-gfm-extensions_static AND TARGET libcmark-gfm_static)
  # Linux/macOS: use vcpkg imported targets (correct link order: ext before main).
  target_link_libraries(nift_markdown
    PUBLIC
      nift::core
      libcmark-gfm-extensions_static
      libcmark-gfm_static
      nlohmann_json::nlohmann_json
      fmt::fmt
  )
else()
  # Windows/other: find_library with correct link order.
  find_library(CMARK_GFM_EXT_LIB NAMES cmark-gfm-extensions libcmark-gfm-extensions)
  find_library(CMARK_GFM_LIB NAMES cmark-gfm libcmark-gfm)

  if(NOT CMARK_GFM_LIB)
    message(FATAL_ERROR "cmark-gfm library not found")
  endif()

  # Extensions must link BEFORE main lib (dependency direction).
  set(_CMARK_LIBS "")
  if(CMARK_GFM_EXT_LIB)
    list(APPEND _CMARK_LIBS ${CMARK_GFM_EXT_LIB})
  endif()
  list(APPEND _CMARK_LIBS ${CMARK_GFM_LIB})

  target_link_libraries(nift_markdown
    PUBLIC
      nift::core
      ${_CMARK_LIBS}
      nlohmann_json::nlohmann_json
      fmt::fmt
  )
endif()

target_compile_features(nift_markdown PUBLIC cxx_std_20)
