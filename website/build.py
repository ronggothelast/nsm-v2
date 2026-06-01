#!/usr/bin/env python3
"""Render Nift v2 docs site pages with shared nav/footer.

Reads page-specific bodies from ./pages/*.body.html and wraps each in the
shell defined here. Re-run any time. Idempotent.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PAGES = ROOT / "pages"
OUT = ROOT

NAV = """
<nav class="nav" aria-label="Primary">
  <div class="nav-inner">
    <a class="brand" href="./index.html">
      <span class="brand-icon">N</span>
      Nift
      <span class="brand-version">v2.0.0-dev</span>
    </a>
    <button class="nav-toggle" aria-controls="primary-nav" aria-expanded="false">Menu</button>
    <div class="nav-links" id="primary-nav">
      <a href="./install.html">Install</a>
      <a href="./docs.html">Docs</a>
      <a href="./cli.html">CLI</a>
      <a href="./examples.html">Examples</a>
      <a href="./plugins.html">Plugins</a>
      <a href="./faq.html">FAQ</a>
      <span class="sep"></span>
      <a href="https://github.com/ronggothelast/nsm-v2" rel="noopener" target="_blank">GitHub ↗</a>
    </div>
  </div>
</nav>
""".strip()

FOOTER = """
<footer class="footer">
  <div class="container">
    <div class="footer-grid">
      <div>
        <a class="brand" href="./index.html"><span class="brand-icon">N</span> Nift <span class="brand-version">v2.0.0-dev</span></a>
        <p class="muted" style="max-width:32ch; margin-top:0.8rem; font-size:0.88rem; line-height:1.6;">A native C++20 static site framework. Open source. No telemetry. No tracking.</p>
      </div>
      <div>
        <h5>Get started</h5>
        <ul>
          <li><a href="./install.html">Install</a></li>
          <li><a href="./docs.html#quickstart">Quickstart</a></li>
          <li><a href="./examples.html">Examples</a></li>
          <li><a href="./docs.html#migration">Migration from v1</a></li>
        </ul>
      </div>
      <div>
        <h5>Reference</h5>
        <ul>
          <li><a href="./docs.html">Documentation</a></li>
          <li><a href="./cli.html">CLI Reference</a></li>
          <li><a href="./architecture.html">Architecture</a></li>
          <li><a href="./changelog.html">Changelog</a></li>
        </ul>
      </div>
      <div>
        <h5>Project</h5>
        <ul>
          <li><a href="https://github.com/ronggothelast/nsm-v2" rel="noopener" target="_blank">GitHub</a></li>
          <li><a href="https://github.com/ronggothelast/nsm-v2/issues" rel="noopener" target="_blank">Issues</a></li>
          <li><a href="./faq.html">FAQ</a></li>
          <li><a href="./plugins.html">Plugins</a></li>
        </ul>
      </div>
    </div>
    <div class="footer-bottom">
      <span>MIT licensed · Built with C++20</span>
      <span>Phase 7 · v2.0.0-dev</span>
    </div>
  </div>
</footer>
""".strip()

SHELL = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>{title}</title>
<meta name="description" content="{description}">
<meta property="og:type" content="website">
<meta property="og:title" content="{title}">
<meta property="og:description" content="{description}">
<meta property="og:url" content="https://ronggothelast.github.io/nsm-v2/{slug}">
<meta name="twitter:card" content="summary">
<meta name="theme-color" content="#050505">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="./assets/css/site.css">
</head>
<body>
<a class="skip-link" href="#main-content">Skip to content</a>
<div class="mesh-bg" aria-hidden="true"></div>
{nav}
<main id="main-content">
{body}
</main>
{footer}
<script src="./assets/js/site.js" defer></script>
</body>
</html>
""".lstrip()

# Page metadata
PAGES_META = {
    "index": ("Nift v2 — Native C++20 static site framework", "A modern C++20 static site generator with SIMD-scanned templates, sandboxed Lua plugins, work-stealing build pipeline, and incremental BLAKE3 cache.", "index.html"),
    "install": ("Install Nift v2", "Install Nift v2 on Linux, macOS, Windows, or build from source. Pre-built binaries and package managers supported.", "install.html"),
    "docs": ("Documentation — Nift v2", "Complete Nift v2 documentation: quickstart, configuration, template language, layouts, plugins, caching, and migration guide.", "docs.html"),
    "cli": ("CLI Reference — Nift v2", "Complete command-line reference for Nift v2: init, build, serve, clean, migrate, plugin, bench, and all global flags.", "cli.html"),
    "examples": ("Examples — Nift v2", "Code examples for Nift v2: minimal site, blog with tags, multi-language, custom plugins, and build pipeline configuration.", "examples.html"),
    "plugins": ("Plugin Development — Nift v2", "Build Nift v2 plugins with Lua (sandboxed) or native C ABI. Security model, lifecycle hooks, and API reference.", "plugins.html"),
    "faq": ("FAQ — Nift v2", "Frequently asked questions about Nift v2: performance, C++ choice, template compatibility, contributing, and more.", "faq.html"),
    "architecture": ("Architecture — Nift v2", "Technical architecture of Nift v2: crate dependency graph, parser pipeline, build executor, SIMD scanner, and plugin sandbox.", "architecture.html"),
    "changelog": ("Changelog — Nift v2", "Nift v2 release history: Phase 0 through Phase 7 milestones, breaking changes, and migration notes.", "changelog.html"),
    "404": ("Page not found — Nift v2", "The page you are looking for does not exist.", "404.html"),
}


def render(page_name: str) -> str:
    body_path = PAGES / f"{page_name}.body.html"
    if not body_path.exists():
        print(f"  SKIP  {page_name} (no body file)", file=sys.stderr)
        return ""
    body = body_path.read_text(encoding="utf-8")
    title, desc, slug = PAGES_META.get(page_name, (f"{page_name} — Nift v2", "", f"{page_name}.html"))
    return SHELL.format(title=title, description=desc, slug=slug, nav=NAV, footer=FOOTER, body=body)


def main() -> None:
    built = 0
    for body_file in sorted(PAGES.glob("*.body.html")):
        name = body_file.stem.replace(".body", "")
        html = render(name)
        if not html:
            continue
        out = OUT / f"{name}.html"
        out.write_text(html, encoding="utf-8")
        built += 1
        print(f"  {name}.html")
    print(f"\nBuilt {built} pages.")


if __name__ == "__main__":
    main()
