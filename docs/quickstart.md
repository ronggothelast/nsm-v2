# Quickstart

Get a Nift v2 site running in under 60 seconds.

## Install

### From source (recommended for now)

```bash
git clone --recurse-submodules https://github.com/ronggothelast/nsm-v2.git
cd nsm-v2
cmake --preset release
cmake --build --preset release
sudo cp build/release/apps/nift/nift /usr/local/bin/
```

### From a release tarball (Phase 7+)

Download `nift-<platform>.tar.gz` from
[Releases](https://github.com/ronggothelast/nsm-v2/releases), extract,
copy `nift` somewhere on your `$PATH`.

## Create a project

```bash
nift init my-blog
cd my-blog
```

This scaffolds:

```
my-blog/
├── nift.json          # Project config
├── content/           # Source files (HTML / MD / templated)
│   └── index.html
├── templates/         # Reusable templates
└── public/            # Build output (gitignore me)
```

## Build

```bash
nift build
# Built 1 · cached 0 · failed 0 in 0 ms
```

Output lands in `public/`. Re-runs use the BLAKE3 incremental cache —
unchanged sources are skipped.

## Develop

```bash
nift serve
# Serving public/ at http://127.0.0.1:8080
# Watching content/ for changes
```

Edits to `content/` trigger a rebuild and a livereload bump. Open the
URL in your browser; the page reloads automatically.

## Migrate from Nift v1

If you have a `nsm` project (v1):

```bash
nift migrate /path/to/old-project
```

The migrator reads your `siteInfo/nsm.config`, writes a `nift.json`
with translated keys, and copies `tracked.json` over. Your v1 files
are untouched — revert by deleting `nift.json`.

## Common commands

| Command | Purpose |
|---|---|
| `nift init [path]` | Scaffold a new project |
| `nift build` | Build once |
| `nift serve` | Build + dev server with hot reload |
| `nift migrate` | v1 → v2 |
| `nift clean` | Remove `public/` and `.nift/cache/` |
| `nift version` | Print version |
| `nift help` | Full help |

## Useful flags

- `--quiet` — suppress non-error output
- `--no-cache` — disable incremental build
- `--threads N` — set worker pool size
- `--port 3000` — dev server port
- `--no-watch` — disable file watcher in `serve`

## Next steps

- [Architecture](architecture.md) — how the build pipeline is wired
- [Plugin author guide](plugin-author.md) — add custom directives
- [Migration guide](migration.md) — moving from v1
