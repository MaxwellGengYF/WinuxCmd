# `scripts/build.py`

Convenience wrapper around `xmake` that auto-detects platform/arch/toolchain,
runs `xmake f` + `xmake build`, and optionally copies artifacts after build.

## Quick Start

```bash
# Release build, auto-detect everything
python scripts/build.py

# Debug build, clean config cache
python scripts/build.py -m debug -c

# Build + install artifacts to ./dist
python scripts/build.py --copy-to ./dist

# Full rebuild with 8 jobs
python scripts/build.py -r -j 8

# Build a single target
python scripts/build.py -t winuxcmd-tests
```

## Options

| Flag | Default | Description |
|------|---------|-------------|
| `-m`, `--mode` | `release` | Build mode: `release`, `debug`, `releasedbg` |
| `-p`, `--plat` | *auto* | Platform: `windows`, `linux`, `macosx` |
| `-a`, `--arch` | *auto* | Architecture: `x64` (Windows), `x86_64`/`arm64` (others) |
| `--toolchain` | *auto* | Toolchain name (e.g. `msvc`, `clang`, `gcc`, `clang-cl`, `llvm`) |
| `-c`, `--clean` | off | Clean `xmake` config cache before configuring |
| `-r`, `--rebuild` | off | Force rebuild (`xmake -r`) |
| `-j`, `--jobs` | *xmake default* | Parallel build jobs (e.g. `-j 8`) |
| `-v`, `--verbose` | off | Verbose `xmake` output |
| `-t`, `--target` | *all* | Build a specific target (e.g. `winuxcmd`, `winuxcmd-tests`) |
| `--root` | project root | Project root directory |
| `--copy-to` | — | Copy `winuxcmd.exe` + command links to this directory after build |
| `--copy-force` | off | Overwrite existing files when copying |
| `--copy` | off | Use file copies instead of hardlinks for command stubs |

## Auto-Detection

If `-p`, `-a`, or `--toolchain` are omitted, the script detects them:

### Platform
- Windows → `windows`
- macOS → `macosx`
- Anything else → `linux`

### Architecture
- Windows → `x64`
- `amd64`/`x86_64` → `x86_64`
- `arm64`/`aarch64` → `arm64`

### Toolchain
- **Windows**: tries `msvc` → `clang-cl` → `llvm` → `gcc` (first available)
- **macOS**: `clang`
- **Linux**: tries `gcc` → `clang` (first available)

Detection uses `xmake show --toolchains` to check availability.

## Copy Artifacts (`--copy-to`)

When `--copy-to ./dist` is specified, after a successful build the script:

1. Locates `winuxcmd.exe` under `build/` (mode-specific directory searched first)
2. Copies it into `./dist/`

This mirrors the logic in `scripts/copy.py`.

## What It Runs

Equivalent to these two `xmake` commands:

```bash
# Step 1: Configure
xmake f -p windows -a x64 --toolchain=msvc -m release [-c] [--verbose]

# Step 2: Build
xmake [-r] [--verbose] [-j N] [target]
```

## Comparison: `build.py` vs Raw `xmake`

| Scenario | `build.py` | Raw `xmake` |
|----------|-----------|-------------|
| Quick release build | `python scripts/build.py` | `xmake f -p windows -a x64 --toolchain=msvc -m release && xmake` |
| Auto-detect toolchain | ✔ built-in | Must pass `--toolchain=` explicitly |
| Copy artifacts after build | `--copy-to ./dist` | Run `scripts/copy.py` separately |
| Custom xmake options | Not supported | Full `xmake` flags available |

Use `build.py` for quick full-cycle builds and CI; use raw `xmake` for
fine-grained control or when you need extra `xmake f` options.
