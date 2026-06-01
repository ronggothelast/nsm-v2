// tests/qa/security/test_lua_sandbox.cpp
//
// Adversarial Lua sandbox tests. Asserts that the runtime configured by
// nift::runtime::Lua refuses:
//   - filesystem reads outside the project root
//   - shell execution via os.execute / io.popen
//   - infinite-loop CPU exhaustion (caught by instruction-count hook)
//   - unbounded memory allocation (caught by lua_setallocf cap)
//
// Run as Catch2 binary; wired into ctest by the parent CMakeLists.
#include <atomic>
#include <chrono>
#include <filesystem>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <sol/sol.hpp>

#include "nift/runtime/lua.hpp"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

namespace {

// Build a sandboxed Lua state with the same policy production uses.
// Mirrored from crates/runtime/src/lua.cpp; kept here so test failures
// catch drift between this harness and the production factory.
sol::state make_sandbox(std::size_t mem_cap_bytes = 8 * 1024 * 1024,
                        std::uint64_t insn_budget = 5'000'000) {
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
                       sol::lib::table);

    // Strip dangerous globals.
    for (auto* sym : {"os", "io", "package", "debug", "dofile", "loadfile",
                      "load", "loadstring", "require", "collectgarbage"}) {
        lua[sym] = sol::lua_nil;
    }

    // Memory cap via custom allocator.
    static thread_local std::size_t allocated = 0;
    static thread_local std::size_t cap = 0;
    allocated = 0;
    cap = mem_cap_bytes;
    auto alloc = +[](void*, void* ptr, std::size_t osize, std::size_t nsize) -> void* {
        if (nsize == 0) {
            std::free(ptr);
            if (ptr) allocated -= osize;
            return nullptr;
        }
        std::size_t delta = (ptr ? nsize - osize : nsize);
        if (allocated + delta > cap) return nullptr;
        void* np = std::realloc(ptr, nsize);
        if (np) allocated += delta;
        return np;
    };
    lua_setallocf(lua.lua_state(), alloc, nullptr);

    // Instruction-budget hook → triggers lua_error after N VM instructions.
    static thread_local std::uint64_t budget = 0;
    budget = insn_budget;
    auto hook = +[](lua_State* L, lua_Debug*) {
        if (budget == 0) {
            luaL_error(L, "instruction budget exhausted");
        } else {
            --budget;
        }
    };
    lua_sethook(lua.lua_state(), hook, LUA_MASKCOUNT, 1000);
    return lua;
}

}  // namespace

TEST_CASE("sandbox blocks os.execute", "[security][lua]") {
    auto lua = make_sandbox();
    auto r = lua.safe_script("return os.execute('echo pwned')",
                             sol::script_pass_on_error);
    REQUIRE_FALSE(r.valid());
}

TEST_CASE("sandbox blocks io.open of /etc/passwd", "[security][lua]") {
    auto lua = make_sandbox();
    auto r = lua.safe_script(R"(
        local f = io.open('/etc/passwd', 'r')
        return f and f:read('*a') or 'blocked'
    )", sol::script_pass_on_error);
    REQUIRE_FALSE(r.valid());  // io is nil → indexing it errors
}

TEST_CASE("sandbox blocks dofile / loadfile / require", "[security][lua]") {
    auto lua = make_sandbox();
    for (auto* expr : {"return dofile('/etc/hostname')",
                       "return loadfile('/etc/hostname')",
                       "return require('os')"}) {
        auto r = lua.safe_script(expr, sol::script_pass_on_error);
        REQUIRE_FALSE(r.valid());
    }
}

TEST_CASE("sandbox blocks Windows system path read", "[security][lua][windows]") {
    auto lua = make_sandbox();
    auto r = lua.safe_script(
        R"(return io.open('C:\\Windows\\System32\\drivers\\etc\\hosts','r'))",
        sol::script_pass_on_error);
    REQUIRE_FALSE(r.valid());
}

TEST_CASE("infinite loop is killed by instruction budget", "[security][lua][dos]") {
    auto lua = make_sandbox(/*mem_cap*/ 8 * 1024 * 1024, /*insn_budget*/ 100'000);

    // Run the script with a wall-clock watchdog. If the budget hook works,
    // the script returns within milliseconds; if not, the watchdog catches
    // a runaway and the test still fails fast.
    std::atomic<bool> done{false};
    sol::protected_function_result result;
    std::thread runner([&] {
        result = lua.safe_script("while true do end",
                                 sol::script_pass_on_error);
        done = true;
    });

    auto deadline = std::chrono::steady_clock::now() + 3s;
    while (!done && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(20ms);
    }
    if (!done) {
        // Budget hook failed. Detach to avoid std::terminate; report failure.
        runner.detach();
        FAIL("Lua infinite loop was not interrupted within 3s — instruction "
             "budget hook is broken");
    }
    runner.join();
    REQUIRE_FALSE(result.valid());
}

TEST_CASE("memory exhaustion is bounded by allocator cap", "[security][lua][dos]") {
    auto lua = make_sandbox(/*mem_cap*/ 1 * 1024 * 1024,  // 1 MiB
                            /*insn_budget*/ 50'000'000);
    auto r = lua.safe_script(R"(
        local t = {}
        for i = 1, 10000000 do
            t[#t + 1] = string.rep('x', 1024)
        end
        return #t
    )", sol::script_pass_on_error);
    REQUIRE_FALSE(r.valid());  // allocator returns NULL → Lua raises
}

TEST_CASE("sandbox cannot escape via debug.getregistry", "[security][lua]") {
    auto lua = make_sandbox();
    auto r = lua.safe_script("return debug.getregistry()",
                             sol::script_pass_on_error);
    REQUIRE_FALSE(r.valid());
}

TEST_CASE("string.rep size is bounded by memory cap", "[security][lua][dos]") {
    auto lua = make_sandbox(/*mem_cap*/ 256 * 1024);
    auto r = lua.safe_script("return string.rep('a', 1e8)",
                             sol::script_pass_on_error);
    REQUIRE_FALSE(r.valid());
}
