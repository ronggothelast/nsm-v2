#!/usr/bin/env bash
# tests/qa/fuzzing/run_fuzz.sh
# Short fuzzing loop; each target gets ${RUNTIME_SECS:-60}s.
# Build with: cmake -S . -B build/fuzz -DNIFT_QA_FUZZING=ON \
#                   -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
#             cmake --build build/fuzz
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build/fuzz}"
RUNTIME_SECS="${RUNTIME_SECS:-60}"
CORPUS="$ROOT/tests/qa/fuzzing/corpus"

# Seed corpus from real inputs to give the fuzzer a head start.
mkdir -p "$CORPUS"/{parser,simd,cli}
[[ -f "$CORPUS/parser/seed_basic" ]] || \
    printf '@layout(default)\n@for(i;0;3)<li>@var(i)</li>@endfor' > "$CORPUS/parser/seed_basic"
[[ -f "$CORPUS/simd/seed_markers" ]] || \
    printf '%.0sxxxxxxxxxxxxxxxx@xxxxxxxxxxxxxxxx${a}xxxxxxxx' {1..16} > "$CORPUS/simd/seed_markers"
[[ -f "$CORPUS/cli/seed_args" ]] || \
    printf 'build\0--source\0./content\0--out\0./output' > "$CORPUS/cli/seed_args"

run_one() {
    local target="$1" sub="$2"
    local bin="$BUILD/tests/qa/fuzzing/$target"
    [[ -x "$bin" ]] || { echo "FAIL: missing $bin (build with -DNIFT_QA_FUZZING=ON)"; return 2; }
    echo "==> $target — ${RUNTIME_SECS}s"
    "$bin" "$CORPUS/$sub" \
        -max_total_time="$RUNTIME_SECS" \
        -timeout=10 \
        -rss_limit_mb=2048 \
        -print_final_stats=1
}

run_one fuzz_parser       parser
run_one fuzz_simd_scanner simd
run_one fuzz_cli          cli
echo "OK: fuzzing run complete"
