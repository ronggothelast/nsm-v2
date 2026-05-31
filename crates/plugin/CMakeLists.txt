# crates/plugin/CMakeLists.txt — dynamic plugin loader.

add_library(nift_plugin STATIC src/plugin_loader.cpp)
add_library(nift::plugin ALIAS nift_plugin)

target_include_directories(nift_plugin PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(nift_plugin PUBLIC nift::core)

if(UNIX AND NOT APPLE)
  target_link_libraries(nift_plugin PUBLIC ${CMAKE_DL_LIBS})
endif()

set_target_properties(nift_plugin PROPERTIES POSITION_INDEPENDENT_CODE ON)
