# ADR-0018 — Image optimization: resize, WebP/AVIF, responsive srcset

**Status:** Accepted
**Date:** 2026-06-04
**Phase:** 8 (Polish & Advanced Features)
**Deciders:** lead architect agent

---

## Context

Modern web performance demands optimized images: correct dimensions for
each viewport, modern formats (WebP, AVIF) for smaller file sizes, and
responsive `srcset` for adaptive loading. SSGs that don't handle this
force users into manual workflows or third-party services.

Nift v2 needs a built-in image pipeline that:

1. Resizes images to configured breakpoints.
2. Converts to WebP and AVIF formats.
3. Generates `<img>` / `<picture>` HTML with `srcset` and `sizes`.
4. Caches processed images to avoid re-encoding on every build.

## Decision

**Implement an image optimization pipeline using `libvips` (via vips-cpp)
for resize and format conversion, with automatic srcset generation.**

### Pipeline stages

1. **Resize**: Generate multiple widths from the source image (configurable
   breakpoints, default: 320, 640, 960, 1280, 1920 px). Only widths
   smaller than the source are generated.
2. **Format conversion**: Each resized variant is encoded as:
   - Original format (JPEG/PNG) for fallback.
   - WebP (quality 80, lossy).
   - AVIF (quality 65, lossy) — if libvips was built with AVIF support.
3. **srcset generation**: Template helper `@imgsrc("path.jpg")` emits
   `<img>` or `<picture>` HTML with:
   - `srcset` listing all width variants.
   - `sizes` attribute (user-configurable or default `100vw`).
   - `<source>` elements for WebP and AVIF inside `<picture>`.
   - Fallback `<img src="...">` for legacy browsers.

### Configuration

```toml
[images]
breakpoints = [320, 640, 960, 1280, 1920]
webp_quality = 80
avif_quality = 65
avif_enabled = true  # false if libvips lacks AVIF support
```

### Caching

- Processed images cached in `.nift/cache/images/` keyed by:
  source path + source hash (BLAKE3) + target width + format.
- Unchanged source images skip reprocessing entirely.

### Library: libvips

- `libvips` via vcpkg (`vips` package).
- C++ binding: `vips-cpp` (ships with libvips).
- 10–20× faster than ImageMagick for batch resize operations.
- Memory-efficient: streaming pipeline, no full-image decode into RAM.

## Alternatives considered

- **ImageMagick (Magick++)** — ubiquitous but slow, memory-hungry, and
  has a history of security issues. Rejected for performance.
- **stb_image + stb_image_write** — header-only, no external deps, but
  no AVIF support, no streaming, and single-threaded. Too slow for
  large image sets.
- **Sharp (Node.js subprocess)** — adds Node.js dependency. libvips is
  the same engine underneath but without the JS overhead.
- **Cloudinary / imgix integration** — external service dependency,
  costs money, requires internet during build.
- **No image optimization** — rejected; Lighthouse penalizes unoptimized
  images and users expect it.

## Consequences

- **Positive:** Automatic responsive images — users drop in a full-res
  photo and get optimized variants for every viewport.
- **Positive:** WebP + AVIF typically 30–60 % smaller than JPEG at
  equivalent quality.
- **Positive:** libvips is the fastest open-source image processing
  library available.
- **Positive:** Cached results — second build of unchanged images is
  near-instant.
- **Negative:** libvips is a large dependency (~15 MB shared library).
  Acceptable for a build tool; not ideal for minimal installs.
- **Negative:** AVIF encoding is slow (~2–5 s per image). Mitigated by
  caching and optional disable via config.
- **Negative:** First build of a photo-heavy site can be slow.
  Documented expected build times.

## References

- libvips: https://www.libvips.org/
- ADR-0005 (BLAKE3 hashing) — used for image cache keys.
- ADR-0006 (work-stealing pool) — parallel image processing.
- WebP format: https://developers.google.com/speed/webp
- AVIF format: https://aomediacodec.github.io/av1-avif/
