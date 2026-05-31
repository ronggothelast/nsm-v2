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

# ─── unit tests for nift::parser ─────────────────────────────────────

add_executable(nift_parser_tests
  test_lexer.cpp
  test_parser.cpp
  test_evaluator.cpp
)

target_link_libraries(nift_parser_tests
  PRIVATE
    nift::parser
    Catch2::Catch2WithMain
)

# ─── unit tests for nift::runtime ────────────────────────────────────

add_executable(nift_runtime_tests
  test_lua_runtime.cpp
  test_expr.cpp
)

target_link_libraries(nift_runtime_tests
  PRIVATE
    nift::runtime
    Catch2::Catch2WithMain
)

# ─── unit tests for nift::project ────────────────────────────────────

add_executable(nift_project_tests
  test_project.cpp
  test_build_cache.cpp
)

target_link_libraries(nift_project_tests
  PRIVATE
    nift::project
    Catch2::Catch2WithMain
)

# ─── unit tests for nift::build ──────────────────────────────────────

add_executable(nift_build_tests
  test_executor.cpp
  test_mmap_reader.cpp
  test_simd_scanner.cpp
  test_pipeline.cpp
)

target_link_libraries(nift_build_tests
  PRIVATE
    nift::build
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(nift_core_tests)
catch_discover_tests(nift_parser_tests)
catch_discover_tests(nift_runtime_tests)
catch_discover_tests(nift_project_tests)
catch_discover_tests(nift_build_tests)
