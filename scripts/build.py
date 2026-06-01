#!/usr/bin/env python3
"""Build WinuxCmd using XMake.

Run xmake config and build in one go, with sensible defaults per platform.
Optionally copies build artifacts and creates command links after a successful
build (same logic as scripts/copy.py).
"""

import argparse
import os
import platform
import subprocess
import sys
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parent.parent

# Command names derived from src/commands/*.cpp (kept in sync with copy.py)
_COMMAND_NAMES = [
    "arch", "b2sum", "base32", "base64", "basename", "basenc", "cal", "cat",
    "chmod", "chown", "cksum", "clear", "cmp", "column", "comm", "cp", "cpio",
    "csplit", "cut", "cygpath", "d2u", "date", "dd", "df", "diff", "diff3",
    "dirname", "dos2unix", "du", "echo", "env", "expand", "expr", "factor",
    "false", "file", "find", "fmt", "fold", "free", "getconf", "grep", "groups",
    "head", "hmac256", "hostid", "hostname", "id", "infocmp", "install", "join",
    "jq", "kill", "less", "link", "ln", "locale", "logname", "ls", "lsof", "man",
    "md5sum", "mkdir", "mktemp", "mpicalc", "mv", "nice", "nl", "nohup", "nproc",
    "numfmt", "od", "paste", "patch", "pathchk", "pinky", "pr", "printenv",
    "printf", "ps", "ptx", "pwd", "readlink", "realpath", "reset", "rev", "rm",
    "rmdir", "sdiff", "sed", "seq", "sha1sum", "sha224sum", "sha256sum",
    "sha384sum", "sha512sum", "shred", "shuf", "sleep", "sort", "split", "stat",
    "stdbuf", "sum", "sync", "tac", "tail", "tee", "test", "[", "tic", "timeout",
    "toe", "top", "touch", "tput", "tr", "tree", "true", "truncate", "tsort",
    "tty", "tzset", "u2d", "uname", "unexpand", "uniq", "unix2dos", "unlink",
    "uptime", "users", "watch", "wc", "which", "who", "whoami", "xargs", "xxd",
    "yes",
]


def log_info(msg: str) -> None:
    print(f"[INFO] {msg}")


def log_ok(msg: str) -> None:
    print(f"[OK]   {msg}")


def log_error(msg: str) -> None:
    print(f"[ERR]  {msg}", file=sys.stderr)


def log_warn(msg: str) -> None:
    print(f"[WARN] {msg}", file=sys.stderr)


def detect_toolchain() -> str:
    """Pick a default toolchain for the current platform."""
    system = platform.system().lower()
    if system == "windows":
        for candidate in ["msvc", "clang-cl", "llvm", "gcc"]:
            if _has_toolchain(candidate):
                return candidate
        return "msvc"
    if system == "darwin":
        return "clang"
    for candidate in ["gcc", "clang"]:
        if _has_toolchain(candidate):
            return candidate
    return "gcc"


def _has_toolchain(name: str) -> bool:
    """Ask xmake whether a toolchain is available."""
    try:
        result = subprocess.run(
            ["xmake", "show", "--toolchains"],
            capture_output=True,
            text=True,
            timeout=10,
        )
        return name in result.stdout
    except Exception:
        return False


def run_xmake(args: list[str], cwd: Path | None = None) -> int:
    """Run an xmake command and stream output."""
    log_info("Executing: xmake " + " ".join(args))
    result = subprocess.run(
        ["xmake"] + args,
        cwd=cwd,
    )
    return result.returncode


def find_winuxcmd_exe(root: Path, mode: str = "release") -> Path | None:
    """Locate the built winuxcmd.exe under the project root.

    *mode* must match the build mode used ('debug', 'release', 'releasedbg').
    The mode-specific directory is always searched first to avoid picking up
    artifacts from a previous build with a different mode.
    """
    candidates = [
        root / "build" / "windows" / "x64" / mode / "winuxcmd.exe",
        root / "build" / "winuxcmd.exe",
        root / "winuxcmd.exe",
    ]
    for c in candidates:
        if c.exists():
            return c
    for exe in root.rglob("winuxcmd.exe"):
        if ".winuxcmd" not in str(exe):
            return exe
    return None


def _create_link(link_path: Path, target: Path, use_copy: bool = False) -> bool:
    """Create a hardlink or copy."""
    try:
        if link_path.exists() or link_path.is_symlink():
            link_path.unlink()
        if use_copy:
            import shutil
            shutil.copy2(target, link_path)
        else:
            if platform.system().lower() == "windows":
                import ctypes
                kernel32 = ctypes.windll.kernel32
                kernel32.CreateHardLinkW(str(link_path), str(target), None)
            else:
                os.link(target, link_path)
        return True
    except Exception as e:
        log_warn(f"Failed to link {link_path.name}: {e}")
        return False


def copy_artifacts(
    root: Path,
    dest: Path,
    mode: str = "release",
    force: bool = False,
    use_copy: bool = False,
) -> int:
    """Copy winuxcmd.exe and create command links in *dest*."""
    exe = find_winuxcmd_exe(root, mode=mode)
    if exe is None:
        log_error("winuxcmd.exe not found. Build the project first.")
        return 1

    log_info(f"Copy source: {exe}")
    log_info(f"Copy destination: {dest}")

    dest.mkdir(parents=True, exist_ok=True)

    dest_exe = dest / "winuxcmd.exe"
    if not _create_link(dest_exe, exe, use_copy=True):
        log_error("Failed to copy winuxcmd.exe")
        return 1
    log_ok("winuxcmd.exe copied")
    return 0

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build WinuxCmd with XMake",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                          # release build, auto-detect toolchain
  %(prog)s -m debug                 # debug build
  %(prog)s -c                       # clean config cache before building
  %(prog)s -r                       # force rebuild
  %(prog)s -j 8                     # use 8 parallel jobs
  %(prog)s --toolchain clang        # use Clang instead of default
  %(prog)s --copy-to ./dist         # build + copy artifacts to ./dist
        """.strip(),
    )
    parser.add_argument(
        "-m", "--mode",
        default="release",
        choices=["release", "debug", "releasedbg"],
        help="build mode (default: release)",
    )
    parser.add_argument(
        "-p", "--plat",
        default=None,
        help="platform (default: auto-detect)",
    )
    parser.add_argument(
        "-a", "--arch",
        default=None,
        help="architecture (default: auto-detect)",
    )
    parser.add_argument(
        "--toolchain",
        default=None,
        help="toolchain name (default: auto-detect)",
    )
    parser.add_argument(
        "-c", "--clean",
        action="store_true",
        help="clean configuration cache before configuring",
    )
    parser.add_argument(
        "-r", "--rebuild",
        action="store_true",
        help="force rebuild",
    )
    parser.add_argument(
        "-j", "--jobs",
        type=int,
        default=None,
        help="number of parallel build jobs",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="verbose output",
    )
    parser.add_argument(
        "-t", "--target",
        default=None,
        help="specific target to build (default: all)",
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=PROJECT_ROOT,
        help=f"project root directory (default: {PROJECT_ROOT})",
    )
    parser.add_argument(
        "--copy-to",
        dest="copy_to",
        type=Path,
        default=None,
        help="copy build artifacts to this directory after build",
    )
    parser.add_argument(
        "--copy-force",
        dest="copy_force",
        action="store_true",
        help="overwrite existing files when copying",
    )
    parser.add_argument(
        "--copy",
        action="store_true",
        help="use file copies instead of hardlinks for command stubs",
    )

    args = parser.parse_args()

    # Determine platform defaults
    plat = args.plat
    arch = args.arch
    toolchain = args.toolchain

    if plat is None:
        system = platform.system().lower()
        if system == "windows":
            plat = "windows"
        elif system == "darwin":
            plat = "macosx"
        else:
            plat = "linux"

    if arch is None:
        machine = platform.machine().lower()
        if plat == "windows":
            arch = "x64"
        elif machine in ("amd64", "x86_64"):
            arch = "x86_64"
        elif machine in ("arm64", "aarch64"):
            arch = "arm64"
        else:
            arch = machine

    if toolchain is None:
        toolchain = detect_toolchain()

    log_info(f"Platform:  {plat}")
    log_info(f"Arch:      {arch}")
    log_info(f"Toolchain: {toolchain}")
    log_info(f"Mode:      {args.mode}")
    log_info(f"Root:      {args.root.resolve()}")

    # Build xmake config arguments
    config_args = [
        "f",
        "-p", plat,
        "-a", arch,
        f"--toolchain={toolchain}",
        "-m", args.mode,
    ]
    if args.clean:
        config_args.append("-c")
    if args.verbose:
        config_args.append("--verbose")

    # Config
    rc = run_xmake(config_args, cwd=args.root)
    if rc != 0:
        log_error("xmake config failed")
        return rc
    log_ok("xmake config completed")

    # Build
    build_args = []
    if args.rebuild:
        build_args.append("-r")
    if args.verbose:
        build_args.append("--verbose")
    if args.jobs is not None:
        build_args.extend(["-j", str(args.jobs)])
    if args.target:
        build_args.append(args.target)

    rc = run_xmake(build_args, cwd=args.root)
    if rc != 0:
        log_error("xmake build failed")
        return rc

    log_ok("xmake build completed")

    # Copy artifacts if requested
    if args.copy_to is not None:
        rc = copy_artifacts(
            root=args.root,
            dest=args.copy_to,
            mode=args.mode,
            force=args.copy_force,
            use_copy=args.copy,
        )
        if rc != 0:
            return rc

    return 0


if __name__ == "__main__":
    sys.exit(main())
