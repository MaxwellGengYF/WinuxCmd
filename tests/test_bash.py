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


# ============================================================================
# ── Helper for bash-native complex commands ─────────────────────────────────
# ============================================================================

def _win_to_bash_path(p: Path) -> str:
    """Convert a Windows path to a Git-Bash style Unix path."""
    s = str(p.resolve()).replace("\\", "/")
    if len(s) >= 2 and s[1] == ":":
        s = f"/{s[0].lower()}{s[2:]}"
    return s


def run_bash(script: str, timeout: int = 30, cwd=None) -> subprocess.CompletedProcess:
    """Run a bash script through ``winuxcmd -c "bash -c '<script>'"``."""
    _require_exe()
    return run_c_raw(f'bash -c "{script}"', timeout=timeout, cwd=cwd)


# ============================================================================
# ── Complex bash commands (similar to D:/kimi-test/tests/test_bash.py) ──────
# ============================================================================


class TestComplexBashCommands:
    """Tests for complex bash commands: pipes, redirects, substitution,
    conditionals, loops, arithmetic, etc.  These exercise bash explicitly
    through the native-shell fallback path.
    """

    # -- pipes -----------------------------------------------------------------

    def test_pipe_echo_to_wc(self) -> None:
        r = run_bash("echo hello | wc -l")
        assert r.returncode == 0
        assert "1" in r.stdout

    def test_pipe_echo_to_grep(self) -> None:
        r = run_bash("echo -e 'apple\\nbanana\\ncherry' | grep ana")
        assert r.returncode == 0
        assert "banana" in r.stdout

    def test_multiple_pipes(self) -> None:
        r = run_bash("echo hello | tr 'a-z' 'A-Z' | tr 'A-Z' 'a-z'")
        assert r.returncode == 0
        assert "hello" in r.stdout

    def test_pipe_ls_to_head(self) -> None:
        r = run_bash("ls / | head -1")
        assert r.returncode == 0
        assert len(r.stdout.strip()) > 0

    # -- redirects -------------------------------------------------------------

    def test_redirect_stdout_to_file(self, tmp_path: Path) -> None:
        outfile = tmp_path / "redirected.txt"
        posix = _win_to_bash_path(outfile)
        r = run_bash(f"echo redirected_content > {posix}")
        assert r.returncode == 0
        assert outfile.read_text(encoding="utf-8").strip() == "redirected_content"

    def test_redirect_append(self, tmp_path: Path) -> None:
        outfile = tmp_path / "append.txt"
        posix = _win_to_bash_path(outfile)
        run_bash(f"echo line1 > {posix}")
        run_bash(f"echo line2 >> {posix}")
        content = outfile.read_text(encoding="utf-8").strip().splitlines()
        assert "line1" in content[0]
        assert "line2" in content[-1]

    def test_stderr_redirect(self, tmp_path: Path) -> None:
        outfile = tmp_path / "stderr.txt"
        posix = _win_to_bash_path(outfile)
        r = run_bash(f"ls nonexistent_dir_xyz 2> {posix}")
        # bash returns non-zero, but the stderr should be in the file
        content = outfile.read_text(encoding="utf-8").lower()
        assert "nonexistent" in content or "cannot access" in content or "no such" in content

    # -- command substitution --------------------------------------------------

    def test_command_substitution(self) -> None:
        r = run_bash("echo $(echo nested)")
        assert r.returncode == 0
        assert "nested" in r.stdout

    def test_backtick_substitution(self) -> None:
        r = run_bash("echo `echo backtick`")
        assert r.returncode == 0
        assert "backtick" in r.stdout

    # -- environment variables -------------------------------------------------

    def test_env_var_home(self) -> None:
        r = run_bash("echo $HOME")
        assert r.returncode == 0
        assert len(r.stdout.strip()) > 0

    def test_env_var_user(self) -> None:
        r = run_bash("echo $USER")
        assert r.returncode == 0
        # USER may be empty on some systems; just check no crash

    # -- semicolon-separated commands ------------------------------------------

    def test_semicolon_chain_bash(self) -> None:
        r = run_bash("echo first; echo second")
        assert r.returncode == 0
        assert "first" in r.stdout
        assert "second" in r.stdout

    def test_and_or_operators(self) -> None:
        r = run_bash("true && echo yes || echo no")
        assert r.returncode == 0
        assert "yes" in r.stdout

    def test_and_or_false_branch(self) -> None:
        r = run_bash("false && echo yes || echo no")
        assert r.returncode == 0
        assert "no" in r.stdout

    # -- conditionals ----------------------------------------------------------

    def test_if_statement(self) -> None:
        r = run_bash("if true; then echo TRUE; else echo FALSE; fi")
        assert r.returncode == 0
        assert "TRUE" in r.stdout

    def test_test_bracket(self) -> None:
        r = run_bash("[ 1 -eq 1 ] && echo equal")
        assert r.returncode == 0
        assert "equal" in r.stdout

    # -- exit codes ------------------------------------------------------------

    def test_exit_code_success_check(self) -> None:
        r = run_bash("true; echo $?")
        assert r.returncode == 0
        assert "0" in r.stdout

    def test_exit_code_failure_check(self) -> None:
        r = run_bash("false; echo $")
        # Note: $? is special; in double-quoted bash -c it may be consumed.
        # Use a safer form.
        r = run_bash("bash -c 'false; echo $?'")
        assert r.returncode == 0
        assert "1" in r.stdout

    # -- here-strings / here-docs ----------------------------------------------

    def test_here_string(self) -> None:
        r = run_bash("cat <<<'herestring'")
        assert r.returncode == 0
        assert "herestring" in r.stdout

    def test_here_doc(self, tmp_path: Path) -> None:
        # cmd.exe truncates at newlines, so write the here-doc to a temp script.
        script = tmp_path / "heredoc.sh"
        script.write_text("cat <<EOF\nheredoc_line\nEOF\n")
        posix = _win_to_bash_path(script)
        r = run_bash(f"bash {posix}")
        assert r.returncode == 0
        assert "heredoc_line" in r.stdout

    # -- globbing --------------------------------------------------------------

    def test_glob_expansion(self, tmp_path: Path) -> None:
        (tmp_path / "a.txt").write_text("a")
        (tmp_path / "b.txt").write_text("b")
        posix = _win_to_bash_path(tmp_path)
        r = run_bash(f"cd {posix} && ls *.txt")
        assert r.returncode == 0
        assert "a.txt" in r.stdout
        assert "b.txt" in r.stdout

    # -- arithmetic expansion --------------------------------------------------

    def test_arithmetic_expansion(self) -> None:
        r = run_bash("echo $((3 + 4))")
        assert r.returncode == 0
        assert "7" in r.stdout

    # -- brace expansion -------------------------------------------------------

    def test_brace_expansion(self) -> None:
        r = run_bash("echo {a,b,c}")
        assert r.returncode == 0
        assert "a b c" in r.stdout

    # -- sub-shell -------------------------------------------------------------

    def test_subshell(self) -> None:
        r = run_bash("(cd / && pwd)")
        assert r.returncode == 0
        assert r.stdout.strip() == "/"

    # -- process substitution --------------------------------------------------

    def test_process_substitution_diff(self, tmp_path: Path) -> None:
        f1 = tmp_path / "f1.txt"
        f2 = tmp_path / "f2.txt"
        f1.write_text("same")
        f2.write_text("same")
        p1 = _win_to_bash_path(f1)
        p2 = _win_to_bash_path(f2)
        r = run_bash(f"diff <(cat {p1}) <(cat {p2})")
        assert r.returncode == 0

    def test_process_substitution_diff_differs(self, tmp_path: Path) -> None:
        f1 = tmp_path / "f1.txt"
        f2 = tmp_path / "f2.txt"
        f1.write_text("one")
        f2.write_text("two")
        p1 = _win_to_bash_path(f1)
        p2 = _win_to_bash_path(f2)
        r = run_bash(f"diff <(cat {p1}) <(cat {p2})")
        assert r.returncode != 0

    # -- inline env ------------------------------------------------------------

    def test_inline_env_override(self) -> None:
        r = run_bash("MYVAR=42 bash -c 'echo $MYVAR'")
        assert r.returncode == 0
        assert "42" in r.stdout

    # -- negation --------------------------------------------------------------

    def test_negation_bang(self) -> None:
        r = run_bash("! false; echo $")
        # $? handling is tricky; re-run safely.
        r = run_bash("bash -c '! false; echo $?'")
        assert r.returncode == 0
        assert "0" in r.stdout

    # -- loops -----------------------------------------------------------------

    def test_for_loop(self) -> None:
        r = run_bash("for i in 1 2 3; do echo $i; done")
        assert r.returncode == 0
        assert "1" in r.stdout
        assert "2" in r.stdout
        assert "3" in r.stdout

    def test_while_loop(self) -> None:
        r = run_bash("i=0; while [ $i -lt 3 ]; do echo $i; i=$((i+1)); done")
        assert r.returncode == 0
        assert "0" in r.stdout
        assert "1" in r.stdout
        assert "2" in r.stdout

    # -- temp file with mktemp -------------------------------------------------

    def test_mktemp(self) -> None:
        r = run_bash("mktemp")
        assert r.returncode == 0
        assert "/tmp" in r.stdout or "/temp" in r.stdout.lower() or "/mktemp" in r.stdout

    # -- printf ----------------------------------------------------------------

    def test_printf_bash(self) -> None:
        r = run_bash("printf '%s %s' hello world")
        assert r.returncode == 0
        assert "hello world" in r.stdout

    # -- array -----------------------------------------------------------------

    def test_array(self) -> None:
        r = run_bash("arr=(one two three); echo ${arr[1]}")
        assert r.returncode == 0
        assert "two" in r.stdout

    # -- string manipulation ---------------------------------------------------

    def test_string_length(self) -> None:
        r = run_bash("s=abcdef; echo ${#s}")
        assert r.returncode == 0
        assert "6" in r.stdout

    def test_string_substring(self) -> None:
        r = run_bash("s=hello; echo ${s:1:3}")
        assert r.returncode == 0
        assert "ell" in r.stdout

    # -- sed -------------------------------------------------------------------

    def test_sed_substitution(self) -> None:
        r = run_bash("echo foo | sed 's/foo/bar/'")
        assert r.returncode == 0
        assert "bar" in r.stdout

    # -- awk -------------------------------------------------------------------

    def test_awk_field(self) -> None:
        r = run_bash("echo 'a b c' | awk '{print $2}'")
        assert r.returncode == 0
        assert "b" in r.stdout

    # -- cut -------------------------------------------------------------------

    def test_cut_delimiter_bash(self) -> None:
        r = run_bash("echo 'a:b:c' | cut -d: -f2")
        assert r.returncode == 0
        assert "b" in r.stdout

    # -- sort / uniq -----------------------------------------------------------

    def test_sort_uniq_bash(self) -> None:
        r = run_bash("echo -e 'c\\na\\nb\\na' | sort | uniq")
        assert r.returncode == 0
        lines = [l for l in r.stdout.strip().splitlines() if l]
        assert lines == ["a", "b", "c"]

    # -- head / tail -----------------------------------------------------------

    def test_head_n(self) -> None:
        r = run_bash("seq 10 | head -3")
        assert r.returncode == 0
        lines = [l for l in r.stdout.strip().splitlines() if l]
        assert len(lines) == 3

    def test_tail_n(self) -> None:
        r = run_bash("seq 10 | tail -3")
        assert r.returncode == 0
        lines = [l for l in r.stdout.strip().splitlines() if l]
        assert "8" in lines[0] or "9" in lines[0]
        assert "10" in lines[-1]

    # -- tee -------------------------------------------------------------------

    def test_tee(self, tmp_path: Path) -> None:
        outfile = tmp_path / "tee_out.txt"
        posix = _win_to_bash_path(outfile)
        r = run_bash(f"echo hello_tee | tee {posix}")
        assert r.returncode == 0
        assert "hello_tee" in r.stdout
        assert outfile.read_text(encoding="utf-8").strip() == "hello_tee"

    # -- exit with explicit code -----------------------------------------------

    def test_exit_explicit_code(self) -> None:
        r = run_bash("exit 42")
        assert r.returncode != 0
        assert r.returncode == 42

    # -- chained pipes with special chars --------------------------------------

    def test_pipe_with_dollar_signs(self) -> None:
        r = run_bash("echo '$HOME' | cat")
        assert r.returncode == 0
        assert "$HOME" in r.stdout

    # -- background process via & ----------------------------------------------

    def test_background_ampersand(self) -> None:
        r = run_bash("sleep 1 & wait", timeout=10)
        assert r.returncode == 0

    # -- dirname / basename ----------------------------------------------------

    def test_dirname_basename_bash(self) -> None:
        r = run_bash("dirname /usr/bin/bash && basename /usr/bin/bash")
        assert r.returncode == 0
        assert "/usr/bin" in r.stdout
        assert "bash" in r.stdout

    # -- xargs -----------------------------------------------------------------

    def test_xargs(self) -> None:
        r = run_bash("echo 'a b c' | xargs -n1 echo")
        assert r.returncode == 0
        assert "a" in r.stdout
        assert "b" in r.stdout
        assert "c" in r.stdout

    # -- trap ------------------------------------------------------------------

    def test_trap_does_not_crash(self) -> None:
        r = run_bash("trap 'echo trapped' EXIT; echo done")
        assert r.returncode == 0
        assert "done" in r.stdout
        assert "trapped" in r.stdout

    # -- backslash escapes before metacharacters -------------------------------

    def test_find_with_escaped_parens(self, tmp_path: Path) -> None:
        (tmp_path / "foo.txt").write_text("foo")
        (tmp_path / "bar.py").write_text("bar")
        (tmp_path / "baz.txt").write_text("baz")
        posix = _win_to_bash_path(tmp_path)
        r = run_bash(
            f"find {posix} -maxdepth 1 \\( -name '*.txt' -o -name '*.py' \\) | sort"
        )
        assert r.returncode == 0
        assert "foo.txt" in r.stdout
        assert "bar.py" in r.stdout
        assert "baz.txt" in r.stdout

    def test_echo_escaped_pipe(self) -> None:
        r = run_bash("echo 'a|b' | cat")
        assert r.returncode == 0
        assert "a|b" in r.stdout

    def test_echo_escaped_glob(self) -> None:
        r = run_bash("echo '*'")
        assert r.returncode == 0
        assert "*" in r.stdout

    def test_echo_escaped_semicolon(self) -> None:
        r = run_bash("echo 'a;b'")
        assert r.returncode == 0
        assert "a;b" in r.stdout
