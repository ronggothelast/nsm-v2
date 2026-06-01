# crates/server/CMakeLists.txt
# nift::server — dev server, file watcher, asset pipeline.

add_library(nift_server STATIC
  http/http_server.cpp
  watcher/file_watcher.cpp
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

# cpp-httplib uses Win8+ APIs (GetAddrInfoExCancel) that MinGW only exposes
# when _WIN32_WINNT >= 0x0602. Bump the target version on Windows. Use
# PUBLIC so anything linking nift_server (e.g. test_http_server.cpp) gets it.
if(WIN32)
  target_compile_definitions(nift_server PUBLIC
    _WIN32_WINNT=0x0A00
    WINVER=0x0A00
  )
endif()

set_target_properties(nift_server PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
