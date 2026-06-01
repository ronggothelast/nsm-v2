# Nift v2 — Documentation Website

Static documentation site for Nift v2. Hand-built HTML + a single CSS file
plus a tiny progressive-enhancement script. No frameworks.

## Pages

| File | Purpose |
|------|---------|
| `index.html` | Home — hero, feature grid, migration teaser, bench table |
| `install.html` | Install via source, Homebrew, AUR, Debian, Docker |
| `docs.html` | Quickstart, project layout, config, syntax, build, serve, plugins |
| `cli.html` | Command reference for every `nift` subcommand |
| `examples.html` | Reference projects: blog, docs, marketing, portfolio |
| `plugins.html` | Lua plugins + native C-ABI plugins guide |
| `architecture.html` | Crate-level architecture and pipeline diagram |
| `faq.html` | Twelve common questions, collapsible |
| `changelog.html` | Phase-by-phase release history |
| `404.html` | Not-found page |

## Design

See [`DESIGN.md`](./DESIGN.md) for the full design contract — palette,
typography, motion, spacing, layout rules.

## Build

`index.html` is hand-written. Every other page is a body fragment in
`pages/*.body.html` wrapped by the shared shell in `build.py`.

```bash
python3 build.py        # rewrites all sub-pages from pages/*.body.html
python3 -m http.server  # local preview on http://127.0.0.1:8000
```

The build is idempotent: re-running on an unchanged tree produces no diff.

## Deploy

The output is fully static. Drop the `website/` directory contents on any
HTTP server (or GitHub Pages / Cloudflare Pages / S3). No build step beyond
`python3 build.py`.
