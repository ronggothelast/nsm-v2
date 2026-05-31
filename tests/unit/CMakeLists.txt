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

include(CTest)
include(Catch)
catch_discover_tests(nift_core_tests)
catch_discover_tests(nift_parser_tests)
