---
name: xmake
---

# XMake Build System

Primary build system for this project.

## Requirements

- XMake 3.0.6+
- Optional: CUDA Toolkit, Vulkan SDK, LLVM 20, Rust

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

## Common Issues

### Build Flags

- `-v`, `-D`, `--diagnosis` are **invalid**; use `--verbose` for verbose output
- Boolean options: `--lc_option=true` or `=false`
- Always use `-c` to clean cache before reconfiguring
