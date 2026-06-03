# ADR-0014 — Template inheritance: @extends / @section / @yield / @parent

**Status:** Accepted
**Date:** 2026-06-04
**Phase:** 8 (Polish & Advanced Features)
**Deciders:** lead architect agent

---

## Context

Nift v2's template system supports variable substitution and conditionals
but lacks layout reuse. Every page must either duplicate boilerplate
(header, footer, nav) or use `@include` to paste fragments — neither
approach provides the "define once, override per page" workflow that
modern SSG users expect (cf. Jinja2, Blade, Nunjucks).

## Decision

**Add template inheritance via four new directives: `@extends`,
`@section`, `@yield`, and `@parent`.**

### Syntax

- `@extends("path")` — declares the parent layout. Must be the first
  directive in the child template.
- `@section("name") { ... }` — defines a named content block. Content
  between braces is captured and stored.
- `@section("name") ... @endsection` — alternative brace-free syntax.
- `@yield("name")` — renders the named section's content. Used in
  parent (layout) templates.
- `@parent` — inside a child `@section`, inserts the parent layout's
  section content (append/prepend pattern).

### Evaluation model

1. Parse child template: extract `@extends` path and `@section` blocks.
2. Parse layout template: it is a regular Nift template.
3. Section content stored as `__section_<name>` internal variables.
4. Layout renders normally; `@yield("name")` reads and outputs the
   corresponding section variable.

### Constraints

- Single-level inheritance only (no nested `@extends` chains).
- Section names must be unique within a template.
- `@parent` is a no-op if the parent layout has no corresponding section.

## Alternatives considered

- **Full Jinja2/Handlebars-style inheritance with blocks and super()** —
  richer but significantly more parser complexity. Single-level covers
  95 % of real SSG layouts.
- **Partial/component includes only** — what v1 already has. Rejected
  because it forces copy-paste of layout structure.
- **WASM-based template engine (e.g., embedded Liquid)** — powerful but
  adds runtime weight and a second template language. Nift's own
  template syntax is the differentiator.

## Consequences

- **Positive:** DRY layouts — define once, override per page.
- **Positive:** Familiar syntax for users coming from Laravel Blade or
  Jinja2.
- **Positive:** Integrates cleanly with existing template variable
  system (sections become internal variables).
- **Negative:** Single-level limit may frustrate power users who want
  multi-level layout chains (e.g., base → blog → post).
- **Negative:** `@parent` is currently a no-op; documented as a known
  limitation for future enhancement.

## References

- `docs/template-inheritance.md` — full usage guide with examples.
- `crates/core/template_inheritance.cpp` — parser and evaluator.
