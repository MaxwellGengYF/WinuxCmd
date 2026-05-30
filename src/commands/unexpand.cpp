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
 *  - File: unexpand.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implementation for unexpand.
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

auto constexpr UNEXPAND_OPTIONS = std::array{
    OPTION("-t", "--tabs", "specify tab stop positions (default: 8)",
           STRING_TYPE),
    OPTION("-a", "--all", "convert all spaces, not just leading ones",
           BOOL_TYPE),
    OPTION("", "--first-only", "convert only leading sequences of blanks",
           BOOL_TYPE)
};

namespace unexpand_pipeline {
namespace cp = core::pipeline;

struct Config {
  struct TabStops {
    enum class RepeatMode { every_multiple, after_last, none };
    SmallVector<size_t, 16> stops;
    size_t interval = 8;
    RepeatMode repeat_mode = RepeatMode::every_multiple;
  };

  TabStops tab_stops;
  bool all_spaces = false;
  SmallVector<std::string, 64> files;
};

auto build_config(const CommandContext<UNEXPAND_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;
  cfg.all_spaces = ctx.get<bool>("--all", false) || ctx.get<bool>("-a", false);

  auto tabs_opt = ctx.get<std::string>("--tabs", "");
  if (tabs_opt.empty()) {
    tabs_opt = ctx.get<std::string>("-t", "");
  }

  bool has_tabs_opt = !tabs_opt.empty();
  auto parse_tab_stops = [](const std::string& spec) -> cp::Result<Config::TabStops> {
    Config::TabStops ts;
    if (spec.empty()) return ts;
    std::string normalized = spec;
    for (char& c : normalized) if (c == ',') c = ' ';
    std::istringstream input(normalized);
    std::vector<std::string> tokens;
    for (std::string token; input >> token;) tokens.push_back(token);
    if (tokens.empty()) return cp::unexpected("invalid tab stops");
    auto parse_positive = [](std::string_view value) -> std::optional<size_t> {
      if (value.empty()) return std::nullopt;
      size_t parsed = 0;
      auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed);
      if (ec != std::errc() || ptr != value.data() + value.size() || parsed == 0) return std::nullopt;
      return parsed;
    };
    bool repeat_specified = false;
    for (size_t i = 0; i < tokens.size(); ++i) {
      const std::string& token = tokens[i];
      if (token[0] == '/' || token[0] == '+') {
        if (i + 1 != tokens.size()) return cp::unexpected("repeat tab stop must be last");
        auto interval = parse_positive(std::string_view(token).substr(1));
        if (!interval) return cp::unexpected("invalid tab stop");
        ts.interval = *interval;
        ts.repeat_mode = token[0] == '/' ? Config::TabStops::RepeatMode::every_multiple : Config::TabStops::RepeatMode::after_last;
        repeat_specified = true;
        continue;
      }
      auto stop = parse_positive(token);
      if (!stop) return cp::unexpected("invalid tab stop");
      if (!ts.stops.empty() && *stop <= ts.stops.back()) return cp::unexpected("tab stops must be increasing");
      ts.stops.push_back(*stop);
    }
    if (ts.stops.size() == 1 && !repeat_specified && ts.repeat_mode == Config::TabStops::RepeatMode::every_multiple) {
      ts.interval = ts.stops.front();
      ts.stops.clear();
    } else if (ts.stops.size() > 1 && ts.repeat_mode == Config::TabStops::RepeatMode::every_multiple) {
      ts.repeat_mode = Config::TabStops::RepeatMode::none;
    }
    return ts;
  };

  if (has_tabs_opt) {
    auto parsed = parse_tab_stops(tabs_opt);
    if (!parsed) return core::pipeline::unexpected(parsed.error());
    cfg.tab_stops = *parsed;
    cfg.all_spaces = true;
  }

  bool first_only = ctx.get<bool>("--first-only", false);
  if (first_only && cfg.all_spaces) {
    cfg.all_spaces = false;
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

auto next_tab_stop(size_t column, const Config::TabStops& tab_stops) -> size_t {
  for (size_t stop : tab_stops.stops) {
    if (stop > column) return stop;
  }
  switch (tab_stops.repeat_mode) {
    case Config::TabStops::RepeatMode::every_multiple:
      return ((column / tab_stops.interval) + 1) * tab_stops.interval;
    case Config::TabStops::RepeatMode::after_last: {
      size_t anchor = tab_stops.stops.empty() ? 0 : tab_stops.stops.back();
      if (column < anchor) return anchor;
      return anchor + (((column - anchor) / tab_stops.interval) + 1) * tab_stops.interval;
    }
    case Config::TabStops::RepeatMode::none:
      return column + 1;
  }
  return column + 1;
}

// Convert spaces to tabs in a single line
auto unexpand_line(const std::string& line, const Config::TabStops& tab_stops, bool all_spaces)
    -> std::string {
  std::string result;
  result.reserve(line.size());
  size_t pos = 0;
  size_t column = 0;
  bool before_non_blank = true;

  while (pos < line.size()) {
    if (line[pos] == ' ') {
      // Count consecutive spaces
      size_t space_start = pos;
      size_t space_start_col = column;
      while (pos < line.size() && line[pos] == ' ') {
        pos++;
        column++;
      }
      size_t space_count = pos - space_start;

      // Convert spaces to tabs if possible
      bool convert_to_tabs = all_spaces || before_non_blank;

      if (convert_to_tabs && space_count >= 1) {
        std::string converted;
        size_t col = space_start_col;
        size_t spaces_remaining = space_count;
        while (spaces_remaining > 0) {
          size_t next_stop = next_tab_stop(col, tab_stops);
          size_t dist = next_stop - col;
          if (dist <= spaces_remaining) {
            converted += '\t';
            spaces_remaining -= dist;
            col = next_stop;
          } else {
            break;
          }
        }
        if (spaces_remaining < space_count) {
          // We converted at least one tab
          result += converted;
          result.append(spaces_remaining, ' ');
        } else {
          result.append(space_count, ' ');
        }
      } else {
        result.append(space_count, ' ');
      }
    } else {
      result += line[pos];
      if (line[pos] == '\t') {
        column = next_tab_stop(column, tab_stops);
      } else {
        column++;
      }
      if (line[pos] != ' ') before_non_blank = false;
      pos++;
    }
  }

  return result;
}

auto run(const Config& cfg) -> int {
  bool all_ok = true;

  for (const auto& file : cfg.files) {
    std::string content;

    if (file == "-") {
      // Read from stdin
      content.assign(std::istreambuf_iterator<char>(std::cin),
                     std::istreambuf_iterator<char>());
    } else {
      // Read from file
      std::ifstream f(file, std::ios::binary);
      if (!f) {
        auto err = std::string("cannot open '") + file + "' for reading";
        cp::Result<int> result = core::pipeline::unexpected(std::string_view(err));
        cp::report_error(result, L"unexpand");
        all_ok = false;
        continue;
      }
      content.assign(std::istreambuf_iterator<char>(f),
                     std::istreambuf_iterator<char>());
      if (f.fail() && !f.eof()) {
        cp::Result<int> result = core::pipeline::unexpected("error reading from file");
        cp::report_error(result, L"unexpand");
        all_ok = false;
        continue;
      }
      // Skip UTF-8 BOM if present at the beginning
      if (content.size() >= 3 &&
          static_cast<unsigned char>(content[0]) == 0xEF &&
          static_cast<unsigned char>(content[1]) == 0xBB &&
          static_cast<unsigned char>(content[2]) == 0xBF) {
        content = content.substr(3);
      }
    }

    // Process line by line to maintain line breaks
    std::string result;
    size_t line_start = 0;
    while (line_start < content.size()) {
      size_t line_end = content.find('\n', line_start);
      std::string line;

      if (line_end == std::string::npos) {
        line = content.substr(line_start);
        result += unexpand_line(line, cfg.tab_stops, cfg.all_spaces);
        break;
      } else {
        line = content.substr(line_start,
                              line_end - line_start + 1);  // Include newline
        result += unexpand_line(line, cfg.tab_stops, cfg.all_spaces);
        line_start = line_end + 1;
      }
    }

    safePrint(result);
  }

  return all_ok ? 0 : 1;
}

}  // namespace unexpand_pipeline

REGISTER_COMMAND(
    unexpand, "unexpand", "unexpand [OPTION]... [FILE]...",
    "Convert spaces to tabs.\n"
    "\n"
    "Convert spaces to tabs. By default, only convert leading spaces to tabs.\n"
    "Use -a to convert all spaces.\n"
    "\n"
    "Note: This implementation supports basic space-to-tab conversion.\n"
    "Advanced features like comma-separated tab positions are not implemented.",
    "  unexpand file.txt\n"
    "  unexpand -t 4 file.txt\n"
    "  unexpand -a file.txt\n"
    "  echo 'hello world' | unexpand",
    "expand(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd", UNEXPAND_OPTIONS) {
  using namespace unexpand_pipeline;

  auto cfg_result = build_config(ctx);
  if (!cfg_result) {
    cp::report_error(cfg_result, L"unexpand");
    return 1;
  }

  return run(*cfg_result);
}
