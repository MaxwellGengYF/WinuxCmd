---
name: test
description: WinuxCmd test guide. C++ unit tests (xmake/wctest) and Python integration tests (pytest). Use when writing, running, or debugging tests.
---

# Test Guide

Two test suites: C++ unit tests (xmake + wctest) and Python integration tests (pytest).

## Quick Reference

| Suite | Build | Run All | Run Filtered |
|-------|-------|---------|--------------|
| C++ | `xmake f -m debug -c && xmake build winuxcmd-tests` | `xmake run winuxcmd-tests` | `xmake run winuxcmd-tests -- --gtest_filter=echo.*` |
| Python | (none) | `pytest tests/test_bash.py -v` | `pytest tests/test_bash.py -k "test_ls"` |

---

## C++ Unit Tests (`wctest`)

### Build

```bash
xmake f -m debug --enable_tests=true -c
xmake build winuxcmd-tests
```

Target `winuxcmd-tests` compiles from `tests/unit/**/*.cpp`, depends on `winux-test-lib` + `test_main`.

### Run

```bash
# All tests
xmake run winuxcmd-tests

# By filter (supports * and ? wildcards)
xmake run winuxcmd-tests -- --gtest_filter=cat.*
xmake run winuxcmd-tests -- --gtest_filter=*.basic_*

# Exclude pattern
xmake run winuxcmd-tests -- --gtest_filter=cat.*-cat.solo

# Single test
xmake run winuxcmd-tests -- --run-test echo.echo_basic

# Whole group
xmake run winuxcmd-tests -- --run-test echo

# List all tests
xmake run winuxcmd-tests -- --list-tests
```

### Write a Test

File: `tests/unit/<command>/<command>_unit_test.cpp`

```cpp
#include "framework/winuxtest.h"

TEST(group, test_name) {
  Pipeline p;
  p.add(L"command.exe", {L"arg1", L"arg2"});

  auto r = p.run();

  EXPECT_EQ_TEXT(r.stdout_text, "expected output\n");
}
```

### Assertions

| Macro | Usage |
|-------|-------|
| `EXPECT_TRUE(x)` | Boolean true |
| `EXPECT_FALSE(x)` | Boolean false |
| `EXPECT_EQ(a, b)` | Equality (any streamable type) |
| `EXPECT_NE(a, b)` | Inequality |
| `EXPECT_LT(a, b)` | Less than |
| `EXPECT_GT(a, b)` | Greater than |
| `EXPECT_EQ_TEXT(a, b)` | String equality (normalizes newlines) |
| `EXPECT_BYTES(a, b)` | Binary data equality |
| `EXPECT_EXIT_CODE(r, code)` | Process exit code |

### Logging

| Macro | Usage |
|-------|-------|
| `TEST_LOG(label, text)` | Log with escaped control chars |
| `TEST_LOG_RAW(label, text)` | Log raw text |
| `TEST_LOG_HEX(label, text)` | Log as hex bytes |
| `TEST_LOG_FULL(label, text)` | Both visible and hex |
| `TEST_LOG_CMD_LIST(cmd, args...)` | Log command being executed |
| `TEST_LOG_EXIT_CODE(result)` | Colored exit code |

### Framework Architecture

- `tests/framework/wctest.h` — Header-only framework: `TEST()` macro, assertions, runner
- `tests/framework/test_main.cpp` — Entry point → `wctest::default_main()`
- `tests/framework/winuxtest.h` — WinuxCmd test utilities (Pipeline, paths)
- `tests/framework/pipeline.h` — `Pipeline` class for running commands
- `tests/framework/process_win32.h` — Process spawning
- `tests/framework/temp_dir.h` — Temporary directory management

### Targets

| Target | Kind | Sources | Deps |
|--------|------|---------|------|
| `test_main` | static | `tests/framework/test_main.cpp` | — |
| `winux-test-lib` | static | `tests/framework/*.cpp` (excl. test_main.cpp) | — |
| `winuxcmd-tests` | binary | `tests/unit/**/*.cpp` | `winux-test-lib`, `test_main` |

---

## Python Tests (`pytest`)

### Run

```bash
pytest tests/test_bash.py -v
pytest tests/test_bash.py -k "test_ls" -v
pytest tests/test_bash.py::TestBuiltinCommands::test_ls_current_dir -v
```

### Structure

`tests/test_bash.py` — Tests `winuxcmd.exe` via subprocess. Requires a built `winuxcmd.exe` at `build/windows/x64/debug/winuxcmd.exe`.

Two dispatch modes:
- `run_c("cmd", "arg1", ...)` — Built-in command dispatch (argv tokens)
- `run_c_raw("cmd && cmd2")` — Native cmd.exe fallback (shell metacharacters)
- `run_bash("script")` — Bash via `winuxcmd -c "bash -c 'script'"`

### Test Classes

| Class | Description |
|-------|-------------|
| `TestBuiltinCommands` | ls, echo, cat, grep, sort, cut, du, df, find, etc. |
| `TestNativeFallback` | cmd.exe fallback for non-built-in commands |
| `TestShellOperators` | &&, \|\|, pipes, redirects, semicolons |
| `TestEdgeCases` | Empty args, special chars, unicode |
| `TestReturnCodes` | Exit code propagation |
| `TestStressBasic` | Repeated invocations |
| `TestTopLevelFlags` | --help, --version |
| `TestComplexBashCommands` | Bash pipes, redirects, substitution, loops, sed, awk |
