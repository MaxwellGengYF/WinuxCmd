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
           STRING_TYPE),
    OPTION("-l", "--join-blank-lines",
           "group NUMBER empty lines into one", STRING_TYPE)
};

namespace nl_pipeline {
namespace cp = core::pipeline;

enum class Section { BODY, HEADER, FOOTER };
enum class NumberingStyle { NON_EMPTY, ALL, NONE, PATTERN };

struct Config {
  std::string body_numbering = "t";
  std::string header_numbering = "n";
  std::string footer_numbering = "n";
  std::string number_format = "rn";
  int line_increment = 1;
  std::string separator = "\t";
  int starting_number = 1;
  int number_width = 6;
  bool no_renumber = false;
  std::string section_delimiter = "\\:";
  int join_blank_lines = 1;
  SmallVector<std::string, 64> files;
};

struct SectionConfig {
  NumberingStyle style;
  std::string pattern;
};

auto parse_numbering_style(const std::string& s) -> SectionConfig {
  if (s.empty() || s == "t") return {NumberingStyle::NON_EMPTY, ""};
  if (s == "a") return {NumberingStyle::ALL, ""};
  if (s == "n") return {NumberingStyle::NONE, ""};
  if (!s.empty() && s[0] == 'p') return {NumberingStyle::PATTERN, s.substr(1)};
  return {NumberingStyle::NON_EMPTY, ""};
}

auto format_number(int num, const std::string& format, int width) -> std::string {
  char buf[64];
  if (format == "rz") {
    // Right zero-padded
    if (num < 0) {
      snprintf(buf, sizeof(buf), "-%0*d", std::max(0, width - 1), -num);
    } else {
      snprintf(buf, sizeof(buf), "%0*d", width, num);
    }
  } else if (format == "ln") {
    // Left justified
    snprintf(buf, sizeof(buf), "%-*d", width, num);
  } else {
    // Right justified (rn)
    snprintf(buf, sizeof(buf), "%*d", width, num);
  }
  return buf;
}

auto is_section_delimiter(const std::string& line, const std::string& delim,
                          Section& out_section) -> bool {
  if (delim.size() < 1 || line.empty()) return false;
  char macro = delim[0];
  char section = (delim.size() >= 2) ? delim[1] : ':';

  // Format 1: Standard GNU - macro followed by N consecutive section chars
  if (line[0] == macro) {
    size_t count = 0;
    bool all_section = true;
    for (size_t i = 1; i < line.size(); ++i) {
      if (line[i] == section) {
        count++;
      } else {
        all_section = false;
        break;
      }
    }
    if (all_section && count > 0) {
      if (count >= 3) out_section = Section::HEADER;
      else if (count == 2) out_section = Section::BODY;
      else out_section = Section::FOOTER;
      return true;
    }
  }

  // Format 2: Repeated pattern (macro+section) - used by tests
  if (delim.size() >= 2 && line.size() >= 2 && line.size() % 2 == 0) {
    bool all_match = true;
    for (size_t i = 0; i < line.size(); i += 2) {
      if (line[i] != macro || line[i + 1] != section) {
        all_match = false;
        break;
      }
    }
    if (all_match) {
      size_t repeats = line.size() / 2;
      if (repeats >= 3) out_section = Section::HEADER;
      else if (repeats == 2) out_section = Section::BODY;
      else out_section = Section::FOOTER;
      return true;
    }
  }

  return false;
}

auto should_number_line(const std::string& line, const SectionConfig& cfg,
                        const std::regex* re) -> bool {
  switch (cfg.style) {
    case NumberingStyle::NONE:
      return false;
    case NumberingStyle::ALL:
      return true;
    case NumberingStyle::NON_EMPTY:
      return !line.empty();
    case NumberingStyle::PATTERN:
      if (re) {
        return std::regex_search(line, *re);
      }
      return false;
  }
  return false;
}

auto build_config(const CommandContext<NL_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;

  auto body_opt = ctx.get<std::string>("--body-numbering", "");
  if (body_opt.empty()) body_opt = ctx.get<std::string>("-b", "");
  if (!body_opt.empty()) cfg.body_numbering = body_opt;

  auto header_opt = ctx.get<std::string>("--header-numbering", "");
  if (header_opt.empty()) header_opt = ctx.get<std::string>("-h", "");
  if (!header_opt.empty()) cfg.header_numbering = header_opt;

  auto footer_opt = ctx.get<std::string>("--footer-numbering", "");
  if (footer_opt.empty()) footer_opt = ctx.get<std::string>("-f", "");
  if (!footer_opt.empty()) cfg.footer_numbering = footer_opt;

  auto increment_opt = ctx.get<std::string>("--line-increment", "");
  if (increment_opt.empty()) increment_opt = ctx.get<std::string>("-i", "");
  if (!increment_opt.empty()) {
    try {
      cfg.line_increment = std::stoi(increment_opt);
    } catch (...) {
      return core::pipeline::unexpected("invalid line increment");
    }
  }

  if (ctx.has("--number-separator")) {
    cfg.separator = ctx.get<std::string>("--number-separator", "");
  } else if (ctx.has("-s")) {
    cfg.separator = ctx.get<std::string>("-s", "");
  }

  auto start_opt = ctx.get<std::string>("--starting-line-number", "");
  if (start_opt.empty()) start_opt = ctx.get<std::string>("-v", "");
  if (!start_opt.empty()) {
    try {
      cfg.starting_number = std::stoi(start_opt);
    } catch (...) {
      return core::pipeline::unexpected("invalid starting line number");
    }
  }

  auto width_opt = ctx.get<std::string>("--line-number-width", "");
  if (width_opt.empty()) width_opt = ctx.get<std::string>("-w", "");
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
  if (format_opt.empty()) format_opt = ctx.get<std::string>("-n", "");
  if (!format_opt.empty()) cfg.number_format = format_opt;

  cfg.no_renumber = ctx.get<bool>("--no-renumber", false) ||
                    ctx.get<bool>("-p", false);

  auto delim_opt = ctx.get<std::string>("--section-delimiter", "");
  if (delim_opt.empty()) delim_opt = ctx.get<std::string>("-d", "");
  if (!delim_opt.empty()) {
    if (delim_opt.size() == 1) {
      cfg.section_delimiter = delim_opt + ":";
    } else {
      cfg.section_delimiter = delim_opt.substr(0, 2);
    }
  }

  auto join_opt = ctx.get<std::string>("--join-blank-lines", "");
  if (join_opt.empty()) join_opt = ctx.get<std::string>("-l", "");
  if (!join_opt.empty()) {
    try {
      cfg.join_blank_lines = std::stoi(join_opt);
      if (cfg.join_blank_lines < 1) cfg.join_blank_lines = 1;
    } catch (...) {
      return core::pipeline::unexpected("invalid join blank lines count");
    }
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

  if (cfg.files.empty()) cfg.files.push_back("-");

  return cfg;
}

auto get_section_config(const Config& cfg, Section section) -> SectionConfig {
  switch (section) {
    case Section::HEADER: return parse_numbering_style(cfg.header_numbering);
    case Section::FOOTER: return parse_numbering_style(cfg.footer_numbering);
    case Section::BODY: return parse_numbering_style(cfg.body_numbering);
  }
  return parse_numbering_style(cfg.body_numbering);
}

auto process_stream(std::istream& in, const Config& cfg) -> int {
  int line_number = cfg.starting_number;
  Section current_section = Section::BODY;
  int consecutive_empty = 0;

  SectionConfig current_cfg = get_section_config(cfg, current_section);
  std::optional<std::regex> current_re;
  if (current_cfg.style == NumberingStyle::PATTERN && !current_cfg.pattern.empty()) {
    try {
      current_re = std::regex(current_cfg.pattern);
    } catch (...) {
      current_re = std::nullopt;
    }
  }

  auto refresh_config = [&]() {
    current_cfg = get_section_config(cfg, current_section);
    current_re = std::nullopt;
    if (current_cfg.style == NumberingStyle::PATTERN && !current_cfg.pattern.empty()) {
      try {
        current_re = std::regex(current_cfg.pattern);
      } catch (...) {
        current_re = std::nullopt;
      }
    }
  };

  std::string line;
  bool first_line = true;
  while (std::getline(in, line)) {
    // Skip UTF-8 BOM if present at the beginning of the first line
    if (first_line && line.size() >= 3 &&
        static_cast<unsigned char>(line[0]) == 0xEF &&
        static_cast<unsigned char>(line[1]) == 0xBB &&
        static_cast<unsigned char>(line[2]) == 0xBF) {
      line = line.substr(3);
    }
    first_line = false;

    Section new_section = current_section;
    if (is_section_delimiter(line, cfg.section_delimiter, new_section)) {
      // Section delimiter line is output as empty line (unnumbered)
      safePrintLn("");
      if (new_section != current_section) {
        current_section = new_section;
        if (!cfg.no_renumber) {
          line_number = cfg.starting_number;
        }
        consecutive_empty = 0;
        refresh_config();
      }
      continue;
    }

    bool number_this = should_number_line(line, current_cfg,
                                          current_re.has_value() ? &*current_re : nullptr);

    if (number_this && line.empty()) {
      // Empty line: handle join-blank-lines
      consecutive_empty++;
      if (consecutive_empty == cfg.join_blank_lines) {
        // Number the last line of the group
        safePrint(format_number(line_number, cfg.number_format, cfg.number_width));
        safePrint(cfg.separator);
        safePrintLn(line);
        line_number += cfg.line_increment;
        consecutive_empty = 0;
      } else {
        // Don't number yet
        safePrint(cfg.separator);
        safePrintLn(line);
      }
    } else if (number_this) {
      consecutive_empty = 0;
      safePrint(format_number(line_number, cfg.number_format, cfg.number_width));
      safePrint(cfg.separator);
      safePrintLn(line);
      line_number += cfg.line_increment;
    } else {
      if (line.empty()) {
        consecutive_empty++;
      } else {
        consecutive_empty = 0;
      }
      safePrint(cfg.separator);
      safePrintLn(line);
    }
  }

  if (in.fail() && !in.eof()) {
    cp::Result<int> result = core::pipeline::unexpected("error reading from file");
    cp::report_error(result, L"nl");
    return 1;
  }

  return 0;
}

auto run(const Config& cfg) -> int {
  for (const auto& file : cfg.files) {
    if (file == "-") {
      int rc = process_stream(std::cin, cfg);
      if (rc != 0) return rc;
    } else {
      std::ifstream f(file, std::ios::binary);
      if (!f) {
        auto err = std::string("cannot open '") + file + "' for reading";
        cp::Result<int> result = core::pipeline::unexpected(std::string_view(err));
        cp::report_error(result, L"nl");
        return 1;
      }
      int rc = process_stream(f, cfg);
      if (rc != 0) return rc;
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
    "Mandatory arguments to long options are mandatory for short options too.\n",
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
