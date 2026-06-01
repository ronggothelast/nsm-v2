#!/usr/bin/env bash
# tests/qa/parity/run_parity.sh
#
# Build the same mock project with legacy Nift (v1) and Nift v2, then
# byte-for-byte compare the output trees. Exit non-zero on any drift.
#
# Inputs:
#   NIFT_V1_BIN  — path to legacy nift binary (default: legacy/nift)
#   NIFT_V2_BIN  — path to v2 binary          (default: build/debug/apps/nift/nift)
#
# Usage:
#   ./tests/qa/parity/run_parity.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
QA="$ROOT/tests/qa/parity"
MOCK="$QA/mock_project"
WORK="$(mktemp -d -t nift-parity-XXXXXX)"
trap 'rm -rf "$WORK"' EXIT

NIFT_V1_BIN="${NIFT_V1_BIN:-$ROOT/legacy/nift}"
NIFT_V2_BIN="${NIFT_V2_BIN:-$ROOT/build/debug/apps/nift/nift}"

[[ -x "$NIFT_V1_BIN" ]] || { echo "FAIL: legacy nift not at $NIFT_V1_BIN"; exit 2; }
[[ -x "$NIFT_V2_BIN" ]] || { echo "FAIL: v2 nift not at $NIFT_V2_BIN"; exit 2; }

cp -R "$MOCK" "$WORK/v1"
cp -R "$MOCK" "$WORK/v2"

echo "==> v1 build"
( cd "$WORK/v1" && "$NIFT_V1_BIN" build >v1.log 2>&1 ) \
    || { echo "v1 build failed"; cat "$WORK/v1/v1.log"; exit 3; }

echo "==> v2 build"
( cd "$WORK/v2" && "$NIFT_V2_BIN" build >v2.log 2>&1 ) \
    || { echo "v2 build failed"; cat "$WORK/v2/v2.log"; exit 3; }

V1_OUT="$WORK/v1/output"
V2_OUT="$WORK/v2/output"
[[ -d "$V1_OUT" ]] || { echo "FAIL: v1 produced no output dir"; exit 4; }
[[ -d "$V2_OUT" ]] || { echo "FAIL: v2 produced no output dir"; exit 4; }

# Normalize: strip build timestamps that v1 historically embedded as
# `<!-- nift-built: ... -->` so we compare semantic output, not generation time.
strip_volatiles() {
    find "$1" -type f \( -name '*.html' -o -name '*.xml' -o -name '*.txt' \) -print0 \
        | xargs -0 -I{} sed -i.bak -E \
            -e 's/<!-- nift-built: [^>]*-->//g' \
            -e 's/[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:.+Z-]+//g' \
            "{}"
    find "$1" -name '*.bak' -delete
}
strip_volatiles "$V1_OUT"
strip_volatiles "$V2_OUT"

echo "==> diff -ur"
if ! diff -ur "$V1_OUT" "$V2_OUT" > "$WORK/diff.txt"; then
    echo "FAIL: output trees differ"
    head -200 "$WORK/diff.txt"
    exit 5
fi

echo "==> recursive sha256"
hash_tree() {
    ( cd "$1" && find . -type f -print0 | LC_ALL=C sort -z \
        | xargs -0 sha256sum | sha256sum | awk '{print $1}' )
}
H1=$(hash_tree "$V1_OUT")
H2=$(hash_tree "$V2_OUT")
echo "v1 tree hash: $H1"
echo "v2 tree hash: $H2"

if [[ "$H1" != "$H2" ]]; then
    echo "FAIL: tree hashes differ"
    exit 6
fi

# Parity beyond bytes: file count + total size.
F1=$(find "$V1_OUT" -type f | wc -l)
F2=$(find "$V2_OUT" -type f | wc -l)
S1=$(du -sb "$V1_OUT" | awk '{print $1}')
S2=$(du -sb "$V2_OUT" | awk '{print $1}')
echo "files: v1=$F1 v2=$F2  bytes: v1=$S1 v2=$S2"
[[ "$F1" -eq "$F2" ]] || { echo "FAIL: file count drift"; exit 7; }

echo "PASS: 100% parity"
