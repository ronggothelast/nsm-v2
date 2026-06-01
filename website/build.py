#!/usr/bin/env python3
"""Render Nift v2 docs site pages with a shared nav/footer.

Reads page-specific bodies from ./pages/*.body.html and wraps each in the
shell defined here. Re-run any time. Idempotent.
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PAGES = ROOT / "pages"
OUT = ROOT

NAV = """
<header class="nav">
  <div class="nav-inner">
    <a class="brand" href="./index.html">
      <span class="brand-mark"></span>
      Nift
      <span class="brand-version">v2.0.0-dev</span>
    </a>
    <button class="nav-toggle" aria-controls="primary-nav" aria-expanded="false">Menu</button>
    <nav class="nav-links" id="primary-nav">
      <a href="./install.html">Install</a>
      <a href="./docs.html">Docs</a>
      <a href="./cli.html">CLI</a>
      <a href="./examples.html">Examples</a>
      <a href="./plugins.html">Plugins</a>
      <a href="./faq.html">FAQ</a>
      <a href="https://github.com/ronggothelast/nsm-v2" rel="noopener">GitHub</a>
    </nav>
  </div>
</header>
""".strip()

FOOTER = """
<footer class="footer">
  <div class="container">
    <div class="footer-grid">
      <div>
        <a class="brand" href="./index.html"><span class="brand-mark"></span> Nift <span class="brand-version">v2.0.0-dev</span></a>
        <p class="muted" style="max-width:32ch; margin-top:0.8rem; font-size:0.9rem;">A native C++20 static site framework. Open source. No telemetry. No tracking.</p>
      </div>
      <div>
        <h5>Get started</h5>
        <ul>
          <li><a href="./install.html">Install</a></li>
          <li><a href="./docs.html#quickstart">Quickstart</a></li>
          <li><a href="./examples.html">Examples</a></li>
        </ul>
      </div>
      <div>
        <h5>Reference</h5>
        <ul>
          <li><a href="./docs.html">Docs</a></li>
          <li><a href="./cli.html">CLI</a></li>
          <li><a href="./architecture.html">Architecture</a></li>
          <li><a href="./changelog.html">Changelog</a></li>
        </ul>
      </div>
      <div>
        <h5>Project</h5>
        <ul>
          <li><a href="https://github.com/ronggothelast/nsm-v2" rel="noopener">GitHub</a></li>
          <li><a href="https://github.com/ronggothelast/nsm-v2/issues" rel="noopener">Issues</a></li>
          <li><a href="./faq.html">FAQ</a></li>
          <li><a href="./plugins.html">Plugins</a></li>
        </ul>
      </div>
    </div>
    <div class="footer-bottom">
      <span>MIT licensed.</span>
      <span>Phase 7 · v2.0.0-dev · commit 775cd54</span>
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
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="./assets/css/site.css">
</head>
<body>
{nav}
<main>
{body}
</main>
{footer}
<script src="./assets/js/site.js" defer></script>
</body>
</html>
"""


def render_page(path: Path) -> str:
    raw = path.read_text(encoding="utf-8")
    # Front matter: simple `<!-- key: value -->` lines at top.
    meta = {"title": path.stem, "description": ""}
    body_lines: list[str] = []
    for line in raw.splitlines():
        if line.strip().startswith("<!--") and line.strip().endswith("-->") and ":" in line:
            inner = line.strip().removeprefix("<!--").removesuffix("-->").strip()
            k, _, v = inner.partition(":")
            meta[k.strip()] = v.strip()
        else:
            body_lines.append(line)
    body = "\n".join(body_lines).strip()
    return SHELL.format(
        title=meta.get("title", path.stem),
        description=meta.get("description", ""),
        nav=NAV,
        body=body,
        footer=FOOTER,
    )


def main() -> int:
    if not PAGES.exists():
        print(f"FAIL: no pages directory at {PAGES}", file=sys.stderr)
        return 1
    sources = sorted(PAGES.glob("*.body.html"))
    if not sources:
        print(f"FAIL: no *.body.html files in {PAGES}", file=sys.stderr)
        return 1
    for src in sources:
        name = src.name.removesuffix(".body.html") + ".html"
        out = OUT / name
        out.write_text(render_page(src), encoding="utf-8")
        print(f"  wrote {out.relative_to(ROOT)}")
    print(f"OK: {len(sources)} pages rendered")
    return 0


if __name__ == "__main__":
    sys.exit(main())
