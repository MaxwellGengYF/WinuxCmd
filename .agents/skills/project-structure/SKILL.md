---
name: project-structure
description: WinuxCmd project structure. Targets, source directories, and dependencies. Use when adding/renaming targets, sources, or modifying xmake.lua.
---

# Project Structure

## Directory Layout

```
src/
├── Main/           # main(), readline, native_completion, command_refs
├── commands/       # ~140 Unix-like commands (ls, cat, grep, find, sed, ...)
├── core/           # cmd_meta, command_context, dispatcher, opt, pipeline
├── utils/          # console, encoding, file_io, json, path, text_io, utf8, wildcard, cppbar
├── container/      # constexpr_map, container, small_vector
├── version/        # version.cpp, version.hpp.in
├── ffi/            # ffi.cpp (C ABI exports)
├── tools/          # scaffold.cpp, wpm.cpp
└── pch/pch.h       # precompiled header
tests/
├── framework/      # test_main.cpp + test infrastructure
└── unit/           # unit tests for commands/core/utils
examples/
├── container/      # constexpr_map_example, small_vector_example
└── ffi/            # ffi_example.c
scripts/
└── xmake_func.lua  # lc_basic_settings rule
```

## Targets & Dependencies

### Always built
| Target | Kind | Sources | Deps |
|--------|------|---------|------|
| `winuxcmd-core` | static lib | `src/utils/*.cpp`, `src/core/*.cpp`, `src/container/*.cpp`, `src/version/*.cpp`, `src/ffi/*.cpp`, `src/tools/*.cpp` | — |
| `winuxcmd-commands` | static lib | `src/commands/*.cpp` | `winuxcmd-core` |
| `winuxcmd` | binary | `src/Main/*.cpp` | `winuxcmd-commands`, `winuxcmd-core` |
| `container_module` | static lib | `src/container/constexpr_map.cpp`, `src/container/container.cpp`, `src/container/small_vector.cpp` | — |
| `constexpr_map_example` | binary | `examples/container/constexpr_map_example.cpp` | `container_module` |
| `small_vector_example` | binary | `examples/container/small_vector_example.cpp` | `container_module` |

### Optional: `build_ffi=true`
| Target | Kind | Sources | Deps |
|--------|------|---------|------|
| `winuxcore` | shared lib | `src/ffi/ffi.cpp` | `winuxcmd-commands` |
| `ffi_example` | binary | `examples/ffi/ffi_example.c` | `winuxcore` |

### Optional: `enable_tests=true`
| Target | Kind | Sources | Deps |
|--------|------|---------|------|
| `test_main` | static lib | `tests/framework/test_main.cpp` | — |
| `winux-test-lib` | static lib | `tests/framework/*.cpp` (excl. test_main.cpp) | — |
| `winux-test` | binary | `tests/framework/test_main.cpp` | `winux-test-lib` |
| `winuxcmd-tests` | binary | `tests/unit/**/*.cpp` | `winux-test-lib`, `test_main` |

## Dependency Graph

```
winuxcmd-core
├── winuxcmd-commands
│   ├── winuxcmd
│   └── winuxcore (opt: build_ffi)
│       └── ffi_example (opt: build_ffi)
container_module
├── constexpr_map_example
└── small_vector_example

test_main ─┐
           ├── winuxcmd-tests (opt: enable_tests)
winux-test-lib ─┘
└── winux-test (opt: enable_tests)
```

## Key Conventions

- All targets use rule `lc_basic_settings` (defined in `scripts/xmake_func.lua`).
- PCH: `src/pch/pch.h` via `winux_cmd_set_pcxxheader()` / `on_load`.
- Include paths: `src`, `$(builddir)/generated`.
- Generated headers: `src/version/version.hpp.in` → `$(builddir)/generated/version.hpp`.
- Windows syslinks for `winuxcmd`: user32, shell32, advapi32, ole32, oleaut32.
