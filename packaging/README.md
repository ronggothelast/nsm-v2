# Packaging recipes for Nift v2.

This directory holds packaging stubs for distributing `nift` to common
platforms. Each subdirectory targets one ecosystem.

| Path | Target | Status |
|---|---|---|
| `homebrew/nift.rb` | macOS / Linux Homebrew | template; needs a tap repo |
| `aur/PKGBUILD` | Arch Linux AUR | template; needs AUR upload |
| `debian/control` | Debian / Ubuntu .deb | stub; full .deb packaging not yet implemented |

## CI release artifacts

`.github/workflows/release.yml` produces binaries when a `v*` tag is pushed:

- `nift-linux-x86_64.tar.gz`
- `nift-macos-arm64.tar.gz`
- `nift-windows-x86_64.zip`

GitHub release pages serve these. Homebrew/AUR can pull from the tag's tarball.

## Verifying a release

```bash
sha256sum -c nift-linux-x86_64.tar.gz.sha256
```

Sigstore signing is not yet implemented.
