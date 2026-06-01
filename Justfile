# Justfile — Nift v2 task runner
# Usage: just <recipe>

# Default: build debug
default: build

# ─── Build ────────────────────────────────────────────────────────────

# Configure + build debug
build:
  cmake --preset debug
  cmake --build --preset debug

# Build release
release:
  cmake --preset release
  cmake --build --preset release

# Clean build artifacts
clean:
  rm -rf build/

# ─── Test ─────────────────────────────────────────────────────────────

# Run all tests (debug build)
test: build
  ctest --preset debug --output-on-failure

# Run tests with AddressSanitizer
test-asan:
  cmake --preset asan
  cmake --build --preset asan
  ctest --preset asan --output-on-failure

# Run tests with UBSan
test-ubsan:
  cmake --preset ubsan
  cmake --build --preset ubsan
  ctest --preset ubsan --output-on-failure

# Run tests with ThreadSanitizer
test-tsan:
  cmake --preset tsan
  cmake --build --preset tsan

# Run tests with code coverage
test-coverage:
  cmake --preset debug -DNIFT_COVERAGE=ON
  cmake --build --preset debug
  ctest --preset debug --output-on-failure
  @echo "Coverage data in build/debug/**/*.gcda"
  @echo "Run: lcov --capture -d build/debug -o coverage.info && genhtml coverage.info -o coverage-report"

# ─── Quality ─────────────────────────────────────────────────────────

# Format check (dry run)
fmt-check:
  find crates apps tests -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | \
    xargs clang-format --dry-run --Werror 2>&1

# Format fix
fmt:
  find crates apps tests -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | \
    xargs clang-format -i

# Lint with clang-tidy (requires compile_commands.json from debug build)
lint: build
  find crates -name '*.cpp' | \
    xargs clang-tidy -p build/debug --warnings-as-errors='*' 2>&1

# ─── Dev ──────────────────────────────────────────────────────────────

# Build + test + lint (full local CI)
ci: build test
  @echo "✅ Local CI passed"

# Watch for changes (requires entr)
dev:
  find crates tests -name '*.cpp' -o -name '*.hpp' | \
    entr -c just test

# ─── QA suite (adversarial) ──────────────────────────────────────────

NIFT_BIN := "build/release/apps/nift/nift"

# Run every QA domain (excluding nightly fuzzing+parity)
qa: release qa-chaos qa-security qa-recovery qa-network
  @echo "✅ QA suite green"

# Domain 1 — legacy parity (requires legacy/nift binary)
qa-parity: release
  bash tests/qa/parity/run_parity.sh

# Domain 2 — filesystem chaos
qa-chaos: release
  NIFT_BIN={{NIFT_BIN}} python3 tests/qa/chaos/test_chaos.py

# Domain 3 — Lua sandbox (Catch2 binary, built via debug preset)
qa-security:
  cmake --preset debug -DNIFT_QA_SECURITY=ON
  cmake --build --preset debug --target qa_lua_sandbox
  ctest --preset debug -R "^qa_lua_sandbox" --output-on-failure

# Domain 4 — signal handling
qa-recovery: release
  NIFT_BIN={{NIFT_BIN}} python3 tests/qa/recovery/test_signal_recovery.py

# Domain 5 — fuzzing (60s/target by default)
qa-fuzz:
  CC=clang CXX=clang++ cmake -S . -B build/fuzz -G Ninja -DNIFT_QA_FUZZING=ON -DCMAKE_BUILD_TYPE=Release
  cmake --build build/fuzz
  BUILD_DIR=build/fuzz RUNTIME_SECS=60 bash tests/qa/fuzzing/run_fuzz.sh

# Domain 6 — server traversal & stress
qa-network: release
  NIFT_BIN={{NIFT_BIN}} python3 tests/qa/network/test_server_security.py
