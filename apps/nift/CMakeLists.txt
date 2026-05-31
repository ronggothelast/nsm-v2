# apps/nift/CMakeLists.txt — main CLI binary.

add_executable(nift main.cpp)

target_link_libraries(nift
  PRIVATE
    nift::cli
)

set_target_properties(nift PROPERTIES OUTPUT_NAME "nift")
