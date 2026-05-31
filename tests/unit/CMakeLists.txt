# ─── unit tests for nift::core ───────────────────────────────────────

add_executable(nift_core_tests
  test_path.cpp
  test_filesystem.cpp
  test_str_fns.cpp
  test_quoted.cpp
  test_datetime.cpp
  test_system_info.cpp
  test_types.cpp
)

target_link_libraries(nift_core_tests
  PRIVATE
    nift::core
    Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH ${Catch2_DIR})
include(CTest)
include(Catch)
catch_discover_tests(nift_core_tests)
