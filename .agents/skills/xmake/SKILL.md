---
name: xmake
---

# XMake Build System

Primary build system for this project.

## Requirements

- XMake 3.0.6+

## Quick Start

```bash
xmake f -m debug -c && xmake build
```

Update `compile_commands.json`:
```bash
xmake project -k compile_commands --lsp=clangd .vscode
```

## Configuration

### Platform Examples

| Platform | Command |
|----------|---------|
| Linux GCC | `xmake f -p linux -a x86_64 --toolchain=gcc -m release -c` |
| Linux Clang | `xmake f -p linux -a x86_64 --toolchain=clang -m release -c` |
| Windows MSVC | `xmake f -p windows -a x64 --toolchain=msvc -m release -c` |
| Windows Clang-CL | `xmake f -p windows -a x64 --toolchain=clang-cl -m release -c` |
| Windows LLVM | `xmake f -p windows -a x64 --toolchain=llvm -m release -c` |

### Flags

| Flag | Description |
|------|-------------|
| `-c` | Clean configuration cache |
| `-m <mode>` | Build mode: `release`, `debug`, `releasedbg` |
| `-p <plat>` | Platform: `linux`, `windows`, `macosx` |
| `-a <arch>` | Architecture: `x86_64`, `x64`, `arm64` |
| `--check` | Check flags before building |
| `--<option>=<value>` | Set option defined in `xmake.lua` as `option()` function |


## Commands

| Command | Description |
|---------|-------------|
| `xmake clean` | Clean build files |
| `xmake -r` | Rebuild |
| `xmake run <target>` | Run target |
| `xmake -l` | List targets |
| `xmake install -o <dir>` | Install to directory |

## Targets

| Target | Kind | Condition | Description |
|--------|------|-----------|-------------|
| `winuxcmd-core` | static | — | Core library: utils, core, container, version, ffi, tools |
| `winuxcmd-commands` | static | — | Commands library, depends on `winuxcmd-core` |
| `winuxcmd` | binary | — | Main executable, depends on `winuxcmd-commands`, `winuxcmd-core` |
| `winuxcore` | shared | `build_ffi` | FFI shared library, depends on `winuxcmd-commands` |
| `test_main` | static | `enable_tests` | Test framework entry point |
| `winux-test-lib` | static | `enable_tests` | Test framework library (excludes `test_main.cpp`) |
| `winux-test` | binary | `enable_tests` | Test runner binary, depends on `winux-test-lib` |
| `winuxcmd-tests` | binary | `enable_tests` | Unit tests binary, depends on `winux-test-lib`, `test_main` |
| `container_module` | static | — | Container examples static library |
| `constexpr_map_example` | binary | — | Constexpr map example |
| `small_vector_example` | binary | — | Small vector example |
| `ffi_example` | binary | `build_ffi` | FFI example (C), depends on `winuxcore` |

## `scripts/build.py`

Convenience wrapper: see [`build_py_usage.md`](build_py_usage.md) for full usage.

```bash
python scripts/build.py                    # release, auto-detect
python scripts/build.py -m debug -c        # debug + clean cache
python scripts/build.py --copy-to ./dist   # build + copy artifacts
```

## Common Issues

### Build Flags

- `-v`, `-D`, `--diagnosis` are **invalid**; use `--verbose` for verbose output
- Boolean options: `--lc_option=true` or `=false`
- Always use `-c` to clean cache before reconfiguring
