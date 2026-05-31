/*
 *  Copyright ? 2026 WinuxCmd
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 *
 *  - File: install.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implementation for install.
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright ? 2026 WinuxCmd

#include "core/command_macros.h"
#include "../core/core.h"
#include "../utils/utils.h"
#include "../container/container.h"

using cmd::meta::OptionType;

auto constexpr INSTALL_OPTIONS = std::array{
    OPTION("-b", "--backup", "make a backup of each existing destination file",
           BOOL_TYPE),
    OPTION("-c", "", "ignored (for compatibility with old Unix versions)",
           BOOL_TYPE),
    OPTION("-C", "", "ignored (for compatibility with old Unix versions)",
           BOOL_TYPE),
    OPTION("-d", "--directory", "treat all arguments as directory names",
           BOOL_TYPE),
    OPTION("-D", "", "create all leading components of DEST except the last",
           BOOL_TYPE),
    OPTION("-g", "--group", "set group ownership", STRING_TYPE),
    OPTION("-m", "--mode", "set permission mode", STRING_TYPE),
    OPTION("-o", "--owner", "set ownership", STRING_TYPE),
    OPTION("-p", "--preserve-timestamps",
           "apply access/modification times of SOURCE files", BOOL_TYPE),
    OPTION("-s", "--strip", "strip symbol tables", BOOL_TYPE),
    OPTION("", "--strip-program", "program used to strip binaries",
           STRING_TYPE),
    OPTION("-S", "--suffix", "override the usual backup suffix", STRING_TYPE),
    OPTION("-t", "--target-directory", "specify the destination directory",
           STRING_TYPE),
    OPTION("-T", "--no-target-directory",
           "do not treat the last operand specially when it is a directory",
           BOOL_TYPE),
    OPTION("-v", "--verbose",
           "print the name of each directory as it is created", BOOL_TYPE),
    OPTION("", "--preserve-context", "preserve SELinux security context",
           BOOL_TYPE),
    OPTION("-Z", "",
           "set SELinux security context of destination files to default",
           BOOL_TYPE)};

namespace install_pipeline {
namespace cp = core::pipeline;

struct Config {
  bool backup = false;
  bool directory_mode = false;
  bool preserve_timestamps = false;
  bool strip = false;
  bool verbose = false;
  bool create_leading_dirs = false;
  bool no_target_directory = false;
  bool compare = false;
  bool preserve_context = false;
  bool default_context = false;
  std::string backup_suffix = "~";
  std::string group;
  std::string mode;
  std::string owner;
  std::string strip_program;
  std::string target_dir;
  SmallVector<std::string, 64> sources;
};

auto build_config(const CommandContext<INSTALL_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;
  cfg.backup = ctx.get<bool>("--backup", false) || ctx.get<bool>("-b", false);
  cfg.directory_mode =
      ctx.get<bool>("--directory", false) || ctx.get<bool>("-d", false);
  cfg.preserve_timestamps = ctx.get<bool>("--preserve-timestamps", false) ||
                            ctx.get<bool>("-p", false);
  cfg.strip = ctx.get<bool>("--strip", false) || ctx.get<bool>("-s", false);
  cfg.verbose = ctx.get<bool>("--verbose", false) || ctx.get<bool>("-v", false);
  	  cfg.create_leading_dirs = ctx.get<bool>("-D", false);
	  cfg.compare =
	      ctx.get<bool>("-C", false) || ctx.get<bool>("-c", false);
	  cfg.no_target_directory = ctx.get<bool>("-T", false) ||
	                            ctx.get<bool>("--no-target-directory", false);
	  cfg.default_context = ctx.get<bool>("-Z", false);

  auto group_opt = ctx.get<std::string>("--group", "");
  if (group_opt.empty()) {
    group_opt = ctx.get<std::string>("-g", "");
  }
  cfg.group = group_opt;

  auto mode_opt = ctx.get<std::string>("--mode", "");
  if (mode_opt.empty()) {
    mode_opt = ctx.get<std::string>("-m", "");
  }
  cfg.mode = mode_opt;

  auto owner_opt = ctx.get<std::string>("--owner", "");
  if (owner_opt.empty()) {
    owner_opt = ctx.get<std::string>("-o", "");
  }
  cfg.owner = owner_opt;

  auto suffix_opt = ctx.get<std::string>("--suffix", "");
  if (suffix_opt.empty()) {
    suffix_opt = ctx.get<std::string>("-S", "");
  }
  if (!suffix_opt.empty()) {
    cfg.backup_suffix = suffix_opt;
  }

  cfg.strip_program = ctx.get<std::string>("--strip-program", "");

  auto target_opt = ctx.get<std::string>("--target-directory", "");
  if (target_opt.empty()) {
    target_opt = ctx.get<std::string>("-t", "");
  }
  cfg.target_dir = target_opt;

	  // Parse positionals: first N-1 are sources (wildcard expanded),
	  // the last one is the DEST (literal, not wildcard expanded)
	  if (cfg.directory_mode) {
	    // -d mode: all positionals are directory names to create
	    for (auto arg : ctx.positionals) {
	      cfg.sources.push_back(std::string(arg));
	    }
	  } else if (!cfg.target_dir.empty()) {
	    // -t DIR SOURCE...: target is specified separately, all positionals are sources
	    for (auto arg : ctx.positionals) {
	      std::string file_arg(arg);
	      if (contains_wildcard(file_arg)) {
	        auto glob_result = glob_expand(file_arg);
	        if (glob_result.expanded) {
	          for (const auto& file : glob_result.files) {
	            cfg.sources.push_back(wstring_to_utf8(file));
	          }
	          continue;
	        }
	      }
	      cfg.sources.push_back(file_arg);
	    }
	  } else {
	    // SOURCE... DEST: last positional is the destination (literal)
	    for (size_t i = 0; i + 1 < ctx.positionals.size(); ++i) {
	      std::string file_arg(ctx.positionals[i]);
	      if (contains_wildcard(file_arg)) {
	        auto glob_result = glob_expand(file_arg);
	        if (glob_result.expanded) {
	          for (const auto& file : glob_result.files) {
	            cfg.sources.push_back(wstring_to_utf8(file));
	          }
	          continue;
	        }
	      }
	      cfg.sources.push_back(file_arg);
	    }
	    // Push destination as-is (literal)
	    if (!ctx.positionals.empty()) {
	      cfg.sources.push_back(std::string(ctx.positionals.back()));
	    }
	  }

	  if (cfg.sources.empty()) {
	    return core::pipeline::unexpected("missing file operand");
	  }

	  // Validate: -t requires target directory to exist (unless -D)
	  if (!cfg.target_dir.empty() && !cfg.create_leading_dirs) {
	    DWORD attrs = GetFileAttributesA(cfg.target_dir.c_str());
	    if (attrs == INVALID_FILE_ATTRIBUTES ||
	        !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
	      safeErrorPrint("install: target '" + cfg.target_dir +
	                     "' is not a directory\n");
	      return core::pipeline::unexpected("target is not a directory");
	    }
	  }

	  // Validate: -t and -T conflict
	  if (!cfg.target_dir.empty() && cfg.no_target_directory) {
	    return core::pipeline::unexpected(
	        "cannot combine --target-directory (-t) and --no-target-directory (-T)");
	  }

	  return cfg;
}

auto run(const Config& cfg) -> int {
  if (cfg.directory_mode) {
    for (const auto& dir : cfg.sources) {
      if (cfg.verbose) {
        safePrint("install: creating directory '");
        safePrint(dir);
        safePrintLn("'");
      }
      // Create directory with parents if needed
      std::error_code ec;
      std::filesystem::create_directories(
          std::filesystem::path(utf8_to_wstring(dir)), ec);
      if (ec) {
        safePrint("install: cannot create directory '");
        safePrint(dir);
        safePrintLn("'");
        return 1;
      }
    }
    return 0;
  }

  if (cfg.sources.size() < 2 && cfg.target_dir.empty()) {
    return 1;
  }

  // Determine the target directory
  SmallVector<std::string, 64> sources;
  std::string target;
  bool target_is_dir = false;

  if (!cfg.target_dir.empty()) {
    // -t DIR mode: all sources are file sources, target_dir is the directory
    for (size_t i = 0; i < cfg.sources.size(); ++i) {
      sources.push_back(cfg.sources[i]);
    }
    target = cfg.target_dir;
    // Create target directory if -D is set
    if (cfg.create_leading_dirs) {
      std::error_code ec;
      std::filesystem::create_directories(
          std::filesystem::path(utf8_to_wstring(target)), ec);
    }
    DWORD attrs = GetFileAttributesA(target.c_str());
    target_is_dir = (attrs != INVALID_FILE_ATTRIBUTES) &&
                    (attrs & FILE_ATTRIBUTE_DIRECTORY);
  } else {
    // SOURCE... DEST mode
    for (size_t i = 0; i + 1 < cfg.sources.size(); ++i) {
      sources.push_back(cfg.sources[i]);
    }
    target = cfg.sources.back();
    DWORD attrs = GetFileAttributesA(target.c_str());
    target_is_dir = !cfg.no_target_directory &&
                    (attrs != INVALID_FILE_ATTRIBUTES) &&
                    (attrs & FILE_ATTRIBUTE_DIRECTORY);
    if (!target_is_dir && sources.size() > 1) {
      safeErrorPrint("install: target '" + target +
                     "' is not a directory\n");
      return 1;
    }
  }

  if (cfg.no_target_directory && sources.size() > 1) {
    safePrintLn("install: too many sources for -T/--no-target-directory");
    return 1;
  }

  for (const auto& source : sources) {
    std::string dest = target;
    if (target_is_dir || sources.size() > 1) {
      size_t last_slash = source.find_last_of("/\\");
      std::string filename = (last_slash != std::string::npos)
                                 ? source.substr(last_slash + 1)
                                 : source;
      if (!dest.empty() && dest.back() != '\\' && dest.back() != '/') {
        dest += "\\";
      }
      dest += filename;
    }

    if (cfg.create_leading_dirs) {
      std::filesystem::path dest_path(utf8_to_wstring(dest));
      auto parent = dest_path.parent_path();
      if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
      }
    }

    // Compare mode: skip if destination exists and content is identical
    if (cfg.compare) {
      std::ifstream src_file(source, std::ios::binary);
      std::ifstream dst_file(dest, std::ios::binary);
      if (src_file && dst_file) {
        std::string src_content((std::istreambuf_iterator<char>(src_file)),
                                 std::istreambuf_iterator<char>());
        std::string dst_content((std::istreambuf_iterator<char>(dst_file)),
                                 std::istreambuf_iterator<char>());
        if (src_content == dst_content) {
          continue;  // Skip identical files
        }
      }
    }

    if (cfg.backup) {
      DWORD dest_attrs = GetFileAttributesA(dest.c_str());
      if (dest_attrs != INVALID_FILE_ATTRIBUTES) {
        std::string backup_path = dest + cfg.backup_suffix;
        MoveFileExA(dest.c_str(), backup_path.c_str(),
                    MOVEFILE_REPLACE_EXISTING);
        if (cfg.verbose) {
          safePrint("created backup: ");
          safePrintLn(backup_path);
        }
      }
    }

    if (cfg.verbose) {
      safePrint("installing: ");
      safePrint(source);
      safePrint(" -> ");
      safePrintLn(dest);
    }

    if (!CopyFileA(source.c_str(), dest.c_str(), FALSE)) {
      safePrint("install: cannot copy '");
      safePrint(source);
      safePrint("' to '");
      safePrint(dest);
      safePrintLn("'");
      return 1;
    }

    // Preserve timestamps if -p
    if (cfg.preserve_timestamps) {
      HANDLE hSrc = CreateFileA(source.c_str(), FILE_READ_ATTRIBUTES,
                                 FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL, nullptr);
      if (hSrc != INVALID_HANDLE_VALUE) {
        FILETIME ct, la, lm;
        if (GetFileTime(hSrc, &ct, &la, &lm)) {
          HANDLE hDst = CreateFileA(dest.c_str(), FILE_WRITE_ATTRIBUTES,
                                     FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
          if (hDst != INVALID_HANDLE_VALUE) {
            SetFileTime(hDst, nullptr, &la, &lm);
            CloseHandle(hDst);
          }
        }
        CloseHandle(hSrc);
      }
    }
  }

  return 0;
}

}  // namespace install_pipeline

REGISTER_COMMAND(
    install, "install",
    "install [OPTION]... [-T] SOURCE DEST\n"
    "  install [OPTION]... SOURCE... DIRECTORY\n"
    "  install [OPTION]... -t DIRECTORY SOURCE...",
    "Copy files and set attributes.\n"
    "\n"
    "Note: This is a simplified Windows implementation.\n"
    "Advanced features like mode, owner, group, strip, and SELinux\n"
    "context handling are tracked but not fully supported on Windows.",
    "  install source.txt dest.txt\n"
    "  install -b file.txt backup/\n"
    "  install -v src/*.txt /target/\n"
    "  install -d /tmp/dir",
    "cp(1), mv(1)", "WinuxCmd", "Copyright ? 2026 WinuxCmd", INSTALL_OPTIONS) {
  using namespace install_pipeline;

  auto cfg_result = build_config(ctx);
  if (!cfg_result) {
    cp::report_error(cfg_result, L"install");
    return 1;
  }

  return run(*cfg_result);
}
