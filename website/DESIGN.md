# Nift v2 Documentation Site — Design Contract

## Vibe Archetype: Ethereal Glass (SaaS / Tech)

Deep OLED black (#050505), radial mesh gradients, vantablack cards with
heavy backdrop-blur, pure white/10 hairlines. Geist typography.

## Typography

- **Primary:** Geist (400, 500, 600)
- **Mono:** Geist Mono (400, 500)
- **Banned:** Inter, Roboto, Arial, Open Sans, Helvetica, system-ui
- H1: clamp(2.5rem, 5vw, 4rem), line-height 1.08, letter-spacing -0.035em
- H2: clamp(1.5rem, 2.5vw, 2rem), line-height 1.12, letter-spacing -0.025em
- H3: clamp(1.125rem, 1.6vw, 1.35rem), line-height 1.2, letter-spacing -0.015em
- Body: 1rem, line-height 1.7
- Code: 0.875rem, line-height 1.55

## Layout

- Asymmetric 6-col grid (never 3-equal-cards)
- Feature pattern: span 6 / span 3+3 / span 4+2 / span 6
- Hero: split-screen (left: text, right: terminal mock)
- Docs: sticky sidebar + scrollable content
- Section padding: clamp(3rem, 6vw, 5rem)
- Max content: 1280px, hero: 1440px, prose: 65ch

## Colors

- --bg-deep: #050505 (OLED black)
- --bg-card: #0A0A0C (vantablack card)
- --bg-elevated: #111114 (elevated surface)
- --ink: #EAEAEC (primary text)
- --muted: #71717A (secondary text)
- --accent: #A78BFA (violet accent)
- --accent-dim: rgba(167, 139, 250, 0.15)
- --border: rgba(255, 255, 255, 0.06)
- --border-hover: rgba(255, 255, 255, 0.12)
- --glass: rgba(10, 10, 12, 0.7) + backdrop-blur(20px)
- --mint: #34D399 (success/ok)
- --rust: #F87171 (error/warn)

## Motion

- Easing: cubic-bezier(0.32, 0.72, 0, 1) — ALL transitions
- Duration fast: 200ms, base: 350ms, slow: 600ms
- Scroll entry: translate-y-8 → 0, opacity 0 → 1, 600ms
- Hover: scale(1.02) on cards, scale(0.98) on buttons active
- Stagger: 80ms between sibling reveals
- prefers-reduced-motion: disable all transforms, keep opacity only

## Components

### Double-Bezel Cards
- Outer: bg-[rgba(255,255,255,0.03)], ring-1 ring-white/[0.06], rounded-2xl, p-1.5
- Inner: bg-[#0A0A0C], rounded-[calc(2rem-6px)], shadow-inner highlight
- Effect: physical machined hardware feel

### Floating Glass Nav
- position: sticky, top: 16px, mx-auto, w-max, rounded-full
- bg: rgba(10,10,12,0.6), backdrop-blur(20px), border: 1px solid rgba(255,255,255,0.06)
- On scroll: bg opacity increases to 0.85

### Button-in-Button CTA
- Pill shape: rounded-full, px-8 py-4
- Trailing arrow in nested circle: w-10 h-10 rounded-full bg-white/10
- On hover: arrow translates x+1 y-1, scale 1.05

### Code Blocks
- bg: #0D0D10, border: 1px solid rgba(255,255,255,0.04)
- Copy button: absolute top-right, glass style
- Syntax: muted colors that don't clash with violet accent

## Mobile (< 768px)

- Single column, px-4
- Hero stacks vertically
- Nav collapses to hamburger with modal overlay
- Feature grid: single column
- Docs sidebar: hidden (toggle on top)
- No h-screen, use min-h-[100dvh]

## Accessibility

- Skip-to-content link
- Semantic landmarks: nav, main, aside, article, section
- Focus-visible: 2px solid accent ring
- prefers-reduced-motion support
- Contrast ratio: ≥ 4.5:1 on all text
