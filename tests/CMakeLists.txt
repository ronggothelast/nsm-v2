# ─── tests ───────────────────────────────────────────────────────────

add_subdirectory(unit)

if(NIFT_BUILD_BENCH)
  add_subdirectory(bench)
endif()
