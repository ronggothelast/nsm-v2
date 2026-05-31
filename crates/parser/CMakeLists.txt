# ─── nift::parser library ──────────────────────────────────────────
# Template parser: lexer, AST, evaluator, directive handlers.
# Depends on: nift::core

add_library(nift_parser
  lexer/lexer.cpp
  lexer/token.cpp
  ast/ast.cpp
  ast/parser.cpp
  evaluator/evaluator.cpp
)

target_include_directories(nift_parser
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(nift_parser
  PUBLIC
    nift::core
    fmt::fmt-header-only
)

target_compile_features(nift_parser PUBLIC cxx_std_20)

add_library(nift::parser ALIAS nift_parser)
