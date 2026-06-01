# Contributing to Nift v2

## Development setup

```bash
git clone --recurse-submodules https://github.com/ronggothelast/nsm-v2.git
cd nsm-v2
just build    # configure + build debug
just test     # run all tests
```

**Requirements:** CMake ≥ 3.25, C++20 compiler (GCC 11+, Clang 14+, MSVC 2022+), Git, Ninja.

## Workflow

1. Fork the repo
2. Create a feature branch: `git checkout -b feature/my-thing`
3. Make changes
4. Run tests: `just test`
5. Format code: `just fmt`
6. Commit with conventional prefixes (see below)
7. Open a PR against `main`

## Conventional commits

```
feat: add new template directive
fix: handle empty @for loop
perf: optimize SIMD scanner alignment
refactor: extract lexer context into struct
docs: update CLI reference
test: add edge case for nested @if
chore: bump vcpkg baseline
ci: add Windows arm64 runner
build: enable LTO in release preset
```

## Code style

- **Formatter:** clang-format (Google-based, 88-col, 2-space indent). Config in `.clang-format`.
- **Linter:** clang-tidy. Config in `.clang-tidy`.
- **Editor:** `.editorconfig` enforces spaces, LF, UTF-8.
- Run `just fmt` before committing. CI enforces `just fmt-check`.

## Adding a new crate

1. Create directory: `crates/mycrate/`
2. Add `include/nift/mycrate/*.hpp` for public headers
3. Add `src/*.cpp` for implementations
4. Add `CMakeLists.txt` with library target
5. Add `crates/mycrate/test/test_mycrate.cpp` with Catch2 tests
6. Wire into root `CMakeLists.txt` and `tests/CMakeLists.txt`
7. Write an ADR if the crate makes architectural decisions: `docs/adr/NNNN-my-decision.md`

## Testing

- **Framework:** Catch2 v3
- **Location:** `crates/*/test/*.cpp` for unit tests, `tests/` for integration/QA
- **Run:** `just test` (debug) or `just test-asan` (with sanitizers)
- **Coverage:** `just test-coverage` (requires lcov)
- **Fuzz:** `just qa-fuzz` (requires Clang)

Every public function in `crates/core/` should have at least one test. Target: 85%+ line coverage on core.

## Architecture decisions

Significant design choices get an ADR in `docs/adr/`. See `docs/adr/0000-process.md` for the template. Examples:

- Choosing vcpkg over Conan (ADR 0002)
- C++20 baseline with C++23 feature detection (ADR 0003)
- sol2 for Lua binding (ADR 0004)

## Sanitizers

Run with sanitizers before submitting:

```bash
just test-asan    # AddressSanitizer — memory errors
just test-ubsan   # UndefinedBehaviorSanitizer — UB
just test-tsan    # ThreadSanitizer — data races
```

CI runs all three on every PR.

## Questions

Open an issue or start a discussion on GitHub.
