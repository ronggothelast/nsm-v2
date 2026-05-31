# crates/runtime/CMakeLists.txt
# nift::runtime — Lua bridge + expression evaluator.

add_library(nift_runtime STATIC
  lua/lua_runtime.cpp
  expr/expr.cpp
)

add_library(nift::runtime ALIAS nift_runtime)

target_include_directories(nift_runtime PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(nift_runtime
  PUBLIC
    nift::core
  PRIVATE
    sol2::sol2
    ${LUA_LIBRARIES}
)

target_include_directories(nift_runtime PRIVATE ${LUA_INCLUDE_DIR})

set_target_properties(nift_runtime PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
