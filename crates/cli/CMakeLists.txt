# crates/cli/CMakeLists.txt — CLI argparse + subcommand dispatch.

add_library(nift_cli STATIC
  src/argparse.cpp
  src/commands.cpp
)
add_library(nift::cli ALIAS nift_cli)

target_include_directories(nift_cli PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(nift_cli
  PUBLIC
    nift::core
    nift::project
    nift::build
    nift::server
    nift::compat
)

set_target_properties(nift_cli PROPERTIES POSITION_INDEPENDENT_CODE ON)
