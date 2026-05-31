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
 *  - File: fmt.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implementation for fmt.
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright © 2026 WinuxCmd
// include other header after pch.h
#include "core/command_macros.h"

#include "../core/core.h"
#include "../utils/utils.h"
#include "../container/container.h"

#include <cctype>
#include <sstream>

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

auto constexpr FMT_OPTIONS = std::array{
    OPTION("-c", "--crown-margin",
           "preserve indentation of the first two lines", BOOL_TYPE),
    OPTION("-p", "--prefix", "reformat only lines beginning with STRING",
           STRING_TYPE),
    OPTION("-s", "--split-only", "split long lines, but do not refill",
           BOOL_TYPE),
    OPTION("-t", "--tagged-paragraph", "expect indentation in first 2 lines",
           BOOL_TYPE),
    OPTION("-u", "--uniform-spacing",
           "one space between words, two after sentences", BOOL_TYPE),
    OPTION("-w", "--width", "maximum line width (default 75)", STRING_TYPE),
    OPTION("-g", "--goal", "goal width (default 93% of width)", STRING_TYPE)};

namespace fmt_pipeline {
namespace cp = core::pipeline;

struct Config {
  bool crown_margin = false;
  bool split_only = false;
  bool tagged_paragraph = false;
  bool uniform_spacing = false;
  int width = 75;
  int goal = 0;
  std::string prefix;
  SmallVector<std::string, 64> files;
};

auto build_config(const CommandContext<FMT_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;
  cfg.crown_margin =
      ctx.get<bool>("--crown-margin", false) || ctx.get<bool>("-c", false);
  cfg.split_only =
      ctx.get<bool>("--split-only", false) || ctx.get<bool>("-s", false);
  cfg.tagged_paragraph =
      ctx.get<bool>("--tagged-paragraph", false) || ctx.get<bool>("-t", false);
  cfg.uniform_spacing =
      ctx.get<bool>("--uniform-spacing", false) || ctx.get<bool>("-u", false);

  auto prefix_opt = ctx.get<std::string>("--prefix", "");
  if (prefix_opt.empty()) {
    prefix_opt = ctx.get<std::string>("-p", "");
  }
  cfg.prefix = prefix_opt;

  auto width_opt = ctx.get<std::string>("--width", "");
  if (width_opt.empty()) {
    width_opt = ctx.get<std::string>("-w", "");
  }
  if (!width_opt.empty()) {
    if (!std::all_of(width_opt.begin(), width_opt.end(),
                     [](unsigned char c) { return std::isdigit(c); })) {
      return core::pipeline::unexpected("invalid width value");
    }
    try {
      cfg.width = std::stoi(width_opt);
    } catch (...) {
      return core::pipeline::unexpected("invalid width value");
    }
  }

  auto goal_opt = ctx.get<std::string>("--goal", "");
  if (goal_opt.empty()) {
    goal_opt = ctx.get<std::string>("-g", "");
  }
  if (!goal_opt.empty()) {
    try {
      cfg.goal = std::stoi(goal_opt);
    } catch (...) {
      return core::pipeline::unexpected("invalid goal value");
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

  if (cfg.files.empty()) {
    cfg.files.push_back("-");
  }

  return cfg;
}

auto read_input(const std::string& filename) -> cp::Result<std::string> {
  std::string content;

  if (filename == "-") {
    std::string line;
    while (std::getline(std::cin, line)) {
      content += line + "\n";
    }
  } else {
    std::ifstream f(filename, std::ios::binary);
    if (!f) {
      return core::pipeline::unexpected(std::string("cannot open '") + filename +
                             "' for reading");
    }
    content.assign(std::istreambuf_iterator<char>(f),
                   std::istreambuf_iterator<char>());
    if (f.fail() && !f.eof()) {
      return core::pipeline::unexpected("error reading from file");
    }
    if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
      content = content.substr(3);
    }
  }

  return content;
}

auto count_leading_spaces(const std::string& line) -> size_t {
  size_t i = 0;
  while (i < line.size() && line[i] == ' ') ++i;
  return i;
}

auto format_words(const std::vector<std::string>& words, int goal_width,
                  int max_width, const std::string& indent,
                  bool uniform_spacing) -> std::vector<std::string> {
  std::vector<std::string> lines;
  if (words.empty()) return lines;

  std::string current = indent + words[0];
  for (size_t i = 1; i < words.size(); ++i) {
    const std::string& prev_word = words[i - 1];
    const std::string& next_word = words[i];

    bool ends_sentence = !prev_word.empty() &&
        (prev_word.back() == '.' || prev_word.back() == '!' || prev_word.back() == '?');
    std::string gap = (uniform_spacing && ends_sentence) ? "  " : " ";

    // Check against goal width for breaking; never exceed max width
    int total_with_next = static_cast<int>(current.size() + gap.size() + next_word.size());
    if (total_with_next > goal_width && !current.empty()) {
      lines.push_back(current);
      current = indent + next_word;
    } else {
      // If it exceeds max width, we must break anyway (shouldn't happen with normal goal < max)
      if (total_with_next > max_width && !current.empty()) {
        lines.push_back(current);
        current = indent + next_word;
      } else {
        current += gap + next_word;
      }
    }
  }
  lines.push_back(current);
  return lines;
}

auto split_line_at_words(const std::string& line, int max_width,
                         bool uniform_spacing) -> std::vector<std::string> {
  std::vector<std::string> result;
  std::istringstream iss(line);
  std::vector<std::string> words;
  std::string word;
  while (iss >> word) {
    words.push_back(word);
  }

  if (words.empty()) {
    result.push_back(line);
    return result;
  }

  std::string current = words[0];
  for (size_t i = 1; i < words.size(); ++i) {
    const std::string& prev_word = words[i - 1];
    const std::string& next_word = words[i];

    bool ends_sentence = !prev_word.empty() &&
        (prev_word.back() == '.' || prev_word.back() == '!' || prev_word.back() == '?');
    std::string gap = (uniform_spacing && ends_sentence) ? "  " : " ";

    if (static_cast<int>(current.size() + gap.size() + next_word.size()) > max_width) {
      result.push_back(current);
      current = next_word;
    } else {
      current += gap + next_word;
    }
  }
  result.push_back(current);
  return result;
}

auto run(const Config& cfg) -> int {
  int effective_goal = (cfg.goal > 0) ? cfg.goal : (cfg.width * 93 / 100);

  for (const auto& file : cfg.files) {
    auto content_result = read_input(file);
    if (!content_result) {
      cp::report_error(content_result, L"fmt");
      return 1;
    }

    const std::string& content = *content_result;
    std::vector<std::string> lines;
    {
      std::istringstream iss(content);
      std::string line;
      while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') {
          line.pop_back();
        }
        lines.push_back(line);
      }
    }

    if (cfg.split_only) {
      for (const auto& line : lines) {
        if (static_cast<int>(line.length()) <= cfg.width) {
          safePrintLn(line);
        } else {
          auto split = split_line_at_words(line, cfg.width, cfg.uniform_spacing);
          for (const auto& sl : split) {
            safePrintLn(sl);
          }
        }
      }
      continue;
    }

    size_t i = 0;
    while (i < lines.size()) {
      // Output blank lines as-is
      while (i < lines.size() && lines[i].empty()) {
        safePrintLn("");
        ++i;
      }
      if (i >= lines.size()) break;

      // In prefix mode, collect consecutive prefix lines into a paragraph
      // Non-prefix lines are passed through individually
      if (!cfg.prefix.empty()) {
        if (lines[i].compare(0, cfg.prefix.size(), cfg.prefix) == 0) {
          // Collect consecutive prefix lines
          std::vector<std::string> paragraph;
          while (i < lines.size() && !lines[i].empty() &&
                 lines[i].compare(0, cfg.prefix.size(), cfg.prefix) == 0) {
            paragraph.push_back(lines[i]);
            ++i;
          }

          // Extract indentation and words
          std::string indent = cfg.prefix;
          std::vector<std::string> words;

          for (size_t j = 0; j < paragraph.size(); ++j) {
            std::string line_content = paragraph[j].substr(cfg.prefix.size());
            // Strip one leading space after prefix if present
            if (!line_content.empty() && line_content[0] == ' ') {
              line_content = line_content.substr(1);
            }
            std::istringstream iss(line_content);
            std::string w;
            while (iss >> w) words.push_back(w);
          }

          auto formatted = format_words(words, effective_goal, cfg.width,
                                        indent, cfg.uniform_spacing);
          for (const auto& fl : formatted) {
            safePrintLn(fl);
          }
        } else {
          // Non-prefix line: pass through
          safePrintLn(lines[i]);
          ++i;
        }
        continue;
      }

      // Normal paragraph formatting (no prefix)
      std::vector<std::string> paragraph;
      while (i < lines.size() && !lines[i].empty()) {
        paragraph.push_back(lines[i]);
        ++i;
      }
      if (paragraph.empty()) continue;

      if (cfg.crown_margin) {
        size_t first_spaces = count_leading_spaces(paragraph[0]);
        std::string first_indent = paragraph[0].substr(0, first_spaces);
        std::string first_content = paragraph[0].substr(first_spaces);

        std::string second_indent = first_indent;
        if (paragraph.size() >= 2) {
          size_t second_spaces = count_leading_spaces(paragraph[1]);
          second_indent = paragraph[1].substr(0, second_spaces);
        }

        std::vector<std::string> words;
        {
          std::istringstream iss(first_content);
          std::string w;
          while (iss >> w) words.push_back(w);
        }
        for (size_t j = 1; j < paragraph.size(); ++j) {
          size_t spaces = count_leading_spaces(paragraph[j]);
          std::string content = paragraph[j].substr(spaces);
          std::istringstream iss(content);
          std::string w;
          while (iss >> w) words.push_back(w);
        }

        // Use second_indent for formatting so continuation lines fit properly;
        // then restore first_indent on the first line.
        auto formatted = format_words(words, effective_goal, cfg.width,
                                      second_indent, cfg.uniform_spacing);
        for (size_t j = 0; j < formatted.size(); ++j) {
          if (j == 0) {
            if (formatted[j].compare(0, second_indent.size(), second_indent) == 0) {
              formatted[j] = first_indent + formatted[j].substr(second_indent.size());
            }
          }
          safePrintLn(formatted[j]);
        }
      } else {
        size_t spaces = count_leading_spaces(paragraph[0]);
        std::string indent = paragraph[0].substr(0, spaces);
        std::string content = paragraph[0].substr(spaces);

        std::vector<std::string> words;
        {
          std::istringstream iss(content);
          std::string w;
          while (iss >> w) words.push_back(w);
        }
        for (size_t j = 1; j < paragraph.size(); ++j) {
          size_t s = count_leading_spaces(paragraph[j]);
          std::string c = paragraph[j].substr(s);
          std::istringstream iss(c);
          std::string w;
          while (iss >> w) words.push_back(w);
        }

        auto formatted = format_words(words, effective_goal, cfg.width,
                                      indent, cfg.uniform_spacing);
        for (const auto& fl : formatted) {
          safePrintLn(fl);
        }
      }
    }
  }

  return 0;
}

}  // namespace fmt_pipeline

REGISTER_COMMAND(
    fmt, "fmt", "fmt [OPTION]... [FILE]...",
    "Reformat paragraphs.\n"
    "\n"
    "With no FILE, or when FILE is -, read standard input.\n"
    "\n"
    "Note: This is a simplified implementation. Advanced features\n"
    "like crown margin and tagged paragraphs are not fully supported.",
    "  fmt file.txt\n"
    "  fmt -w 60 file.txt\n"
    "  fmt -s file.txt",
    "fold(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd", FMT_OPTIONS) {
  using namespace fmt_pipeline;

  auto cfg_result = build_config(ctx);
  if (!cfg_result) {
    cp::report_error(cfg_result, L"fmt");
    return 1;
  }

  return run(*cfg_result);
}
