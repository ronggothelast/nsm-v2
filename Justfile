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
