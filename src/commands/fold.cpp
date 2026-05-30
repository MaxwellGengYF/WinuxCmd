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
 *  - File: fold.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implementation for fold.
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright © 2026 WinuxCmd
// *** SIMPLIFIED IMPLEMENTATION - Some features may not be fully supported ***
// include other header after pch.h
#include "core/command_macros.h"

#include "../core/core.h"
#include "../utils/utils.h"
#include "../container/container.h"

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

auto constexpr FOLD_OPTIONS = std::array{
    OPTION("-b", "--bytes", "count bytes rather than columns", BOOL_TYPE),
    OPTION("-c", "--characters", "count characters rather than columns", BOOL_TYPE),
    OPTION("-s", "--spaces", "break at spaces", BOOL_TYPE),
    OPTION("-w", "--width", "use WIDTH columns instead of 80", STRING_TYPE)
};

namespace fold_pipeline {
namespace cp = core::pipeline;

struct Config {
  bool count_bytes = false;
  bool break_at_spaces = false;
  int width = 80;
  SmallVector<std::string, 64> files;
};

auto build_config(const CommandContext<FOLD_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;
  cfg.count_bytes =
      ctx.get<bool>("--bytes", false) || ctx.get<bool>("-b", false);
  cfg.break_at_spaces =
      ctx.get<bool>("--spaces", false) || ctx.get<bool>("-s", false);

  // -c is a no-op since column counting is the default behavior

  auto width_opt = ctx.get<std::string>("--width", "");
  if (width_opt.empty()) {
    width_opt = ctx.get<std::string>("-w", "");
  }
  if (!width_opt.empty()) {
    try {
      size_t idx = 0;
      cfg.width = std::stoi(width_opt, &idx);
      if (idx != width_opt.size() || cfg.width <= 0) {
        return core::pipeline::unexpected("invalid width");
      }
    } catch (...) {
      return core::pipeline::unexpected("invalid width");
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

auto char_display_width(char c, size_t current_col) -> int {
  if (c == '\t') {
    return static_cast<int>(8 - (current_col % 8));
  }
  if (c == '\b') {
    return -1;
  }
  return 1;
}

// Fold a single line
auto fold_line(const std::string& line, int width, bool count_bytes,
               bool break_at_spaces) -> std::string {
  if (line.empty()) {
    return "\n";
  }

  // Check if line ends with newline
  bool has_newline = !line.empty() && line.back() == '\n';
  size_t content_end = has_newline ? line.size() - 1 : line.size();

  std::string result;
  size_t pos = 0;

  while (pos < content_end) {
    int current_length = 0;
    size_t start_pos = pos;
    size_t last_space_pos = std::string::npos;

    // Find where to break
    while (pos < content_end) {
      char c = line[pos];
      int char_width = count_bytes ? 1 : char_display_width(c, current_length);

      if (current_length + char_width > width && c != '\t') {
        // Need to break (tabs are allowed to exceed width)
        if (break_at_spaces && last_space_pos != std::string::npos &&
            last_space_pos >= start_pos) {
          // Break at last space, include the space in the output
          size_t break_after = last_space_pos + 1;
          result.append(line.substr(start_pos, break_after - start_pos));
          result += "\n";
          pos = break_after;
        } else {
          // Break at current position
          result.append(line.substr(start_pos, pos - start_pos));
          result += "\n";
        }
        current_length = 0;
        start_pos = pos;
        last_space_pos = std::string::npos;
      } else {
        if (c == ' ' || c == '\t') {
          last_space_pos = pos;
        }
        if (c == '\b' && !count_bytes && current_length > 0) {
          current_length += char_width;  // -1
          pos++;
        } else if (c == '\t') {
          current_length += char_width;
          pos++;
        } else {
          current_length += char_width;
          pos++;
        }
      }
    }

    // Add remaining text
    if (pos > start_pos) {
      result.append(line.substr(start_pos, pos - start_pos));
    }
  }

  if (has_newline) {
    result += "\n";
  }

  return result;
}

auto process_content(const std::string& content, const Config& cfg) -> std::string {
  std::string result;
  size_t start = 0;
  for (size_t i = 0; i < content.size(); ++i) {
    if (content[i] == '\n') {
      std::string line = content.substr(start, i - start + 1);  // include \n
      auto folded = fold_line(line, cfg.width, cfg.count_bytes, cfg.break_at_spaces);
      result += folded;
      start = i + 1;
    }
  }
  // Handle last line without newline
  if (start < content.size()) {
    std::string line = content.substr(start);
    auto folded = fold_line(line, cfg.width, cfg.count_bytes, cfg.break_at_spaces);
    result += folded;
  }
  return result;
}

auto run(const Config& cfg) -> int {
  bool all_ok = true;

  for (const auto& file : cfg.files) {
    std::string content;

    if (file == "-") {
      content.assign(std::istreambuf_iterator<char>(std::cin),
                     std::istreambuf_iterator<char>());
    } else {
      std::ifstream f(file, std::ios::binary);
      if (!f) {
        auto err = std::string("cannot open '") + file + "' for reading";
        cp::Result<int> result = core::pipeline::unexpected(std::string_view(err));
        cp::report_error(result, L"fold");
        all_ok = false;
        continue;
      }
      content.assign(std::istreambuf_iterator<char>(f),
                     std::istreambuf_iterator<char>());
      if (f.fail() && !f.eof()) {
        cp::Result<int> result = core::pipeline::unexpected("error reading from file");
        cp::report_error(result, L"fold");
        all_ok = false;
        continue;
      }
      // Skip UTF-8 BOM if present
      if (content.size() >= 3 &&
          static_cast<unsigned char>(content[0]) == 0xEF &&
          static_cast<unsigned char>(content[1]) == 0xBB &&
          static_cast<unsigned char>(content[2]) == 0xBF) {
        content = content.substr(3);
      }
    }

    safePrint(process_content(content, cfg));
  }

  return all_ok ? 0 : 1;
}

}  // namespace fold_pipeline

REGISTER_COMMAND(fold, "fold", "fold [OPTION]... [FILE]...",
                 "Wrap input lines to fit specified width.\n"
                 "\n"
                 "Write each FILE to standard output, wrapping input lines to "
                 "fit in width columns.\n"
                 "With no FILE, or when FILE is -, read standard input.\n"
                 "\n"
                 "Note: This implementation supports basic folding.\n"
                 "Advanced features like multi-byte character width "
                 "calculation are not implemented.",
                 "  fold file.txt\n"
                 "  fold -w 60 file.txt\n"
                 "  fold -s file.txt\n"
                 "  fold -b file.txt\n"
                 "  echo 'very long line' | fold -w 10",
                 "fmt(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd",
                 FOLD_OPTIONS) {
  using namespace fold_pipeline;

  auto cfg_result = build_config(ctx);
  if (!cfg_result) {
    cp::report_error(cfg_result, L"fold");
    return 1;
  }

  return run(*cfg_result);
}
