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
 *  - File: nl.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implementation for nl.
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright © 2026 WinuxCmd
// include other header after pch.h
#include "core/command_macros.h"

#include "../core/core.h"
#include "../utils/utils.h"
#include "../container/container.h"

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

auto constexpr NL_OPTIONS = std::array{
    OPTION("-b", "--body-numbering", "use STYLE for numbering body lines",
           STRING_TYPE),
    OPTION("-h", "--header-numbering", "use STYLE for numbering header lines",
           STRING_TYPE),
    OPTION("-f", "--footer-numbering", "use STYLE for numbering footer lines",
           STRING_TYPE),
    OPTION("-i", "line-increment", "line number increment at each line",
           STRING_TYPE),
    OPTION("-s", "number-separator", "add STRING after (possible) line number",
           STRING_TYPE),
    OPTION("-v", "starting-line-number",
           "first line number on each logical page", STRING_TYPE),
    OPTION("-w", "line-number-width", "width of line numbers", STRING_TYPE),
    OPTION("-n", "--number-format", "use FORMAT for line numbers",
           STRING_TYPE),
    OPTION("-p", "--no-renumber", "do not reset line numbers at logical pages"),
    OPTION("-d", "--section-delimiter", "use DELIM for section delimiters",
           STRING_TYPE)
};

namespace nl_pipeline {
namespace cp = core::pipeline;

struct Config {
  std::string body_numbering = "t";  // t: non-empty, a: all, n: none
  std::string header_numbering = "n";  // Default: no header numbering
  std::string footer_numbering = "n";  // Default: no footer numbering
  std::string number_format = "rn";    // rn: right, ln: left, rz: right-zero
  int line_increment = 1;
  std::string separator = "\t";
  int starting_number = 1;
  int number_width = 6;
  bool no_renumber = false;
  std::string section_delimiter;
  SmallVector<std::string, 64> files;
};

auto build_config(const CommandContext<NL_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;

  auto body_opt = ctx.get<std::string>("--body-numbering", "");
  if (body_opt.empty()) {
    body_opt = ctx.get<std::string>("-b", "");
  }
  if (!body_opt.empty()) {
    cfg.body_numbering = body_opt;
    if (cfg.body_numbering != "t" && cfg.body_numbering != "a" &&
        cfg.body_numbering != "n" &&
        cfg.body_numbering != "pRE" &&  // regexp based
        cfg.body_numbering != "p") {
      return core::pipeline::unexpected("invalid body numbering style");
    }
  }

  auto header_opt = ctx.get<std::string>("--header-numbering", "");
  if (header_opt.empty()) {
    header_opt = ctx.get<std::string>("-h", "");
  }
  if (!header_opt.empty()) {
    cfg.header_numbering = header_opt;
  }

  auto footer_opt = ctx.get<std::string>("--footer-numbering", "");
  if (footer_opt.empty()) {
    footer_opt = ctx.get<std::string>("-f", "");
  }
  if (!footer_opt.empty()) {
    cfg.footer_numbering = footer_opt;
  }

  auto increment_opt = ctx.get<std::string>("--line-increment", "");
  if (increment_opt.empty()) {
    increment_opt = ctx.get<std::string>("-i", "");
  }
  if (!increment_opt.empty()) {
    try {
      cfg.line_increment = std::stoi(increment_opt);
      // GNU nl allows negative increments
    } catch (...) {
      return core::pipeline::unexpected("invalid line increment");
    }
  }

  auto separator_opt = ctx.get<std::string>("--number-separator", "");
  if (separator_opt.empty()) {
    separator_opt = ctx.get<std::string>("-s", "");
  }
  if (!separator_opt.empty()) {
    cfg.separator = separator_opt;
  }

  auto start_opt = ctx.get<std::string>("--starting-line-number", "");
  if (start_opt.empty()) {
    start_opt = ctx.get<std::string>("-v", "");
  }
  if (!start_opt.empty()) {
    try {
      cfg.starting_number = std::stoi(start_opt);
    } catch (...) {
      return core::pipeline::unexpected("invalid starting line number");
    }
  }

  auto width_opt = ctx.get<std::string>("--line-number-width", "");
  if (width_opt.empty()) {
    width_opt = ctx.get<std::string>("-w", "");
  }
  if (!width_opt.empty()) {
    try {
      cfg.number_width = std::stoi(width_opt);
      if (cfg.number_width <= 0) {
        return core::pipeline::unexpected("line number width must be positive");
      }
    } catch (...) {
      return core::pipeline::unexpected("invalid line number width");
    }
  }

  auto format_opt = ctx.get<std::string>("--number-format", "");
  if (format_opt.empty()) {
    format_opt = ctx.get<std::string>("-n", "");
  }
  if (!format_opt.empty()) {
    cfg.number_format = format_opt;
  }

  cfg.no_renumber = ctx.get<bool>("--no-renumber", false) ||
                    ctx.get<bool>("-p", false);

  auto delim_opt = ctx.get<std::string>("--section-delimiter", "");
  if (delim_opt.empty()) {
    delim_opt = ctx.get<std::string>("-d", "");
  }
  if (!delim_opt.empty()) {
    cfg.section_delimiter = delim_opt;
  }

  for (auto arg : ctx.positionals) {
    std::string file_arg(arg);
    if (contains_wildcard(file_arg)) {
      auto glob_result = glob_expand(file_arg);
      if (glob_result.expanded) {
        for (const auto& file : glob_result.files) {
          cfg.files.push_back(wstring_to_utf8(file));
        }
        continue;
      }
    }
    cfg.files.push_back(file_arg);
  }

  if (cfg.files.empty()) {
    cfg.files.push_back("-");
  }

  return cfg;
}

auto run(const Config& cfg) -> int {
  int line_number = cfg.starting_number;

  for (const auto& file : cfg.files) {
    if (file == "-") {
      // Read from stdin
      std::string line;
      while (std::getline(std::cin, line)) {
        bool should_number = false;

        if (cfg.body_numbering == "a") {
          // Number all lines
          should_number = true;
        } else if (cfg.body_numbering == "t") {
          // Number only non-empty lines
          should_number = !line.empty();
        }
        // cfg.body_numbering == "n" - don't number

        if (should_number) {
          // Format line number with specified width
          char num_buf[32];
          snprintf(num_buf, sizeof(num_buf), "%*d", cfg.number_width,
                   line_number);
          safePrint(num_buf);
          safePrint(cfg.separator);
          safePrintLn(line);
          line_number += cfg.line_increment;
        } else {
          safePrintLn(line);
        }
      }
    } else {
      // Read from file
      std::ifstream f(file, std::ios::binary);
      if (!f) {
        auto err = std::string("cannot open '") + file + "' for reading";
        cp::Result<int> result = core::pipeline::unexpected(std::string_view(err));
        cp::report_error(result, L"nl");
        return 1;
      }

      bool first_line = true;
      std::string line;
      while (std::getline(f, line)) {
        // Skip UTF-8 BOM if present at the beginning of the first line
        if (first_line && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
          line = line.substr(3);
        }
        first_line = false;

        bool should_number = false;

        if (cfg.body_numbering == "a") {
          should_number = true;
        } else if (cfg.body_numbering == "t") {
          should_number = !line.empty();
        }

        if (should_number) {
          char num_buf[32];
          snprintf(num_buf, sizeof(num_buf), "%*d", cfg.number_width,
                   line_number);
          safePrint(num_buf);
          safePrint(cfg.separator);
          safePrintLn(line);
          line_number += cfg.line_increment;
        } else {
          safePrintLn(line);
        }
      }

      if (f.fail() && !f.eof()) {
        cp::Result<int> result = core::pipeline::unexpected("error reading from file");
        cp::report_error(result, L"nl");
        return 1;
      }
    }
  }

  return 0;
}

}  // namespace nl_pipeline

REGISTER_COMMAND(
    nl, "nl", "nl [OPTION]... [FILE]...",
    "Number lines of files.\n"
    "\n"
    "Write each FILE to standard output, with line numbers added.\n"
    "With no FILE, or when FILE is -, read standard input.\n"
    "\n"
    "Mandatory arguments to long options are mandatory for short options too.\n"
    "\n"
    "Note: This implementation supports basic numbering.\n"
    "Advanced features like section delimiters are not implemented.",
    "  nl file.txt\n"
    "  nl -b a file.txt          # number all lines\n"
    "  nl -i 5 file.txt          # increment by 5\n"
    "  nl -s ': ' file.txt       # use custom separator\n"
    "  nl -v 10 file.txt         # start from 10\n"
    "  nl -w 3 file.txt         # 3-digit numbers",
    "cat(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd", NL_OPTIONS) {
  using namespace nl_pipeline;

  auto cfg_result = build_config(ctx);
  if (!cfg_result) {
    cp::report_error(cfg_result, L"nl");
    return 1;
  }

  return run(*cfg_result);
}
