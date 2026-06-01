# ─── nift::tailwind library ──────────────────────────────────────────
# Tailwind CSS integration via subprocess (Tailwind CLI).
# Depends on: nift::core, nlohmann_json

add_library(nift_tailwind STATIC
  src/tailwind.cpp
)
add_library(nift::tailwind ALIAS nift_tailwind)

target_include_directories(nift_tailwind
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(nift_tailwind
  PUBLIC
    nift::core
    nlohmann_json::nlohmann_json
    fmt::fmt
)

target_compile_features(nift_tailwind PUBLIC cxx_std_20)
