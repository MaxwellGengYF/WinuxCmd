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
 *  - File: join.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implementation for join.
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

auto constexpr JOIN_OPTIONS = std::array{
    OPTION("-1", "", "join on this FIELD of file 1", STRING_TYPE),
    OPTION("-2", "", "join on this FIELD of file 2", STRING_TYPE),
    OPTION("-t", "", "use CHAR as input and output field separator",
           STRING_TYPE),
    OPTION("-e", "--empty", "replace missing input fields with EMPTY",
           STRING_TYPE),
    OPTION("-o", "--output", "use specified output format", STRING_TYPE),
    OPTION("-a", "", "output unpairable lines from file FILENUM",
           STRING_TYPE),
    OPTION("-j", "", "equivalent to -1 FIELD -2 FIELD", STRING_TYPE),
    OPTION("-v", "", "like -a but suppress joined output lines",
           STRING_TYPE),
    OPTION("-i", "--ignore-case", "ignore case when comparing keys"),
    OPTION("", "--header", "treat first line as header"),
    OPTION("-z", "--zero-terminated", "line delimiter is NUL, not newline")
};

namespace join_pipeline {
namespace cp = core::pipeline;

struct OutputField {
  int file_num = 0;  // 0 = join field, 1 or 2 = file number
  int field_idx = -1;  // 0-based field index, -1 = all fields
};

struct Config {
  int field1 = 1;
  int field2 = 1;
  char separator = ' ';  // Default to space (standard join uses whitespace)
  std::string empty_field;
  std::string output_format_raw;
  std::vector<OutputField> output_format;
  int unpaired_file = 0;  // bitmask: 1=file1, 2=file2
  int suppress_joined = 0;  // bitmask: 1=file1, 2=file2
  bool ignore_case = false;
  bool header = false;
  char delimiter = '\n';
  SmallVector<std::string, 64> files;
};

auto is_whitespace(char c) -> bool {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

auto split_line(const std::string& line, char sep, int field_num)
    -> std::string {
  std::string result;
  int current_field = 1;
  bool in_delim = true;

  for (size_t i = 0; i < line.size(); ++i) {
    bool is_sep = (sep == ' ') ? is_whitespace(line[i]) : (line[i] == sep);
    if (is_sep) {
      if (!in_delim) {
        current_field++;
        in_delim = true;
        if (current_field > field_num) {
          break;
        }
      }
    } else {
      in_delim = false;
      if (current_field == field_num) {
        result += line[i];
      }
    }
  }

  return result;
}

auto split_all_fields(const std::string& line, char sep)
    -> SmallVector<std::string, 64> {
  SmallVector<std::string, 64> fields;
  std::string current;
  bool in_delim = true;
  bool whitespace_mode = (sep == ' ');

  for (char c : line) {
    bool is_sep = whitespace_mode ? is_whitespace(c) : (c == sep);
    if (is_sep) {
      if (!in_delim) {
        fields.push_back(current);
        current.clear();
        in_delim = true;
      } else if (!whitespace_mode) {
        // Consecutive non-whitespace separators produce empty fields
        fields.push_back("");
      }
    } else {
      in_delim = false;
      current += c;
    }
  }

  // Add the last field
  if (!in_delim || !whitespace_mode) {
    fields.push_back(current);
  }

  return fields;
}

auto parse_output_format(const std::string& fmt) -> std::vector<OutputField> {
  std::vector<OutputField> result;
  if (fmt.empty()) return result;

  size_t pos = 0;
  while (pos < fmt.size()) {
    // Skip whitespace/commas
    while (pos < fmt.size() && (fmt[pos] == ' ' || fmt[pos] == ','))
      ++pos;
    if (pos >= fmt.size()) break;

    OutputField of;
    // Check if it's "0" (join field)
    if (fmt[pos] == '0') {
      of.file_num = 0;
      of.field_idx = -1;
      pos++;
      result.push_back(of);
      continue;
    }
    // Parse file number (1 or 2)
    if (fmt[pos] == '1' || fmt[pos] == '2') {
      of.file_num = fmt[pos] - '0';
      pos++;
      if (pos < fmt.size() && fmt[pos] == '.') {
        pos++;  // Skip dot
        // Parse field number
        size_t start = pos;
        while (pos < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[pos])))
          ++pos;
        if (pos > start) {
          of.field_idx = std::stoi(fmt.substr(start, pos - start)) - 1;
          // 1-based to 0-based
        }
      } else {
        // Just file number means all fields from that file
        of.field_idx = -1;
      }
      result.push_back(of);
    } else {
      // Invalid, skip
      pos++;
    }
  }
  return result;
}

auto build_config(const CommandContext<JOIN_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;

  auto field1_opt = ctx.get<std::string>("-1", "");
  if (!field1_opt.empty()) {
    try {
      cfg.field1 = std::stoi(field1_opt);
    } catch (...) {
      return core::pipeline::unexpected("invalid field number for -1");
    }
  }

  auto field2_opt = ctx.get<std::string>("-2", "");
  if (!field2_opt.empty()) {
    try {
      cfg.field2 = std::stoi(field2_opt);
    } catch (...) {
      return core::pipeline::unexpected("invalid field number for -2");
    }
  }

  auto j_opt = ctx.get<std::string>("-j", "");
  if (!j_opt.empty()) {
    try {
      cfg.field1 = std::stoi(j_opt);
      cfg.field2 = cfg.field1;
    } catch (...) {
      return core::pipeline::unexpected("invalid field number for -j");
    }
  }

  auto sep_opt = ctx.get<std::string>("-t", "");
  if (!sep_opt.empty()) {
    if (sep_opt.size() != 1) {
      return core::pipeline::unexpected("separator must be a single character");
    }
    cfg.separator = sep_opt[0];
  }

  auto empty_opt = ctx.get<std::string>("--empty", "");
  if (empty_opt.empty()) {
    empty_opt = ctx.get<std::string>("-e", "");
  }
  cfg.empty_field = empty_opt;

  auto output_opt = ctx.get<std::string>("--output", "");
  if (output_opt.empty()) {
    output_opt = ctx.get<std::string>("-o", "");
  }
  cfg.output_format_raw = output_opt;
  if (output_opt == "auto") {
    cfg.output_format = {};  // empty = use default format
  } else if (!output_opt.empty()) {
    cfg.output_format = parse_output_format(output_opt);
  }

  // Handle multiple -a options (bitmask)
  auto unpaired_opts = ctx.get_all<std::string>("-a");
  for (const auto& opt : unpaired_opts) {
    try {
      int f = std::stoi(opt);
      if (f == 1) cfg.unpaired_file |= 1;
      else if (f == 2) cfg.unpaired_file |= 2;
      else return core::pipeline::unexpected("invalid file number for -a");
    } catch (...) {
      return core::pipeline::unexpected("invalid file number for -a");
    }
  }

  auto suppress_opts = ctx.get_all<std::string>("-v");
  for (const auto& opt : suppress_opts) {
    try {
      int f = std::stoi(opt);
      if (f == 1) cfg.suppress_joined |= 1;
      else if (f == 2) cfg.suppress_joined |= 2;
      else return core::pipeline::unexpected("invalid file number for -v");
    } catch (...) {
      return core::pipeline::unexpected("invalid file number for -v");
    }
  }

  cfg.ignore_case = ctx.get<bool>("--ignore-case", false) ||
                    ctx.get<bool>("-i", false);
  cfg.header = ctx.get<bool>("--header", false);
  cfg.delimiter = ctx.get<bool>("--zero-terminated", false) ? '\0' : '\n';

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

  if (cfg.files.size() < 2) {
    return core::pipeline::unexpected("missing operand after '" +
                           (cfg.files.empty() ? std::string() : cfg.files[0]) +
                           "'");
  }
  if (cfg.files.size() > 2) {
    return core::pipeline::unexpected("extra operand '" + cfg.files[2] + "'");
  }

  return cfg;
}

auto read_records(const std::string& filename, char delim)
    -> cp::Result<SmallVector<std::string, 1024>> {
  SmallVector<std::string, 1024> records;

  std::string content;
  if (filename == "-") {
    content.assign(std::istreambuf_iterator<char>(std::cin),
                   std::istreambuf_iterator<char>());
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
  }

  size_t start = 0;
  for (size_t i = 0; i < content.size(); ++i) {
    if (content[i] == delim) {
      records.push_back(content.substr(start, i - start));
      start = i + 1;
    }
  }
  if (start < content.size()) {
    records.push_back(content.substr(start));
  }

  // Skip UTF-8 BOM if present at the beginning of the first record
  if (!records.empty() && records[0].size() >= 3 &&
      static_cast<unsigned char>(records[0][0]) == 0xEF &&
      static_cast<unsigned char>(records[0][1]) == 0xBB &&
      static_cast<unsigned char>(records[0][2]) == 0xBF) {
    records[0] = records[0].substr(3);
  }

  return records;
}

auto key_to_lower(std::string s) -> std::string {
  for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

auto format_output(const Config& cfg, const std::string& line1,
                   const std::string& line2, bool is_paired,
                   int max_fields1, int max_fields2) -> std::string {
  auto fields1 = split_all_fields(line1, cfg.separator);
  auto fields2 = split_all_fields(line2, cfg.separator);
  std::string empty = cfg.empty_field;

  if (!cfg.output_format.empty()) {
    std::string result;
    for (size_t i = 0; i < cfg.output_format.size(); ++i) {
      if (i > 0) result += cfg.separator;
      const auto& of = cfg.output_format[i];
      if (of.file_num == 0) {
        // Join field
        if (!line1.empty()) {
          result += split_line(line1, cfg.separator, cfg.field1);
        } else if (!line2.empty()) {
          result += split_line(line2, cfg.separator, cfg.field2);
        }
      } else if (of.file_num == 1) {
        if (of.field_idx < 0) {
          // All fields from file1
          for (size_t j = 0; j < fields1.size(); ++j) {
            if (j > 0) result += cfg.separator;
            result += fields1[j];
          }
        } else if (static_cast<size_t>(of.field_idx) < fields1.size()) {
          result += fields1[of.field_idx];
        } else {
          result += empty;
        }
      } else if (of.file_num == 2) {
        if (of.field_idx < 0) {
          for (size_t j = 0; j < fields2.size(); ++j) {
            if (j > 0) result += cfg.separator;
            result += fields2[j];
          }
        } else if (static_cast<size_t>(of.field_idx) < fields2.size()) {
          result += fields2[of.field_idx];
        } else {
          result += empty;
        }
      }
    }
    return result;
  }

  // Default format: join field + rest of line1 + rest of line2
  // For unpaired lines, fill missing fields with empty value
  std::string key;
  std::string rest1, rest2;

  if (line1.empty() && !line2.empty()) {
    key = split_line(line2, cfg.separator, cfg.field2);
    auto f2 = split_all_fields(line2, cfg.separator);
    for (size_t i = cfg.field2; i < f2.size(); ++i) {
      if (!rest2.empty()) rest2 += cfg.separator;
      rest2 += f2[i];
    }
  } else if (line2.empty() && !line1.empty()) {
    key = split_line(line1, cfg.separator, cfg.field1);
    auto f1 = split_all_fields(line1, cfg.separator);
    for (size_t i = cfg.field1; i < f1.size(); ++i) {
      if (!rest1.empty()) rest1 += cfg.separator;
      rest1 += f1[i];
    }
  } else {
    key = split_line(line1, cfg.separator, cfg.field1);
    auto f1 = split_all_fields(line1, cfg.separator);
    for (size_t i = cfg.field1; i < f1.size(); ++i) {
      if (!rest1.empty()) rest1 += cfg.separator;
      rest1 += f1[i];
    }
    auto f2 = split_all_fields(line2, cfg.separator);
    for (size_t i = cfg.field2; i < f2.size(); ++i) {
      if (!rest2.empty()) rest2 += cfg.separator;
      rest2 += f2[i];
    }
  }

  std::string result = key;
  auto f1 = split_all_fields(line1.empty() ? std::string() : line1, cfg.separator);
  auto f2 = split_all_fields(line2.empty() ? std::string() : line2, cfg.separator);
  int actual1 = line1.empty() ? 0 : std::max(0, static_cast<int>(f1.size()) - cfg.field1);
  int actual2 = line2.empty() ? 0 : std::max(0, static_cast<int>(f2.size()) - cfg.field2);

  // Determine output widths: use max_fields if set (auto mode), else use actual
  int width1 = (max_fields1 > 0) ? max_fields1 : actual1;
  int width2 = (max_fields2 > 0) ? max_fields2 : actual2;

  for (int i = 0; i < width1; ++i) {
    result += cfg.separator;
    result += (i < actual1) ? f1[cfg.field1 + i] : (cfg.empty_field.empty() ? "" : cfg.empty_field);
  }
  for (int i = 0; i < width2; ++i) {
    result += cfg.separator;
    result += (i < actual2) ? f2[cfg.field2 + i] : (cfg.empty_field.empty() ? "" : cfg.empty_field);
  }
  return result;
}

auto run(const Config& cfg) -> int {
  const std::string& file1 = cfg.files[0];
  const std::string& file2 = cfg.files[1];

  auto lines1_result = read_records(file1, cfg.delimiter);
  if (!lines1_result) {
    cp::report_error(lines1_result, L"join");
    return 1;
  }

  auto lines2_result = read_records(file2, cfg.delimiter);
  if (!lines2_result) {
    cp::report_error(lines2_result, L"join");
    return 1;
  }

  const auto& lines1 = *lines1_result;
  const auto& lines2 = *lines2_result;

  size_t start1 = cfg.header ? 1 : 0;
  size_t start2 = cfg.header ? 1 : 0;

  // If -o auto, compute max field counts for padding
  int max_fields1 = 0, max_fields2 = 0;
  if (cfg.output_format_raw == "auto") {
    if (cfg.header && start1 < lines1.size() && start2 < lines2.size()) {
      auto h1 = split_all_fields(lines1[0], cfg.separator);
      auto h2 = split_all_fields(lines2[0], cfg.separator);
      max_fields1 = std::max(0, static_cast<int>(h1.size()) - cfg.field1);
      max_fields2 = std::max(0, static_cast<int>(h2.size()) - cfg.field2);
    } else {
      std::unordered_map<std::string, size_t> file2_first;
      for (size_t i = start2; i < lines2.size(); ++i) {
        std::string k = split_line(lines2[i], cfg.separator, cfg.field2);
        if (cfg.ignore_case) k = key_to_lower(k);
        if (file2_first.find(k) == file2_first.end()) file2_first[k] = i;
      }
      for (size_t i = start1; i < lines1.size() && max_fields1 == 0; ++i) {
        std::string k = split_line(lines1[i], cfg.separator, cfg.field1);
        if (cfg.ignore_case) k = key_to_lower(k);
        auto it = file2_first.find(k);
        if (it != file2_first.end()) {
          auto f1 = split_all_fields(lines1[i], cfg.separator);
          auto f2 = split_all_fields(lines2[it->second], cfg.separator);
          max_fields1 = std::max(0, static_cast<int>(f1.size()) - cfg.field1);
          max_fields2 = std::max(0, static_cast<int>(f2.size()) - cfg.field2);
        }
      }
    }
  }

  // Output header line if --header and -o auto/default
  if (cfg.header && cfg.output_format.empty() &&
      start1 < lines1.size() && start2 < lines2.size()) {
    std::string header_line = format_output(cfg, lines1[0], lines2[0], true,
                                            max_fields1, max_fields2);
    safePrint(header_line);
    safePrint(std::string(1, cfg.delimiter));
  }

  // Build index for file2
  std::unordered_map<std::string, SmallVector<size_t, 16>> file2_index;
  for (size_t i = start2; i < lines2.size(); ++i) {
    std::string key = split_line(lines2[i], cfg.separator, cfg.field2);
    if (cfg.ignore_case) key = key_to_lower(key);
    file2_index[key].push_back(i);
  }

  // Track which file2 lines were matched (for -a/-v)
  std::vector<bool> file2_matched(lines2.size(), false);

  // Join lines
  for (size_t i = start1; i < lines1.size(); ++i) {
    const auto& line1 = lines1[i];
    std::string key = split_line(line1, cfg.separator, cfg.field1);
    std::string lookup_key = cfg.ignore_case ? key_to_lower(key) : key;

    auto it = file2_index.find(lookup_key);
    if (it != file2_index.end()) {
      // Found matches
      if ((cfg.suppress_joined & 1) == 0) {
        for (size_t idx : it->second) {
          file2_matched[idx] = true;
          safePrint(format_output(cfg, line1, lines2[idx], true,
                                   max_fields1, max_fields2));
          safePrint(std::string(1, cfg.delimiter));
        }
      } else {
        for (size_t idx : it->second) {
          file2_matched[idx] = true;
        }
      }
    } else if ((cfg.unpaired_file & 1) || (cfg.suppress_joined & 1)) {
      // Unpaired line from file1
      safePrint(format_output(cfg, line1, "", false, max_fields1, max_fields2));
      safePrint(std::string(1, cfg.delimiter));
    }
  }

  // Output unpaired lines from file2
  if ((cfg.unpaired_file & 2) || (cfg.suppress_joined & 2)) {
    for (size_t i = start2; i < lines2.size(); ++i) {
      if (!file2_matched[i]) {
        safePrint(format_output(cfg, "", lines2[i], false, max_fields1, max_fields2));
        safePrint(std::string(1, cfg.delimiter));
      }
    }
  }

  return 0;
}

}  // namespace join_pipeline

REGISTER_COMMAND(
    join, "join", "join [OPTION]... FILE1 FILE2",
    "For each pair of input lines with identical join fields, write a line to\n"
    "standard output. The default join field is the first, delimited by "
    "blanks.\n"
    "\n"
    "Note: This is a basic implementation. Advanced features like -o format\n"
    "are not fully supported.",
    "  join file1 file2\n"
    "  join -j 1 file1 file2        # join on first field\n"
    "  join -t ',' file1 file2     # use comma as separator\n"
    "  join -1 2 -2 1 file1 file2  # join on field 2 of file1 and field 1 of "
    "file2",
    "comm(1), paste(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd",
    JOIN_OPTIONS) {
  using namespace join_pipeline;

  auto cfg_result = build_config(ctx);
  if (!cfg_result) {
    cp::report_error(cfg_result, L"join");
    return 1;
  }

  return run(*cfg_result);
}
