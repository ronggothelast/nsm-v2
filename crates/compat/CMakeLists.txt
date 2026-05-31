# crates/compat/CMakeLists.txt — v1 → v2 migrator.

add_library(nift_compat STATIC src/migrator.cpp)
add_library(nift::compat ALIAS nift_compat)

target_include_directories(nift_compat PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(nift_compat PUBLIC nift::core nift::project)

set_target_properties(nift_compat PROPERTIES POSITION_INDEPENDENT_CODE ON)
