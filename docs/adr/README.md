# Architecture Decision Records

Register of all ADRs for Nift v2. Newest-first within each status group.

## Accepted

| # | Title | Phase |
|---:|---|---|
| [0019](0019-plugin-structured-args.md) | Plugin ABI v2: structured key-value arguments | 8 |
| [0018](0018-image-optimization.md) | Image optimization: resize, WebP/AVIF, responsive srcset | 8 |
| [0017](0017-asset-pipeline.md) | Asset pipeline: lightningcss, esbuild, BLAKE3 fingerprinting | 8 |
| [0016](0016-tailwind-integration.md) | Tailwind CSS integration via subprocess | 8 |
| [0015](0015-markdown-support.md) | Markdown support: cmark-gfm, front matter, code highlighting | 8 |
| [0014](0014-template-inheritance.md) | Template inheritance: @extends / @section / @yield / @parent | 8 |
| [0013](0013-native-filesystem-watcher.md) | Native filesystem watcher: inotify + polling fallback | 8 |
| [0012](0012-cbor-cache-format.md) | Build cache: dual CBOR + JSON format | 8 |
| [0011](0011-sse-livereload.md) | SSE livereload replacing polling | 8 |
| [0010](0010-packaging-and-ci.md) | Packaging + CI release matrix | 7 |
| [0009](0009-cli-plugin-migrator.md) | CLI design + plugin C ABI + v1 migrator | 6 |
| [0008](0008-dev-server-cpp-httplib.md) | Dev server: cpp-httplib + polling watcher | 5 |
| [0007](0007-mmap-and-simd-scanning.md) | mmap I/O + xsimd scanning | 4 |
| [0006](0006-work-stealing-pool.md) | Work-stealing thread pool | 4 |
| [0005](0005-build-cache-blake3-json.md) | Build cache: BLAKE3 + JSON (CBOR deferred) | 3 |
| [0004](0004-lua-bridge-sol2.md) | Lua bridge: sol2 | 3 |
| [0003](0003-cpp-standard-baseline.md) | C++20 baseline + C++23 feature detection | 1 |
| [0002](0002-dependency-manager-vcpkg.md) | Dependency manager: vcpkg manifest mode | 1 |
| [0001](0001-build-system-cmake-presets.md) | Build system: CMake with Presets + Ninja + just | 1 |
| [0000](0000-process.md) | ADR process & template | 0 |

## Proposed

_(none yet)_

## Superseded

_(none yet)_

## Deprecated

_(none yet)_

---

See [`0000-process.md`](0000-process.md) for the ADR template and when to write one.
