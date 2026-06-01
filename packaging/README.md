# Packaging recipes for nift v2.

This directory holds packaging stubs for distributing `nift` to common
platforms. Each subdirectory targets one ecosystem.

| Path | Target | Status |
|---|---|---|
| `homebrew/nift.rb` | macOS / Linux Homebrew | usable; needs a tap |
| `aur/PKGBUILD` | Arch Linux AUR | usable; needs AUR upload |
| `debian/control` | Debian / Ubuntu .deb | stub; full packaging in Phase 8+ |
| `../Dockerfile` | Docker (root of repo) | usable: `docker build -t nift .` |

## CI release artifacts

`.github/workflows/ci.yml` produces signed binaries when a `v*` tag is
pushed:

- `nift-linux-x86_64.tar.gz`
- `nift-macos-x86_64.tar.gz`
- `nift-windows-x86_64.tar.gz`

GitHub release pages serve these. Homebrew/AUR can pull from the tag's
tarball.

## Verifying a release

Each release tarball ships with a `.sha256` and (Phase 8+) a Sigstore
detached signature. Verify before installing:

```bash
sha256sum -c nift-linux-x86_64.tar.gz.sha256
```
