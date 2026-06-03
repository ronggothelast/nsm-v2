# crates/server/CMakeLists.txt
# nift::server — dev server, file watcher, asset pipeline.

add_library(nift_server STATIC
  http/http_server.cpp
  watcher/file_watcher.cpp
  watcher/native_watcher.cpp
  assets/assets.cpp
)

add_library(nift::server ALIAS nift_server)

target_include_directories(nift_server PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(nift_server
  PUBLIC
    nift::core
  PRIVATE
    httplib::httplib
    Threads::Threads
)

set_target_properties(nift_server PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
