# tests/qa/security/CMakeLists.txt
# Built only when QA security suite is requested; pulls sol2 + Lua + Catch2.
find_package(sol2 CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(Catch2 3 CONFIG REQUIRED)

add_executable(qa_lua_sandbox test_lua_sandbox.cpp)
target_link_libraries(qa_lua_sandbox
    PRIVATE
        nift::runtime
        sol2::sol2
        ${LUA_LIBRARIES}
        Catch2::Catch2WithMain
)
target_include_directories(qa_lua_sandbox PRIVATE ${LUA_INCLUDE_DIR})
target_compile_features(qa_lua_sandbox PRIVATE cxx_std_20)

include(Catch)
catch_discover_tests(qa_lua_sandbox)
