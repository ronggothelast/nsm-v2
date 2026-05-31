# ─── nift::core library ──────────────────────────────────────────────
# Low-level utilities: path, filesystem, string, time, system info.
# Zero external deps beyond fmt + spdlog.

add_library(nift_core STATIC)
add_library(nift::core ALIAS nift_core)

target_sources(nift_core
  PRIVATE
    path/path.cpp
    fs/filesystem.cpp
    str/str_fns.cpp
    str/quoted.cpp
    time/datetime.cpp
    sys/system_info.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../../
    FILES
      include/nift/core/path.hpp
      include/nift/core/filesystem.hpp
      include/nift/core/str_fns.hpp
      include/nift/core/quoted.hpp
      include/nift/core/datetime.hpp
      include/nift/core/system_info.hpp
      include/nift/core/types.hpp
)

target_include_directories(nift_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(nift_core
  PUBLIC
    fmt::fmt
    spdlog::spdlog
)

target_compile_features(nift_core PUBLIC cxx_std_20)
