# ADR-0015 — Markdown support: cmark-gfm with front matter and code highlighting

**Status:** Accepted
**Date:** 2026-06-04
**Phase:** 8 (Polish & Advanced Features)
**Deciders:** lead architect agent

---

## Context

A static site generator without Markdown support is non-competitive.
Every modern SSG (Hugo, Eleventy, Jekyll, Zola) treats Markdown as the
primary content format. Nift v2 needs:

1. GitHub-Flavored Markdown (tables, strikethrough, task lists,
   autolinks).
2. YAML and JSON front matter for per-page metadata.
3. Syntax-highlighted fenced code blocks.
4. High performance — Markdown rendering must not be the build
   bottleneck for 10 K+ page sites.

## Decision

**Use `cmark-gfm` (GitHub's C implementation of CommonMark + GFM
extensions) as the Markdown rendering engine, with a custom front matter
parser and compile-time code highlighting.**

### Rendering engine: cmark-gfm

- C library, available via vcpkg (`cmark-gfm`).
- Parses CommonMark + GFM extensions (tables, strikethrough, task lists,
  autolinks).
- Extremely fast: single-pass parser, no AST post-processing overhead.
- Renders directly to HTML string.

### Front matter

- YAML front matter delimited by `---` (triple dash) at file start.
- JSON front matter delimited by `{` / `}` at file start (auto-detected).
- Parsed with existing `yaml-cpp` (vcpkg) for YAML; `nlohmann/json` for
  JSON.
- Front matter keys become template variables (`@title`, `@date`, etc.).
- Front matter is stripped before Markdown parsing.

### Code highlighting

- Fenced code blocks with language tag (````lang`) trigger syntax
  highlighting.
- Implemented as a post-processing pass over cmark-gfm HTML output:
  `<pre><code class="language-X">` blocks are rewritten with `<span>`
  tags carrying CSS classes.
- Tokenizer is a lightweight regex-based scanner supporting ~20 common
  languages (C, C++, Python, JS, Rust, Go, Shell, SQL, HTML, CSS, etc.).
- No external runtime dependency (no highlight.js, no Pygments
  subprocess).

## Alternatives considered

- **md4c** — pure C, very fast, but no built-in GFM extensions. Would
  require hand-rolling table and task-list support.
- **maddy / mistletoe (Python subprocess)** — adds Python runtime
  dependency, unacceptable for a zero-dep SSG.
- **Embed highlight.js in output HTML** — client-side highlighting adds
  JS payload to every page and shifts work to the browser.
- **CommonMark-only (no GFM)** — users expect tables and task lists
  out of the box.

## Consequences

- **Positive:** GFM compatibility — users can copy-paste from GitHub
  READMEs.
- **Positive:** cmark-gfm is the fastest C Markdown parser available.
- **Positive:** Front matter integrates seamlessly with the template
  variable system.
- **Positive:** Server-side code highlighting means zero JS payload in
  output HTML.
- **Negative:** Regex-based syntax highlighter is less accurate than
  full parsers (tree-sitter, Pygments). Covers common cases well enough
  for blog content.
- **Negative:** Adding ~20 language tokenizers is maintenance work.
  Languages are opt-in and self-contained.

## References

- cmark-gfm: https://github.com/github/cmark-gfm
- CommonMark spec: https://spec.commonmark.org/
- GFM spec: https://github.github.com/gfm/
- `crates/markdown/` — Markdown crate implementation.
