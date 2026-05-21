/*
 *  Copyright © 2026 [caomengxuan666]
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
 *  - File: xargs.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - @contributor1 caomengxuan666 2507560089@qq.com
///   - @contributor2 <email2@example.com>
///   - @contributor3 <email3@example.com>
/// @Description: Implementation for xargs.
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright © 2026 WinuxCmd

#include "core/command_macros.h"
#include "pch/pch.h"

#pragma comment(lib, "advapi32.lib")
import std;
import core;
import utils;
import container;

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

/**
 * @brief XARGS command options definition
 *
 * This array defines all the options supported by the xargs command.
 * Each option is described with its short form, long form, and description.
 * The implementation status is also indicated for each option.
 *
 * @par Options:
 * - @a -n, --max-args: Use at most max-args arguments per command line
 * [IMPLEMENTED]
 * - @a -I: Replace occurrences of replace-str in the initial-arguments with
 * names [IMPLEMENTED]
 * - @a -0, --null: Input items are terminated by a null character [IMPLEMENTED]
 * - @a -d, --delimiter: Input items are terminated by the specified character
 * [IMPLEMENTED]
 * - @a -t, --verbose: Print the command line on the standard error before
 * executing it [IMPLEMENTED]
 * - @a -r, --no-run-if-empty: If the standard input does not contain any
 * nonblanks, do not run the command [IMPLEMENTED]
 * - @a -P, --max-procs: Run up to max-procs processes at a time [IMPLEMENTED]
 * - @a -s, --max-chars: Use at most max-chars chars per command line
 * [IMPLEMENTED]
 * - @a -x, --exit: Exit if the size (see -s) is exceeded [IMPLEMENTED]
 */
auto constexpr XARGS_OPTIONS = std::array{
    OPTION("-n", "--max-args",
           "use at most max-args arguments per command line", INT_TYPE),
    OPTION("-I", "",
           "replace occurrences of replace-str in the initial-arguments with "
           "names",
           STRING_TYPE),
    OPTION("-0", "--null", "input items are terminated by a null character"),
    OPTION("-d", "--delimiter",
           "input items are terminated by the specified character",
           STRING_TYPE),
    OPTION("-t", "--verbose",
           "print the command line on the standard error before executing it"),
    OPTION("-r", "--no-run-if-empty",
           "if the standard input does not contain any nonblanks, do not run "
           "the command"),
    OPTION("-P", "--max-procs", "run up to max-procs processes at a time",
           INT_TYPE),
    OPTION("-s", "--max-chars", "use at most max-chars chars per command line",
           INT_TYPE),
    OPTION("-x", "--exit", "exit if the size (see -s) is exceeded")};

namespace xargs_pipeline {
namespace cp = core::pipeline;

/**
 * @brief Parse arguments from stdin
 * @param delimiter Delimiter character (default: space/newline)
 * @param replace_str Replacement string for -I option
 * @return Vector of parsed arguments
 */
auto parse_delimiter(std::string_view text) -> cp::Result<char> {
  if (text.empty()) return std::unexpected("delimiter must not be empty");
  if (text == "\\0") return '\0';
  if (text == "\\n") return '\n';
  if (text == "\\r") return '\r';
  if (text == "\\t") return '\t';
  if (text.size() == 1) return text[0];
  return std::unexpected("delimiter must be a single character");
}

auto parse_arguments(char delimiter, bool split_blanks)
    -> std::vector<std::string> {
  SmallVector<std::string, 256> args;
  std::string arg;

  char c;
  while (std::cin.get(c)) {
    bool is_separator = c == delimiter;
    if (split_blanks && (c == '\n' || c == '\r' || c == ' ' || c == '\t')) {
      is_separator = true;
    }

    if (is_separator) {
      if (!arg.empty()) {
        args.push_back(arg);
        arg.clear();
      }
    } else {
      arg += c;
    }
  }

  // Add last argument if not empty
  if (!arg.empty()) {
    args.push_back(arg);
  }

  return std::vector<std::string>(args.begin(), args.end());
}

auto materialize_arguments(const std::vector<std::string> &base_args,
                           const std::vector<std::string> &input_args,
                           const std::string &replace_str)
    -> std::vector<std::string> {
  SmallVector<std::string, 256> all_args;

  for (const auto &base_arg : base_args) {
    if (!replace_str.empty() &&
        base_arg.find(replace_str) != std::string::npos) {
      std::string replaced = base_arg;
      std::string replacement;
      for (size_t j = 0; j < input_args.size(); ++j) {
        if (j > 0) replacement += " ";
        replacement += input_args[j];
      }

      size_t pos = 0;
      while ((pos = replaced.find(replace_str, pos)) != std::string::npos) {
        replaced.replace(pos, replace_str.length(), replacement);
        pos += replacement.length();
      }
      all_args.push_back(replaced);
    } else {
      all_args.push_back(base_arg);
    }
  }

  if (replace_str.empty()) {
    for (const auto &input_arg : input_args) {
      all_args.push_back(input_arg);
    }
  }

  return std::vector<std::string>(all_args.begin(), all_args.end());
}

auto quote_arg(const std::wstring &arg) -> std::wstring {
  if (arg.empty()) return L"\"\"";

  bool need_quote = arg.find_first_of(L" \t\"") != std::wstring::npos;
  if (!need_quote) return arg;

  std::wstring out = L"\"";
  size_t backslashes = 0;
  for (wchar_t c : arg) {
    if (c == L'\\') {
      ++backslashes;
    } else if (c == L'"') {
      out.append(backslashes * 2 + 1, L'\\');
      out.push_back(L'"');
      backslashes = 0;
    } else {
      out.append(backslashes, L'\\');
      backslashes = 0;
      out.push_back(c);
    }
  }
  out.append(backslashes * 2, L'\\');
  out.push_back(L'"');
  return out;
}

auto build_command_line(const std::string &command,
                        const std::vector<std::string> &args) -> std::wstring {
  std::wstring cmd_line = quote_arg(utf8_to_wstring(command));
  for (const auto &arg : args) {
    cmd_line.push_back(L' ');
    cmd_line += quote_arg(utf8_to_wstring(arg));
  }
  return cmd_line;
}

struct ChildProcess {
  PROCESS_INFORMATION pi{};
  DWORD exit_code = 0;
};

auto launch_process(const std::string &command,
                    const std::vector<std::string> &args)
    -> cp::Result<ChildProcess> {
  auto cmd_line = build_command_line(command, args);
  STARTUPINFOW si = {sizeof(si)};
  PROCESS_INFORMATION pi{};

  BOOL success =
      CreateProcessW(nullptr, cmd_line.data(), nullptr, nullptr, TRUE,
                     CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si, &pi);
  if (!success) {
    return std::unexpected("failed to execute '" + command + "'");
  }

  ChildProcess child;
  child.pi = pi;
  return child;
}

auto wait_child(ChildProcess &child) -> int {
  WaitForSingleObject(child.pi.hProcess, INFINITE);
  DWORD result = 0;
  GetExitCodeProcess(child.pi.hProcess, &result);
  CloseHandle(child.pi.hProcess);
  CloseHandle(child.pi.hThread);
  child.exit_code = result;
  return static_cast<int>(result);
}

/**
 * @brief Execute command with arguments
 * @param command Command to execute
 * @param base_args Base arguments from command line
 * @param input_args Arguments from stdin
 * @param replace_str Replacement string
 * @param max_args Maximum arguments per execution
 * @param verbose Print command before execution
 * @return Exit code
 */
auto execute_command(const std::string &command,
                     const std::vector<std::string> &base_args,
                     const std::vector<std::string> &input_args,
                     const std::string &replace_str, int max_args,
                     int max_chars, bool exit_if_exceeded, int max_procs,
                     bool verbose) -> int {
  int exit_code = 0;

  if (max_args <= 0) {
    max_args = static_cast<int>(input_args.size());
  }
  if (!replace_str.empty()) max_args = 1;
  if (max_args <= 0) max_args = 1;
  if (max_procs <= 0) {
    max_procs = static_cast<int>(std::thread::hardware_concurrency());
    if (max_procs <= 0) max_procs = 1;
  }

  std::vector<ChildProcess> running;
  running.reserve(static_cast<size_t>(max_procs));

  auto wait_one = [&]() {
    if (running.empty()) return;
    int child_status = wait_child(running.front());
    if (child_status != 0) exit_code = child_status;
    running.erase(running.begin());
  };

  SmallVector<std::string, 256> batch;
  batch.reserve(static_cast<size_t>(std::max(max_args, 1)));

  auto fits_limits = [&](const std::vector<std::string> &candidate) -> bool {
    if (max_args > 0 && static_cast<int>(candidate.size()) > max_args) {
      return false;
    }
    if (max_chars > 0) {
      auto materialized =
          materialize_arguments(base_args, candidate, replace_str);
      auto cmd_line = build_command_line(command, materialized);
      if (cmd_line.size() > static_cast<size_t>(max_chars)) {
        return false;
      }
    }
    return true;
  };

  auto run_batch = [&](const std::vector<std::string> &current_batch) -> bool {
    if (current_batch.empty()) return true;

    auto all_args =
        materialize_arguments(base_args, current_batch, replace_str);
    if (verbose) {
      safeErrorPrint(command);
      for (const auto &arg : all_args) {
        safeErrorPrint(" ");
        safeErrorPrint(arg);
      }
      safeErrorPrint("\n");
    }

    auto child = launch_process(command, all_args);
    if (!child) {
      safeErrorPrint("xargs: failed to execute '");
      safeErrorPrint(command);
      safeErrorPrint("'\n");
      exit_code = 127;
      return true;
    }

    running.push_back(*child);
    if (running.size() >= static_cast<size_t>(max_procs)) {
      wait_one();
    }
    return true;
  };

  auto flush_batch = [&]() -> bool {
    if (batch.empty()) return true;
    bool ok = run_batch(std::vector<std::string>(batch.begin(), batch.end()));
    batch.clear();
    return ok;
  };

  for (const auto &input_arg : input_args) {
    batch.push_back(input_arg);
    if (fits_limits(std::vector<std::string>(batch.begin(), batch.end()))) {
      continue;
    }

    batch.pop_back();
    if (!flush_batch()) {
      return exit_code == 0 ? 1 : exit_code;
    }

    batch.push_back(input_arg);
    auto current_batch = std::vector<std::string>(batch.begin(), batch.end());
    if (!fits_limits(current_batch)) {
      if (exit_if_exceeded) {
        cp::report_custom_error(L"xargs", L"command line length exceeded");
        return 1;
      }
    }
  }

  if (!flush_batch()) {
    return exit_code == 0 ? 1 : exit_code;
  }

  while (!running.empty()) wait_one();
  return exit_code;
}

}  // namespace xargs_pipeline

REGISTER_COMMAND(
    xargs, "xargs", "build and execute command lines from standard input",
    "Build and execute command lines from standard input.\n"
    "\n"
    "Items are separated by blanks. The result command line is executed\n"
    "after each group of max-args items is read.",
    "  find . -name '*.cpp' | xargs rm -f     Delete all cpp files\n"
    "  echo file1 file2 | xargs cat         Concatenate files\n"
    "  find . -name '*.txt' | xargs -n1 grep 'pattern'  Search one file at a "
    "time",
    "find(1), grep(1), sed(1)", "caomengxuan666", "Copyright © 2026 WinuxCmd",
    XARGS_OPTIONS) {
  using namespace xargs_pipeline;

  bool use_null = ctx.get<bool>("-0", false) || ctx.get<bool>("--null", false);
  bool verbose =
      ctx.get<bool>("-t", false) || ctx.get<bool>("--verbose", false);
  bool no_run_if_empty =
      ctx.get<bool>("-r", false) || ctx.get<bool>("--no-run-if-empty", false);
  int max_args = ctx.get<int>("-n", 0);
  int max_procs = ctx.get<int>("--max-procs", 1);
  if (max_procs == 1) max_procs = ctx.get<int>("-P", 1);
  int max_chars = ctx.get<int>("--max-chars", 0);
  if (max_chars == 0) max_chars = ctx.get<int>("-s", 0);
  bool exit_if_exceeded =
      ctx.get<bool>("--exit", false) || ctx.get<bool>("-x", false);
  if (max_procs < 0) {
    cp::report_custom_error(L"xargs", L"max-procs must be non-negative");
    return 1;
  }
  if (max_chars < 0) {
    cp::report_custom_error(L"xargs", L"max-chars must be non-negative");
    return 1;
  }
  std::string replace_str = ctx.get<std::string>("-I", "");
  std::string delimiter_arg = ctx.get<std::string>("--delimiter", "");
  if (delimiter_arg.empty()) delimiter_arg = ctx.get<std::string>("-d", "");

  char delimiter = use_null ? '\0' : ' ';
  bool split_blanks = !use_null && replace_str.empty();
  if (!delimiter_arg.empty()) {
    auto parsed_delim = parse_delimiter(delimiter_arg);
    if (!parsed_delim) {
      cp::report_error(parsed_delim, L"xargs");
      return 1;
    }
    delimiter = *parsed_delim;
    split_blanks = false;
  } else if (!replace_str.empty() && !use_null) {
    delimiter = '\n';
    split_blanks = false;
  }

  // Get command to execute (first positional arg)
  if (ctx.positionals.empty()) {
    // Default to echo if no command specified
    auto input_args_vec = parse_arguments(delimiter, split_blanks);
    SmallVector<std::string, 256> input_args(input_args_vec.begin(),
                                             input_args_vec.end());

    if (no_run_if_empty && input_args.empty()) {
      return 0;
    }

    if (verbose) {
      safeErrorPrint("echo");
      for (const auto &arg : input_args) {
        safeErrorPrint(" ");
        safeErrorPrint(arg);
      }
      safeErrorPrint("\n");
    }

    for (const auto &arg : input_args) {
      safePrint(arg);
      safePrint(" ");
    }
    safePrint("\n");
    return 0;
  }

  std::string command = std::string(ctx.positionals[0]);
  SmallVector<std::string, 32> base_args;

  for (size_t i = 1; i < ctx.positionals.size(); ++i) {
    base_args.push_back(std::string(ctx.positionals[i]));
  }

  // Parse arguments from stdin
  auto input_args_vec = parse_arguments(delimiter, split_blanks);
  SmallVector<std::string, 256> input_args(input_args_vec.begin(),
                                           input_args_vec.end());

  // If no input arguments, check if we should skip execution
  if (input_args.empty()) {
    // Skip if -r (no-run-if-empty) is specified
    if (no_run_if_empty) {
      return 0;
    }
    // Skip if -I is specified but there's nothing to replace
    if (!replace_str.empty()) {
      return 0;
    }
    // If no input args and no base args, skip execution
    if (base_args.empty()) {
      return 0;
    }
  }

  // Execute command with arguments - convert SmallVector to std::vector for
  // compatibility
  std::vector<std::string> base_args_vec(base_args.begin(), base_args.end());
  return execute_command(
      command, base_args_vec,
      std::vector<std::string>(input_args.begin(), input_args.end()),
      replace_str, max_args, max_chars, exit_if_exceeded, max_procs, verbose);
}
