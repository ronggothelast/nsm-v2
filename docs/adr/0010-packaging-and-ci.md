# ADR 0010 — Packaging + CI release matrix

**Status:** Accepted
**Date:** 2026-05-31
**Phase:** 7 (Docs + packaging + signed release)

## Context

Phase 7 closes the loop: turn the working binary into something a user
can install in one command, with reproducible builds across platforms.

## Decision

### CI matrix

`.github/workflows/ci.yml` runs three jobs on every push:

1. **build-test** — Linux (gcc-11, clang-14), macOS (clang), Windows
   (MSVC 2022). Each runs `cmake --preset debug && cmake --build &&
   ctest`. vcpkg binary cache is keyed on the manifest + baseline so
   second runs skip the dep build.
2. **lint** — clang-format-14 dry-run on all source. CI fails on diff.
3. **sanitizers** — Ubuntu-only ASan + UBSan run of the full test suite.

A fourth job **release-binary** runs only on `v*` tags. It builds the
release preset on each platform and uploads tarballs as CI artifacts.

### Packaging targets

We ship binaries through:

- **Docker** — multi-stage `Dockerfile` at repo root. Build image is
  Ubuntu 22.04 + gcc-11 + vcpkg. Runtime image is Ubuntu 22.04 with
  just the binary + ca-certificates.
- **Homebrew** — formula in `packaging/homebrew/nift.rb`. Needs a tap
  to publish (out of repo scope).
- **AUR** — `PKGBUILD` in `packaging/aur/`. Pulls from `git`, runs the
  same release preset.
- **Debian** — control file stub in `packaging/debian/`. Full Debian
  packaging requires a sponsor and is deferred to Phase 8.
- **Plain tarballs** — CI release artifacts with SHA256.

### Signing

Sigstore detached signatures (`cosign sign-blob`) are deferred to
Phase 8 — they require a Sigstore-trusted CI identity and the workflow
to obtain a short-lived signing certificate. For Phase 7 we ship SHA256
checksums alongside tarballs; a future ADR will add Sigstore signing
once we have the OIDC plumbing.

## Consequences

✅ A user can `git clone && cmake --preset release && cmake --build`
   on any of Linux/macOS/Windows and get a working binary.
✅ A user can `docker run nift build /site` without installing
   anything besides Docker.
✅ Every commit is built and tested on three platforms, with lint and
   sanitizers gating the merge.
✅ Release artifacts are reproducible from a tag — no secret build steps.

⚠️ Sigstore signing is not yet wired up; SHA256 only.
⚠️ Debian packaging is a stub — the full DPKG pipeline (sponsor + dput)
   is out of scope for this phase.
⚠️ Tested matrix omits ARM — adding aarch64 runners is Phase 8.
