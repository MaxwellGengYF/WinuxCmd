"""Comprehensive tests for winuxcmd -c (shell command execution).

Tests both built-in CommandRegistry dispatch and native cmd.exe fallback.
Uses the built winuxcmd.exe directly via subprocess.

Important: ``winuxcmd -c <cmd> [args...]`` dispatches to the registry with
``<cmd>`` as the command name and ``[args...]`` as its arguments.  To
exercise this correctly the test helper passes each token as a separate
argument to winuxcmd.exe.
"""

import subprocess
from pathlib import Path

import pytest

# ---------------------------------------------------------------------------
# Locate winuxcmd.exe
# ---------------------------------------------------------------------------

PROJECT_ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = PROJECT_ROOT / "build" / "windows" / "x64" / "debug"
WINUXCMD_EXE = BUILD_DIR / "winuxcmd.exe"


def _require_exe() -> None:
    if not WINUXCMD_EXE.exists():
        pytest.skip(f"winuxcmd.exe not found at {WINUXCMD_EXE}")


def run_c(*args: str, timeout: int = 30, cwd=None) -> subprocess.CompletedProcess:
    """Run ``winuxcmd -c <arg0> <arg1> ...`` — one argv token per positional.

    This is the primary interface for testing *built-in* command dispatch.
    """
    _require_exe()
    return subprocess.run(
        [str(WINUXCMD_EXE), "-c"] + list(args),
        capture_output=True,
        text=True,
        errors="replace",
        timeout=timeout,
        cwd=cwd,
    )


def run_c_raw(cmd: str, timeout: int = 30, cwd=None) -> subprocess.CompletedProcess:
    """Run ``winuxcmd -c <cmd>`` with a *single* raw command string.

    Use this for testing shell meta-characters (pipes, redirects, &&, ||)
    that must be passed untouched to the native cmd.exe fallback.
    """
    _require_exe()
    return subprocess.run(
        [str(WINUXCMD_EXE), "-c", cmd],
        capture_output=True,
        text=True,
        errors="replace",
        timeout=timeout,
        cwd=cwd,
    )


# ============================================================================
# ── Built-in command dispatch (the core feature) ───────────────────────────
# ============================================================================


class TestBuiltinCommands:
    """Test that -c dispatches to built-in CommandRegistry commands."""

    # -- ls -----------------------------------------------------------------

    def test_ls_current_dir(self) -> None:
        r = run_c("ls")
        assert r.returncode == 0
        assert "src" in r.stdout or "tests" in r.stdout

    def test_ls_with_args(self) -> None:
        r = run_c("ls", "-la")
        assert r.returncode == 0
        assert "rwx" in r.stdout or "rw-" in r.stdout or "drwx" in r.stdout

    def test_ls_multiple_dirs(self, tmp_path: Path) -> None:
        d1 = tmp_path / "d1"
        d2 = tmp_path / "d2"
        d1.mkdir()
        d2.mkdir()
        r = run_c("ls", str(d1), str(d2))
        assert r.returncode == 0

    def test_ls_nonexistent_dir(self) -> None:
        r = run_c("ls", "nonexistent_dir_12345")
        assert r.returncode != 0

    # -- echo ---------------------------------------------------------------

    def test_echo_basic(self) -> None:
        r = run_c("echo", "hello", "world")
        assert r.returncode == 0
        assert "hello world" in r.stdout

    def test_echo_n_flag(self) -> None:
        r = run_c("echo", "-n", "no_newline")
        assert r.returncode == 0
        assert "no_newline" in r.stdout

    def test_echo_e_flag(self) -> None:
        r = run_c("echo", "-e", r"hello\nworld")
        assert r.returncode == 0
        assert "hello" in r.stdout
        assert "world" in r.stdout

    def test_echo_multiple_args(self) -> None:
        r = run_c("echo", "a", "b", "c", "d", "e")
        assert r.returncode == 0
        assert "a b c d e" in r.stdout

    def test_echo_preserves_spaces(self) -> None:
        r = run_c("echo", "hello    world")
        assert r.returncode == 0
        assert "hello    world" in r.stdout

    # -- cat ----------------------------------------------------------------

    def test_cat_file(self, tmp_path: Path) -> None:
        f = tmp_path / "cat_test.txt"
        f.write_text("meow meow", encoding="utf-8")
        r = run_c("cat", str(f))
        assert r.returncode == 0
        assert "meow meow" in r.stdout

    def test_cat_n_flag(self, tmp_path: Path) -> None:
        f = tmp_path / "cat_n_test.txt"
        f.write_text("line1\nline2\n", encoding="utf-8")
        r = run_c("cat", "-n", str(f))
        assert r.returncode == 0
        assert "1" in r.stdout
        assert "line1" in r.stdout
        assert "2" in r.stdout

    def test_cat_nonexistent(self) -> None:
        r = run_c("cat", "nonexistent_file_12345")
        assert r.returncode != 0

    # -- grep ---------------------------------------------------------------

    def test_grep_literal(self, tmp_path: Path) -> None:
        f = tmp_path / "grep_test.txt"
        f.write_text("apple\nbanana\ncherry\n", encoding="utf-8")
        r = run_c("grep", "ana", str(f))
        assert r.returncode == 0
        assert "banana" in r.stdout
        assert "apple" not in r.stdout

    def test_grep_invert(self, tmp_path: Path) -> None:
        f = tmp_path / "grep_v_test.txt"
        f.write_text("keep\nskip\nkeep\n", encoding="utf-8")
        r = run_c("grep", "-v", "skip", str(f))
        assert r.returncode == 0
        assert "skip" not in r.stdout
        assert "keep" in r.stdout

    def test_grep_no_match(self, tmp_path: Path) -> None:
        f = tmp_path / "grep_nomatch.txt"
        f.write_text("nothing here\n")
        r = run_c("grep", "absent", str(f))
        assert r.returncode != 0

    def test_grep_multiple_files(self, tmp_path: Path) -> None:
        f1 = tmp_path / "g1.txt"
        f2 = tmp_path / "g2.txt"
        f1.write_text("apple\n")
        f2.write_text("apple pie\n")
        r = run_c("grep", "apple", str(f1), str(f2))
        assert r.returncode == 0
        assert "apple" in r.stdout

    # -- sort ---------------------------------------------------------------

    def test_sort(self, tmp_path: Path) -> None:
        f = tmp_path / "sort_test.txt"
        f.write_text("c\na\nb\n", encoding="utf-8")
        r = run_c("sort", str(f))
        assert r.returncode == 0
        lines = [l for l in r.stdout.strip().splitlines() if l]
        assert lines == ["a", "b", "c"]

    def test_sort_reverse(self, tmp_path: Path) -> None:
        f = tmp_path / "sort_r_test.txt"
        f.write_text("a\nb\nc\n", encoding="utf-8")
        r = run_c("sort", "-r", str(f))
        assert r.returncode == 0
        lines = [l for l in r.stdout.strip().splitlines() if l]
        assert lines == ["c", "b", "a"]

    # -- cut ----------------------------------------------------------------

    def test_cut_delimiter(self, tmp_path: Path) -> None:
        f = tmp_path / "cut_test.txt"
        f.write_text("a:b:c\nd:e:f\n", encoding="utf-8")
        r = run_c("cut", "-d:", "-f2", str(f))
        assert r.returncode == 0
        assert "b" in r.stdout
        assert "e" in r.stdout

    # -- du / df / date / dirname / basename --------------------------------

    def test_du(self) -> None:
        r = run_c("du", "-s", str(PROJECT_ROOT / "src"))
        assert r.returncode == 0
        assert len(r.stdout.strip()) > 0

    def test_df(self) -> None:
        r = run_c("df")
        assert r.returncode == 0
        assert len(r.stdout.strip()) > 0

    def test_date(self) -> None:
        r = run_c("date")
        assert r.returncode == 0
        assert len(r.stdout.strip()) > 0

    def test_dirname(self) -> None:
        r = run_c("dirname", "/usr/bin/bash")
        assert r.returncode == 0
        assert "/usr/bin" in r.stdout

    def test_basename(self) -> None:
        r = run_c("basename", "/usr/bin/bash")
        assert r.returncode == 0
        assert "bash" in r.stdout.strip()

    # -- env ----------------------------------------------------------------

    def test_env(self) -> None:
        r = run_c("env")
        assert r.returncode == 0
        assert "PATH" in r.stdout or "path" in r.stdout.lower()

    # -- printf -------------------------------------------------------------

    def test_printf_builtin(self) -> None:
        r = run_c("printf", "%s %s", "hello", "world")
        assert r.returncode == 0
        assert "hello world" in r.stdout

    # -- true / false -------------------------------------------------------

    def test_true(self) -> None:
        r = run_c("true")
        assert r.returncode == 0
        assert r.stdout.strip() == ""

    def test_false(self) -> None:
        r = run_c("false")
        assert r.returncode != 0

    # -- cmp ----------------------------------------------------------------

    def test_cmp_identical(self, tmp_path: Path) -> None:
        f1 = tmp_path / "cmp_a.txt"
        f2 = tmp_path / "cmp_b.txt"
        f1.write_text("same content")
        f2.write_text("same content")
        r = run_c("cmp", str(f1), str(f2))
        assert r.returncode == 0

    def test_cmp_different(self, tmp_path: Path) -> None:
        f1 = tmp_path / "cmp_c.txt"
        f2 = tmp_path / "cmp_d.txt"
        f1.write_text("content A")
        f2.write_text("content B")
        r = run_c("cmp", str(f1), str(f2))
        assert r.returncode != 0

    # -- diff ---------------------------------------------------------------

    def test_diff_identical(self, tmp_path: Path) -> None:
        f1 = tmp_path / "diff_a.txt"
        f2 = tmp_path / "diff_b.txt"
        f1.write_text("same\ncontent\n")
        f2.write_text("same\ncontent\n")
        r = run_c("diff", str(f1), str(f2))
        assert r.returncode == 0

    def test_diff_different(self, tmp_path: Path) -> None:
        f1 = tmp_path / "diff_c.txt"
        f2 = tmp_path / "diff_d.txt"
        f1.write_text("hello\n")
        f2.write_text("world\n")
        r = run_c("diff", str(f1), str(f2))
        assert r.returncode != 0

    # -- cp / mv / mkdir / rmdir --------------------------------------------

    def test_cp(self, tmp_path: Path) -> None:
        src = tmp_path / "cp_src.txt"
        dst = tmp_path / "cp_dst.txt"
        src.write_text("copy me")
        r = run_c("cp", str(src), str(dst))
        assert r.returncode == 0
        assert dst.read_text() == "copy me"

    def test_mv(self, tmp_path: Path) -> None:
        src = tmp_path / "mv_src.txt"
        dst = tmp_path / "mv_dst.txt"
        src.write_text("move me")
        r = run_c("mv", str(src), str(dst))
        assert r.returncode == 0
        assert not src.exists()
        assert dst.read_text() == "move me"

    def test_mkdir_rmdir(self, tmp_path: Path) -> None:
        d = tmp_path / "test_mkdir"
        r = run_c("mkdir", str(d))
        assert r.returncode == 0
        assert d.is_dir()
        r2 = run_c("rmdir", str(d))
        assert r2.returncode == 0
        assert not d.exists()

    # -- find (built-in) ----------------------------------------------------

    def test_find_builtin(self, tmp_path: Path) -> None:
        (tmp_path / "find_a.txt").write_text("a")
        (tmp_path / "find_b.txt").write_text("b")
        r = run_c("find", str(tmp_path), "-name", "find_a.txt")
        assert r.returncode == 0
        assert "find_a.txt" in r.stdout

    # -- help / version flags on a command ----------------------------------

    def test_ls_help_flag(self) -> None:
        r = run_c("ls", "--help")
        assert r.returncode == 0
        assert len(r.stdout) > 0

    def test_ls_version_flag(self) -> None:
        # --version via -c dispatches to the command; ls treats it as a file.
        # Top-level --version is tested in TestTopLevelFlags.
        r = run_c("ls", "--version")
        # ls --version is not special-cased in -c mode; it fails (file not found)
        assert r.returncode != 0


# ============================================================================
# ── Native shell fallback ───────────────────────────────────────────────────
# ============================================================================


class TestNativeFallback:
    """Test that non-built-in commands fall back to cmd.exe."""

    def test_cmd_dir(self) -> None:
        """'dir' is actually a built-in on WinuxCmd, so it dispatches there."""
        r = run_c("dir")
        assert r.returncode == 0
        # Built-in dir outputs a simple listing (not cmd.exe format with Volume/<DIR>)
        assert len(r.stdout) > 0

    def test_cmd_type(self, tmp_path: Path) -> None:
        f = tmp_path / "type_test.txt"
        f.write_text("cmd type test")
        r = run_c("type", str(f))
        # 'type' is not a built-in → falls back to cmd.exe type
        assert r.returncode == 0
        assert "cmd type test" in r.stdout

    def test_cmd_set(self) -> None:
        r = run_c("set")
        assert r.returncode == 0
        assert "Path" in r.stdout or "PATH" in r.stdout or "windir" in r.stdout.lower()

    def test_nonexistent_command(self) -> None:
        r = run_c("no_such_command_xyz_12345")
        assert r.returncode != 0

    def test_cmd_echo_time(self) -> None:
        """%TIME% is expanded by cmd.exe, proving native fallback."""
        r = run_c_raw("echo %TIME%")
        assert r.returncode == 0
        # Should contain something looking like a time
        assert len(r.stdout.strip()) > 0


# ============================================================================
# ── Shell operators (pipes, redirects, chaining) via native fallback ────────
# ============================================================================


class TestShellOperators:
    """Shell meta-characters go through native cmd.exe fallback."""

    def test_and_operator(self) -> None:
        r = run_c_raw("echo first && echo second")
        assert r.returncode == 0
        assert "first" in r.stdout
        assert "second" in r.stdout

    def test_or_operator_true(self) -> None:
        r = run_c_raw("echo ok || echo nope")
        assert r.returncode == 0
        assert "ok" in r.stdout
        assert "nope" not in r.stdout

    def test_and_failure_stops(self) -> None:
        r = run_c_raw("no_such_cmd_12345 && echo should_not_appear")
        assert r.returncode != 0
        assert "should_not_appear" not in r.stdout

    def test_pipe_with_findstr(self) -> None:
        r = run_c_raw("echo hello world | findstr hello")
        assert r.returncode == 0
        assert "hello" in r.stdout.lower()

    def test_redirect_stdout(self, tmp_path: Path) -> None:
        outfile = tmp_path / "redirect_out.txt"
        r = run_c_raw(f"echo redirected_text > {outfile}")
        assert r.returncode == 0
        assert outfile.read_text().strip() == "redirected_text"

    def test_redirect_append(self, tmp_path: Path) -> None:
        outfile = tmp_path / "append_out.txt"
        run_c_raw(f"echo line1 > {outfile}")
        run_c_raw(f"echo line2 >> {outfile}")
        content = outfile.read_text().strip().splitlines()
        assert "line1" in content[0]
        assert "line2" in content[1]

    def test_semicolon_chain(self) -> None:
        r = run_c_raw("echo one & echo two")
        assert r.returncode == 0
        assert "one" in r.stdout
        assert "two" in r.stdout


# ============================================================================
# ── Edge cases & error handling ─────────────────────────────────────────────
# ============================================================================


class TestEdgeCases:
    """Edge cases for -c command execution."""

    def test_empty_args(self) -> None:
        """-c with no command should error."""
        r = subprocess.run(
            [str(WINUXCMD_EXE), "-c"],
            capture_output=True,
            text=True,
            errors="replace",
            timeout=10,
        )
        assert r.returncode != 0
        assert "requires" in r.stderr.lower()

    def test_command_with_special_chars(self) -> None:
        r = run_c("echo", "special!@#$%^&*()")
        assert r.returncode == 0

    def test_command_with_unicode(self) -> None:
        r = run_c("echo", "你好世界")
        assert r.returncode == 0
        assert "你好世界" in r.stdout


# ============================================================================
# ── Return code propagation ─────────────────────────────────────────────────
# ============================================================================


class TestReturnCodes:
    """Verify that winuxcmd -c propagates the child's exit code."""

    def test_success_returns_zero(self) -> None:
        assert run_c("echo", "ok").returncode == 0

    def test_builtin_error_nonzero(self) -> None:
        assert run_c("cat", "nonexistent_abc_xyz").returncode != 0

    def test_native_error_nonzero(self) -> None:
        # 'dir' on a nonexistent path fails with non-zero
        r = run_c_raw("dir nonexistent_abc_xyz")
        assert r.returncode != 0

    def test_false_returns_nonzero(self) -> None:
        assert run_c("false").returncode != 0


# ============================================================================
# ── Stress / basic throughput ───────────────────────────────────────────────
# ============================================================================


class TestStressBasic:
    """Repeated invocations to verify stability."""

    def test_many_echos(self) -> None:
        for _ in range(20):
            r = run_c("echo", "quick")
            assert r.returncode == 0
            assert "quick" in r.stdout

    def test_many_ls(self) -> None:
        for _ in range(10):
            r = run_c("ls", str(PROJECT_ROOT))
            assert r.returncode == 0


# ============================================================================
# ── winuxcmd (without -c) top-level flags ───────────────────────────────────
# ============================================================================


class TestTopLevelFlags:
    """--help and --version when used *without* -c (direct top-level)."""

    def test_help_direct(self) -> None:
        r = subprocess.run(
            [str(WINUXCMD_EXE), "--help"],
            capture_output=True,
            text=True,
            errors="replace",
            timeout=10,
        )
        assert r.returncode == 1  # help returns 1
        assert "WinuxCmd" in r.stdout

    def test_version_direct(self) -> None:
        r = subprocess.run(
            [str(WINUXCMD_EXE), "--version"],
            capture_output=True,
            text=True,
            errors="replace",
            timeout=10,
        )
        assert r.returncode == 0
        assert "WinuxCmd" in r.stdout
