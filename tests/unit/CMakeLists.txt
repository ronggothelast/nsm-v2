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
  test_template_inheritance.cpp
  test_function_invocation.cpp
)

target_link_libraries(nift_parser_tests
  PRIVATE
    nift::parser
    Catch2::Catch2WithMain
)

# ─── unit tests for nift::markdown ────────────────────────────────────

add_executable(nift_markdown_tests
  test_markdown.cpp
)

target_link_libraries(nift_markdown_tests
  PRIVATE
    nift::markdown
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
  test_cache_extra.cpp
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

# ─── unit tests for nift::server ─────────────────────────────────────

add_executable(nift_server_tests
  test_assets.cpp
  test_http_server.cpp
  test_file_watcher.cpp
  test_native_watcher.cpp
)

target_link_libraries(nift_server_tests
  PRIVATE
    nift::server
    httplib::httplib
    Catch2::Catch2WithMain
)

# ─── unit tests for nift::cli, nift::compat, nift::plugin ───────────

add_executable(nift_cli_tests
  test_argparse.cpp
  test_migrator.cpp
  test_plugin_loader.cpp
)

target_link_libraries(nift_cli_tests
  PRIVATE
    nift::cli
    nift::compat
    nift::plugin
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(nift_core_tests)
catch_discover_tests(nift_parser_tests)
catch_discover_tests(nift_markdown_tests)
catch_discover_tests(nift_runtime_tests)
catch_discover_tests(nift_project_tests)
# FileWatcher tests poll the filesystem with short timeouts; under heavy
# parallel CTest load (especially sanitizers) they can time out and report
# spurious failures. Serialize them via RESOURCE_LOCK so they don't compete
# with each other while still letting the rest of the suite run in parallel.
catch_discover_tests(nift_build_tests
  PROPERTIES
    RESOURCE_LOCK file_watcher_fs
    TIMEOUT 60
)
# Server tests bind to ephemeral TCP ports + filesystem under sanitizer load —
# serialize them so port races and slow boot don't cause spurious failures.
catch_discover_tests(nift_server_tests
  PROPERTIES
    RESOURCE_LOCK http_server_port
    TIMEOUT 60
)
catch_discover_tests(nift_cli_tests)

# ─── unit tests for nift::tailwind ──────────────────────────────────

add_executable(nift_tailwind_tests
  test_tailwind.cpp
)

target_link_libraries(nift_tailwind_tests
  PRIVATE
    nift::tailwind
    Catch2::Catch2WithMain
)

catch_discover_tests(nift_tailwind_tests)

# ─── unit tests for nift::assets ────────────────────────────────────

add_executable(nift_assets_tests
  test_assets_pipeline.cpp
)

target_link_libraries(nift_assets_tests
  PRIVATE
    nift::assets
    Catch2::Catch2WithMain
)

catch_discover_tests(nift_assets_tests)

# ─── unit tests for nift::images ────────────────────────────────────

add_executable(nift_images_tests
  test_images.cpp
)

target_link_libraries(nift_images_tests
  PRIVATE
    nift::images
    Catch2::Catch2WithMain
)

catch_discover_tests(nift_images_tests)
