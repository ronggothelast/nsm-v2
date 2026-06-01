# Nift v2 — Exhaustive QA Suite

Six adversarial test domains. Designed for CI matrix (Linux/macOS/Windows) plus
focused local runs.

| Domain | Driver | Entry point |
|--------|--------|-------------|
| 1. Legacy parity | bash + python | `parity/run_parity.sh` |
| 2. Filesystem chaos | python | `chaos/test_chaos.py` |
| 3. Lua sandbox | C++ + Catch2 | `security/test_lua_sandbox.cpp` |
| 4. Signal recovery | python | `recovery/test_signal_recovery.py` |
| 5. Fuzzing | libFuzzer/AFL++ | `fuzzing/fuzz_*.cpp` |
| 6. Server traversal & stress | python | `network/test_server_security.py` |

Run all domains locally:

```sh
just qa            # umbrella recipe
just qa-parity     # individual
just qa-chaos
just qa-security
just qa-recovery
just qa-fuzz       # short run, 60s per target
just qa-network
```

CI: `.github/workflows/qa.yml` runs domains 2,3,4,6 on every PR and the full
matrix (incl. fuzzing + parity) nightly.

All scripts MUST exit non-zero on failure. No `|| true`. No skipped asserts.
