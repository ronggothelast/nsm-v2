# 🐰⚡🦡 nsm-v2 — Nift Revolution

> Work-in-progress modernization of [`nifty-site-manager/nsm`](https://github.com/nifty-site-manager/nsm) — C++23 rewrite, modular architecture, modern build system, full asset pipeline.

**Status:** 🟡 Bootstrap — Phase 0 (recon) belum dimulai.

---

## 📁 Layout

```
nsm-v2/
├── legacy/              # Snapshot Nift v1 (read-only reference, jangan diedit)
├── crates/              # [coming] modular C++23 crates
├── apps/                # [coming] CLI binary
├── third_party/         # [coming] pinned deps via vcpkg
├── tests/               # [coming] unit + integration + fuzz + bench
├── docs/                # [coming] mkdocs-material
├── packaging/           # [coming] deb/rpm/brew/choco/etc
├── .github/workflows/   # [coming] CI matrix
├── CMakeLists.txt       # [coming]
└── README.md
```

## 🎯 Goal

Reborn Nift untuk 2026+:
- Tetap C++/C (C++23 core, C17 hot-path)
- 2-5× lebih cepat dari Hugo
- Modular (no more 456KB `Parser.cpp`)
- Modern build (CMake presets + vcpkg)
- Full asset pipeline (Tailwind v4, libvips, esbuild)
- HMR dev server, WASM playground, plugin sandbox
- Backwards-compat via `nift migrate`

Full spec ada di lokal (lihat `/root/nift-revolution-spec/AGENT_PROMPT.md`) — bukan di-commit ke repo ini sengaja, dipakai sebagai brief untuk agent saja.

## 🔗 Upstream

- Original: https://github.com/nifty-site-manager/nsm
- Author: Nicholas Ham (MIT, 2015-present)
- Website: https://nift.dev

## 📜 License

MIT (inherits from upstream).
