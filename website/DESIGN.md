# Design System: Nift v2 Documentation Site

## 1. Visual Theme & Atmosphere
A clinical-warm developer documentation surface. The mood is "engineering
notebook in a daylit studio": precise, slightly bookish, never cold. Density
sits at 4 (Daily App Balanced) — enough whitespace to read code without
fatigue, dense enough to feel like real reference material. Variance is 6
(Offset Asymmetric) — split-screen heroes, sidebar+content layouts, no
centered marketing-page kitsch. Motion is 4 (Fluid CSS) — spring-driven menu
and copy-button feedback, plus a perpetual two-second pulse on the live CLI
prompt cursor. Nothing decorative animates.

## 2. Color Palette & Roles
- **Warm Canvas** (#FAFAF7) — primary page background; off-white with the
  faintest warm cast so it never reads as clinical hospital-white
- **Pure Surface** (#FFFFFF) — card fill, code-block background interior
- **Charcoal Ink** (#0F0F11) — primary text, headings; never #000000
- **Muted Steel** (#52525B) — secondary text, metadata, captions
- **Whisper Border** (rgba(15, 15, 17, 0.08)) — 1px structural lines, card edges
- **Burnt Umber** (#B45309) — single accent for links, primary CTAs, focus
  rings, active nav state. Saturation 73% — well under the 80% ceiling.
  Distinctly NOT purple, NOT neon, NOT cyan.
- **Code Slate** (#1C1C1F) — inline code background, dark code-block fills
- **Mint Tag** (#15803D) — success/PASS badges only
- **Rust Tag** (#9F1239) — error/FAIL badges only

(Max 1 accent for interactive elements. The Mint/Rust tags are functional
status indicators and never used decoratively.)

## 3. Typography Rules
- **Display:** `Geist` — weights 500/600. Track-tight (-0.02em) at large
  sizes. Hierarchy through weight + color, never through size alone
- **Body:** `Geist` 400 — line-height 1.65, max 65ch line length, color
  Muted Steel for secondary, Charcoal Ink for primary
- **Mono:** `Geist Mono` — code, CLI examples, version strings, file paths
- **Banned:** Inter, generic system fonts, all generic serifs (Times,
  Georgia, Garamond). No serif anywhere on this site — it is a developer
  docs surface
- **Scale:** clamp-driven; h1 clamp(2rem, 4vw, 3.25rem), h2 clamp(1.5rem,
  2.5vw, 2rem), body 1rem (never below)

## 4. Component Stylings
- **Buttons:** Flat fill, NO outer glow. Primary: Burnt Umber on Charcoal
  Ink text-light; secondary: ghost (transparent + 1px Whisper Border).
  Active state: -1px translateY for tactile press feedback. Hover: shift
  background by 6% darker, never by changing hue.
- **Cards:** Used only for plugin tiles and example showcases. 12px radius
  (NOT 2.5rem — that's marketing-deck territory; this is docs). Single 1px
  Whisper Border, no shadow at rest, hover lifts shadow to
  `0 8px 24px rgba(15,15,17,0.06)`. Card grid is 2-col asymmetric, not
  3-col equal.
- **Code blocks:** Code Slate fill, Geist Mono text, copy button anchored
  top-right, language tag bottom-right in muted steel. Inline code: same
  fill but 4px radius, 0.875em size.
- **Inputs (search):** Label hidden visually but available to screen readers,
  placeholder text in Muted Steel, focus ring in Burnt Umber.
- **Nav:** Sticky top bar, Whisper Border bottom, transparent fill that
  becomes Pure Surface with backdrop-blur once scrolled past 12px.
- **Sidebar (docs):** Sticky on >=1024px, simple text list with Burnt Umber
  active state and Whisper Border left-edge indicator on the active item.
  No fancy expand/collapse animations.
- **Loaders:** None. The site is fully static.
- **Empty states:** N/A — every page has content.

## 5. Layout Principles
- CSS Grid for all multi-column layouts. No `calc()` percentage hacks.
- Max-width 1280px for content; 1440px for hero. Centered via
  `margin-inline: auto`.
- Hero: split 60/40 (text left, terminal demo right). NEVER centered.
- Docs page: 240px sidebar + 1fr content + optional 200px on-page-toc on
  >=1280px. Single column on <1024px.
- Section vertical rhythm: `clamp(4rem, 8vw, 7rem)` between major sections.
- Use `min-h-[100dvh]` for full-height hero, never `100vh`.
- No overlapping elements. No absolute-positioned text overlays.

## 6. Motion & Interaction
- Default easing: cubic-bezier(0.32, 0.72, 0, 1) — spring-flavored without
  JS. Duration 220ms for most interactions, 320ms for nav transitions.
- Perpetual micro-loop: a single 2s `cursorBlink` opacity pulse on the
  hero terminal's `_` cursor. Nothing else loops.
- Copy-to-clipboard: button shifts text from "Copy" to "Copied" with a
  220ms cross-fade. No checkmark icon — text suffices.
- Mobile menu: slide-in from right with the spring easing, 280ms.
- Animate exclusively transform + opacity. Never width/height/top/left.
- `@media (prefers-reduced-motion: reduce)` strips all transitions.

## 7. Anti-Patterns (Banned)
- No emojis in body content (allowed only in user-quoted CLI output)
- No `Inter` font, no system-ui fallback for headings
- No serif fonts anywhere
- No pure black `#000000`
- No purple, no cyan, no neon, no gradient text on headlines
- No drop-shadow on text, no text-stroke
- No outer-glow shadows on any element
- No 3-column equal-card "feature" grids — use asymmetric 2-col or list
- No centered hero (variance is 6, splits required)
- No "Scroll to explore", no chevron arrows, no bouncing icons
- No fabricated metrics ("99.99% uptime", "10x faster") — only numbers we
  measured: `271/271 tests`, `2.4× SIMD speedup` from Phase 4 bench
- No fake testimonials, no placeholder John Doe names
- No `LABEL // YEAR` or `SECTION_01` brutalist-LARP labels
- No `Acme`, `Lorem`, broken Unsplash links
- No light/dark theme toggle in v1 — light only, well-executed, beats
  half-baked dark theme
- No custom mouse cursors
