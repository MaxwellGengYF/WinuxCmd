/*
 *  Copyright © 2026 [caomengxuan666]
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 *
 *  - File: sort.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */

/// @Author: WinuxCmd
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implemention for sort.
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

auto constexpr SORT_OPTIONS = std::array{
    OPTION("-b", "--ignore-leading-blanks", "ignore leading blanks"),
    OPTION("-d", "--dictionary-order",
           "consider only blanks and alphanumeric characters"),
    OPTION("-f", "--ignore-case", "fold lower case to upper case"),
    OPTION("-g", "--general-numeric-sort",
           "compare according to general numerical value"),
    OPTION("-h", "--human-numeric-sort",
           "compare human readable numbers (e.g., 1K, 2M)"),
    OPTION("-i", "--ignore-nonprinting", "consider only printable characters"),
    OPTION("-M", "--month-sort", "compare as month names"),
    OPTION("-m", "--merge", "merge already sorted files"),
    OPTION("-n", "--numeric-sort", "compare according to string numerical value"),
    OPTION("-V", "--version-sort", "compare version numbers naturally"),
    OPTION("-R", "--random-sort", "shuffle by random hash of keys"),
    OPTION("-r", "--reverse", "reverse the result of comparisons"),
    OPTION("-s", "--stable", "stabilize sort by disabling last-resort comparison"),
    OPTION("-u", "--unique", "output only the first of equal runs"),
    OPTION("-z", "--zero-terminated", "line delimiter is NUL, not newline"),
    OPTION("-c", "--check", "check for sorted input; do not sort", BOOL_TYPE),
    OPTION("-C", "--check-quiet", "like -c, but do not report first bad line",
           BOOL_TYPE),
    OPTION("-o", "--output", "write result to FILE instead of standard output",
           STRING_TYPE),
    OPTION("-S", "--buffer-size", "specify amount of memory to use", STRING_TYPE),
    OPTION("-T", "--temporary-directory",
           "use DIR for temporaries, not $TMPDIR or /tmp", STRING_TYPE),
    OPTION("", "--sort", "set sort order", STRING_TYPE),
    OPTION("", "--batch-size", "merge at most NMERGE inputs at once", STRING_TYPE),
    OPTION("", "--parallel", "change the number of sorts run concurrently",
           STRING_TYPE),
    OPTION("", "--compress-program", "compress temporaries with PROG", STRING_TYPE),
    OPTION("", "--random-source", "get random bytes from FILE", STRING_TYPE),
    OPTION("", "--files0-from",
           "read input from the files specified by NUL-terminated names",
           STRING_TYPE),
    OPTION("-t", "--field-separator",
           "use SEP instead of non-blank to blank transition", STRING_TYPE),
    OPTION("-k", "--key", "sort via a key; KEYDEF has form F[.C][,F[.C]][opts]",
           STRING_TYPE),
    OPTION("", "--debug", "annotate the part of the line used to sort", BOOL_TYPE)};

namespace sort_pipeline {
namespace cp = core::pipeline;

struct KeyDef {
  size_t start_field = 1;
  size_t start_char = 1;
  size_t end_field = 0;
  size_t end_char = 0;
  bool ignore_blanks = false;
  bool dictionary = false;
  bool ignore_case = false;
  bool general_numeric = false;
  bool human_numeric = false;
  bool ignore_nonprinting = false;
  bool month_sort = false;
  bool numeric = false;
  bool reverse = false;
  bool version_sort = false;
};

struct Config {
  bool ignore_leading_blanks = false;
  bool ignore_case = false;
  bool numeric_sort = false;
  bool version_sort = false;
  bool human_numeric = false;
  bool general_numeric = false;
  bool reverse = false;
  bool unique = false;
  bool stable = false;
  bool dictionary = false;
  bool ignore_nonprinting = false;
  bool month_sort = false;
  bool random_sort = false;
  bool merge = false;
  bool check = false;
  bool check_quiet = false;
  bool debug = false;
  char delimiter = '\n';
  std::optional<char> field_separator;
  std::string output_file;
  std::string random_source;
  std::vector<KeyDef> keys;
  SmallVector<std::string, 64> files{};
};

auto read_all(std::istream& in) -> std::string { return read_text_stream(in); }

auto read_source(std::string_view path) -> cp::Result<std::string> {
  if (path == "-") return read_all(std::cin);

  std::ifstream in(std::string(path), std::ios::binary);
  if (!in.is_open()) {
    return core::pipeline::unexpected("cannot open '" + std::string(path) + "'");
  }
  return read_all(in);
}

auto split_records(std::string_view content, char delimiter)
    -> std::vector<std::string> {
  std::vector<std::string> out;
  out.reserve(content.size() / 20);
  size_t start = 0;
  for (size_t i = 0; i < content.size(); ++i) {
    if (content[i] == delimiter) {
      out.emplace_back(content.substr(start, i - start));
      start = i + 1;
    }
  }
  if (start < content.size()) {
    out.emplace_back(content.substr(start));
  }
  return out;
}

auto to_lower_ascii(std::string_view s) -> std::string {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    out.push_back(static_cast<char>(std::tolower(c)));
  }
  return out;
}

auto ltrim_ascii(std::string_view s) -> std::string_view {
  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0) {
    ++i;
  }
  return s.substr(i);
}

auto skip_blanks(std::string_view s, size_t i) -> size_t {
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0) {
    ++i;
  }
  return i;
}

struct FieldInfo {
  size_t raw_start;
  size_t raw_end;
  size_t eff_start;
};

auto find_field_whitespace(std::string_view line, size_t field_index,
                           bool ignore_blanks) -> FieldInfo {
  size_t field = 0;
  size_t i = 0;
  while (i < line.size()) {
    while (i < line.size() &&
           std::isspace(static_cast<unsigned char>(line[i])) != 0) {
      ++i;
    }
    if (i >= line.size()) break;
    size_t start = i;
    while (i < line.size() &&
           std::isspace(static_cast<unsigned char>(line[i])) == 0) {
      ++i;
    }
    size_t end = i;
    ++field;
    if (field == field_index) {
      size_t eff = ignore_blanks ? skip_blanks(line, start) : start;
      if (eff > end) eff = end;
      return {start, end, eff};
    }
  }
  return {line.size(), line.size(), line.size()};
}

auto find_field_separator(std::string_view line, size_t field_index, char sep,
                          bool ignore_blanks) -> FieldInfo {
  size_t field = 1;
  size_t start = 0;
  while (true) {
    size_t pos = line.find(sep, start);
    if (field == field_index) {
      size_t end = (pos == std::string_view::npos) ? line.size() : pos;
      size_t eff = ignore_blanks ? skip_blanks(line, start) : start;
      if (eff > end) eff = end;
      return {start, end, eff};
    }
    if (pos == std::string_view::npos) break;
    start = pos + 1;
    ++field;
  }
  return {line.size(), line.size(), line.size()};
}

auto find_field(std::string_view line, size_t field_index,
                std::optional<char> sep, bool ignore_blanks) -> FieldInfo {
  if (sep.has_value()) {
    return find_field_separator(line, field_index, *sep, ignore_blanks);
  }
  return find_field_whitespace(line, field_index, ignore_blanks);
}

auto extract_key(std::string_view line, const KeyDef& key,
                 std::optional<char> sep, bool global_ignore_blanks)
    -> std::string_view {
  bool ib = global_ignore_blanks || key.ignore_blanks;
  auto start_field = find_field(line, key.start_field, sep, ib);
  size_t start = start_field.eff_start;
  if (key.start_char > 1) {
    size_t advance = key.start_char - 1;
    start = std::min(start + advance, line.size());
  } else if (key.start_char == 0) {
    start = start_field.raw_end;
  }

  size_t end;
  if (key.end_field == 0) {
    end = line.size();
  } else {
    auto end_field_info = find_field(line, key.end_field, sep, ib);
    if (key.end_char == 0) {
      end = end_field_info.raw_end;
    } else {
      end = end_field_info.eff_start;
      size_t advance = key.end_char;
      end = std::min(end + advance, line.size());
    }
  }

  if (start > end) start = end;
  if (end > line.size()) end = line.size();
  return line.substr(start, end - start);
}

auto parse_key_spec(std::string_view text) -> cp::Result<KeyDef> {
  KeyDef key;
  if (text.empty()) return key;

  size_t pos = 0;
  auto parse_field_char = [&](size_t& field, size_t& char_pos, bool is_end) -> bool {
    if (pos >= text.size() ||
        !std::isdigit(static_cast<unsigned char>(text[pos]))) {
      return false;
    }
    size_t field_val = 0;
    while (pos < text.size() &&
           std::isdigit(static_cast<unsigned char>(text[pos]))) {
      field_val = field_val * 10 + (text[pos] - '0');
      ++pos;
    }
    if (field_val == 0) return false;
    field = field_val;

    char_pos = is_end ? 0 : 1;
    if (pos < text.size() && text[pos] == '.') {
      ++pos;
      if (pos >= text.size() ||
          !std::isdigit(static_cast<unsigned char>(text[pos]))) {
        return false;
      }
      size_t char_val = 0;
      while (pos < text.size() &&
             std::isdigit(static_cast<unsigned char>(text[pos]))) {
        char_val = char_val * 10 + (text[pos] - '0');
        ++pos;
      }
      char_pos = char_val;
    }
    return true;
  };

  if (!parse_field_char(key.start_field, key.start_char, false)) {
    return core::pipeline::unexpected("invalid key spec");
  }

  if (pos < text.size() && text[pos] == ',') {
    ++pos;
    if (!parse_field_char(key.end_field, key.end_char, true)) {
      return core::pipeline::unexpected("invalid key spec");
    }
  }

  for (; pos < text.size(); ++pos) {
    switch (text[pos]) {
      case 'b':
        key.ignore_blanks = true;
        break;
      case 'd':
        key.dictionary = true;
        break;
      case 'f':
        key.ignore_case = true;
        break;
      case 'g':
        key.general_numeric = true;
        break;
      case 'h':
        key.human_numeric = true;
        break;
      case 'i':
        key.ignore_nonprinting = true;
        break;
      case 'M':
        key.month_sort = true;
        break;
      case 'n':
        key.numeric = true;
        break;
      case 'r':
        key.reverse = true;
        break;
      case 'V':
        key.version_sort = true;
        break;
      default:
        break;
    }
  }

  return key;
}

auto parse_numeric_prefix(std::string_view s) -> std::optional<double> {
  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0) {
    ++i;
  }
  if (i >= s.size()) return std::nullopt;
  bool has_sign = false;
  if (s[i] == '-') {
    has_sign = true;
    ++i;
  }
  if (i >= s.size() ||
      !std::isdigit(static_cast<unsigned char>(s[i]))) {
    return std::nullopt;
  }
  std::string num_str;
  if (has_sign) num_str.push_back('-');
  while (i < s.size() &&
         std::isdigit(static_cast<unsigned char>(s[i]))) {
    num_str.push_back(s[i]);
    ++i;
  }
  if (i < s.size() && s[i] == '.') {
    num_str.push_back('.');
    ++i;
    while (i < s.size() &&
           std::isdigit(static_cast<unsigned char>(s[i]))) {
      num_str.push_back(s[i]);
      ++i;
    }
  }
  try {
    return std::stod(num_str);
  } catch (...) {
    return std::nullopt;
  }
}

auto parse_general_numeric_prefix(std::string_view s) -> std::optional<double> {
  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0) {
    ++i;
  }
  if (i >= s.size()) return std::nullopt;
  std::string local(std::string(s.substr(i)));
  char* end = nullptr;
  errno = 0;
  double val = std::strtod(local.c_str(), &end);
  if (end == local.c_str() || errno == ERANGE) return std::nullopt;
  return val;
}

struct HumanValue {
  bool negative = false;
  int suffix_rank = 0;
  double value = 0.0;
};

auto parse_human_value(std::string_view s) -> HumanValue {
  HumanValue hv;
  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0) {
    ++i;
  }
  if (i < s.size() && s[i] == '-') {
    hv.negative = true;
    ++i;
  } else if (i < s.size() && s[i] == '+') {
    ++i;
  }
  std::string num_str;
  while (i < s.size() &&
         (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.')) {
    num_str.push_back(s[i]);
    ++i;
  }
  if (!num_str.empty()) {
    try {
      hv.value = std::stod(num_str);
    } catch (...) {
    }
  }
  if (i < s.size()) {
    char c = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
    switch (c) {
      case 'k':
        hv.suffix_rank = 1;
        break;
      case 'm':
        hv.suffix_rank = 2;
        break;
      case 'g':
        hv.suffix_rank = 3;
        break;
      case 't':
        hv.suffix_rank = 4;
        break;
      case 'p':
        hv.suffix_rank = 5;
        break;
      case 'e':
        hv.suffix_rank = 6;
        break;
      case 'z':
        hv.suffix_rank = 7;
        break;
      case 'y':
        hv.suffix_rank = 8;
        break;
      default:
        hv.suffix_rank = -1;
        break;
    }
  }
  return hv;
}

auto compare_human(std::string_view a, std::string_view b) -> int {
  auto ha = parse_human_value(a);
  auto hb = parse_human_value(b);
  if (ha.negative != hb.negative) return ha.negative ? -1 : 1;
  if (ha.suffix_rank != hb.suffix_rank) return ha.suffix_rank < hb.suffix_rank ? -1 : 1;
  if (ha.value < hb.value) return -1;
  if (ha.value > hb.value) return 1;
  return 0;
}

auto month_rank(std::string_view s) -> int {
  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0) {
    ++i;
  }
  if (i + 3 > s.size()) return 0;
  char m[4];
  m[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
  m[1] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i + 1])));
  m[2] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i + 2])));
  m[3] = '\0';
  static const std::unordered_map<std::string, int> months = {
      {"jan", 1}, {"feb", 2}, {"mar", 3}, {"apr", 4}, {"may", 5}, {"jun", 6},
      {"jul", 7}, {"aug", 8}, {"sep", 9}, {"oct", 10}, {"nov", 11}, {"dec", 12}};
  auto it = months.find(m);
  if (it != months.end()) return it->second;
  return 0;
}

auto compare_month(std::string_view a, std::string_view b) -> int {
  int ra = month_rank(a);
  int rb = month_rank(b);
  if (ra == 0 && rb == 0) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
  }
  if (ra == 0) return -1;
  if (rb == 0) return 1;
  if (ra < rb) return -1;
  if (ra > rb) return 1;
  return 0;
}

auto compare_version_strings(std::string_view a, std::string_view b) -> int {
  size_t i = 0;
  size_t j = 0;

  auto is_digit = [](unsigned char c) { return std::isdigit(c) != 0; };

  while (i < a.size() || j < b.size()) {
    bool a_digit = i < a.size() && is_digit(static_cast<unsigned char>(a[i]));
    bool b_digit = j < b.size() && is_digit(static_cast<unsigned char>(b[j]));

    if (a_digit && b_digit) {
      size_t a_start = i;
      size_t b_start = j;
      while (i < a.size() && is_digit(static_cast<unsigned char>(a[i]))) ++i;
      while (j < b.size() && is_digit(static_cast<unsigned char>(b[j]))) ++j;

      while (a_start < i && a[a_start] == '0') ++a_start;
      while (b_start < j && b[b_start] == '0') ++b_start;

      size_t a_len = i - a_start;
      size_t b_len = j - b_start;
      if (a_len < b_len) return -1;
      if (a_len > b_len) return 1;

      for (size_t k = 0; k < a_len; ++k) {
        unsigned char ac = static_cast<unsigned char>(a[a_start + k]);
        unsigned char bc = static_cast<unsigned char>(b[b_start + k]);
        if (ac < bc) return -1;
        if (ac > bc) return 1;
      }
      continue;
    }

    if (a_digit != b_digit) {
      return a_digit ? -1 : 1;
    }

    while (i < a.size() && !is_digit(static_cast<unsigned char>(a[i])) &&
           j < b.size() && !is_digit(static_cast<unsigned char>(b[j]))) {
      unsigned char ac = static_cast<unsigned char>(a[i]);
      unsigned char bc = static_cast<unsigned char>(b[j]);
      if (ac == bc) {
        ++i;
        ++j;
        continue;
      }

      if (std::tolower(ac) < std::tolower(bc)) return -1;
      if (std::tolower(ac) > std::tolower(bc)) return 1;
      if (ac < bc) return -1;
      if (ac > bc) return 1;
    }

    if (i < a.size() && j < b.size()) {
      continue;
    }
    if (i < a.size()) return 1;
    if (j < b.size()) return -1;
  }

  return 0;
}

auto compare_strings(std::string_view a, std::string_view b, bool ignore_case,
                     bool dictionary, bool ignore_nonprinting) -> int {
  size_t i = 0, j = 0;
  while (true) {
    while (i < a.size()) {
      unsigned char c = static_cast<unsigned char>(a[i]);
      if (dictionary && !std::isalnum(c) && !std::isspace(c)) {
        ++i;
        continue;
      }
      if (ignore_nonprinting && !std::isprint(c)) {
        ++i;
        continue;
      }
      break;
    }
    while (j < b.size()) {
      unsigned char c = static_cast<unsigned char>(b[j]);
      if (dictionary && !std::isalnum(c) && !std::isspace(c)) {
        ++j;
        continue;
      }
      if (ignore_nonprinting && !std::isprint(c)) {
        ++j;
        continue;
      }
      break;
    }
    if (i >= a.size() || j >= b.size()) break;
    unsigned char ac = static_cast<unsigned char>(a[i]);
    unsigned char bc = static_cast<unsigned char>(b[j]);
    if (ignore_case) {
      ac = static_cast<unsigned char>(std::tolower(ac));
      bc = static_cast<unsigned char>(std::tolower(bc));
    }
    if (ac < bc) return -1;
    if (ac > bc) return 1;
    ++i;
    ++j;
  }
  size_t a_rem = 0, b_rem = 0;
  for (size_t k = i; k < a.size(); ++k) {
    unsigned char c = static_cast<unsigned char>(a[k]);
    if (dictionary && !std::isalnum(c) && !std::isspace(c)) continue;
    if (ignore_nonprinting && !std::isprint(c)) continue;
    ++a_rem;
  }
  for (size_t k = j; k < b.size(); ++k) {
    unsigned char c = static_cast<unsigned char>(b[k]);
    if (dictionary && !std::isalnum(c) && !std::isspace(c)) continue;
    if (ignore_nonprinting && !std::isprint(c)) continue;
    ++b_rem;
  }
  if (a_rem < b_rem) return -1;
  if (a_rem > b_rem) return 1;
  return 0;
}

auto compare_key_strings(std::string_view a, std::string_view b,
                         const KeyDef& key) -> int {
  if (key.version_sort) {
    int cmp = compare_version_strings(a, b);
    if (cmp != 0) return cmp;
  } else if (key.numeric) {
    auto na = parse_numeric_prefix(a);
    auto nb = parse_numeric_prefix(b);
    if (na.has_value() && nb.has_value()) {
      if (*na < *nb) return -1;
      if (*na > *nb) return 1;
      return 0;
    }
    if (na.has_value()) return 1;
    if (nb.has_value()) return -1;
  } else if (key.general_numeric) {
    auto na = parse_general_numeric_prefix(a);
    auto nb = parse_general_numeric_prefix(b);
    if (na.has_value() && nb.has_value()) {
      if (*na < *nb) return -1;
      if (*na > *nb) return 1;
      return 0;
    }
    if (na.has_value()) return 1;
    if (nb.has_value()) return -1;
  } else if (key.human_numeric) {
    int cmp = compare_human(a, b);
    if (cmp != 0) return cmp;
  } else if (key.month_sort) {
    int cmp = compare_month(a, b);
    if (cmp != 0) return cmp;
  } else {
    int cmp = compare_strings(a, b, key.ignore_case, key.dictionary,
                              key.ignore_nonprinting);
    if (cmp != 0) return cmp;
  }
  return 0;
}

auto compare_records(std::string_view a, std::string_view b,
                     const Config& cfg) -> int {
  if (!cfg.keys.empty()) {
    for (const auto& key : cfg.keys) {
      auto ka = extract_key(a, key, cfg.field_separator, cfg.ignore_leading_blanks);
      auto kb = extract_key(b, key, cfg.field_separator, cfg.ignore_leading_blanks);
      int cmp = compare_key_strings(ka, kb, key);
      if (cmp != 0) {
        if (key.reverse) cmp = -cmp;
        return cmp;
      }
    }
  } else {
    KeyDef global_key;
    global_key.numeric = cfg.numeric_sort;
    global_key.general_numeric = cfg.general_numeric;
    global_key.human_numeric = cfg.human_numeric;
    global_key.version_sort = cfg.version_sort;
    global_key.month_sort = cfg.month_sort;
    global_key.ignore_case = cfg.ignore_case;
    global_key.dictionary = cfg.dictionary;
    global_key.ignore_nonprinting = cfg.ignore_nonprinting;
    int cmp = compare_key_strings(a, b, global_key);
    if (cmp != 0) return cmp;
  }

  if (cfg.stable || cfg.unique || !cfg.keys.empty()) {
    return 0;
  }

  std::string left(a);
  std::string right(b);
  if (cfg.ignore_case) {
    left = to_lower_ascii(left);
    right = to_lower_ascii(right);
  }
  if (left < right) return -1;
  if (left > right) return 1;
  return 0;
}

auto compute_random_rank(std::string_view s) -> uint64_t {
  uint64_t h = 14695981039346656037ull;
  for (size_t i = s.size(); i-- > 0;) {
    h ^= static_cast<uint64_t>(static_cast<unsigned char>(s[i]));
    h *= 1099511628211ull;
  }
  return h;
}

auto is_valid_buffer_size(std::string_view s) -> bool {
  if (s.empty()) return false;
  if (s.back() == '%') {
    auto num = s.substr(0, s.size() - 1);
    if (num.empty()) return false;
    return std::all_of(num.begin(), num.end(),
                       [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; });
  }
  size_t i = 0;
  while (i < s.size() &&
         (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.')) {
    ++i;
  }
  if (i == 0) return false;
  std::string_view suffix = s.substr(i);
  if (suffix.empty()) return true;
  static const std::string valid = "bKkMmGgTtPpEeZzYy";
  for (char c : suffix) {
    if (valid.find(c) == std::string::npos) return false;
  }
  return true;
}

auto build_config(const CommandContext<SORT_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;

  cfg.ignore_leading_blanks = ctx.get<bool>("--ignore-leading-blanks", false) ||
                              ctx.get<bool>("-b", false);
  cfg.ignore_case =
      ctx.get<bool>("--ignore-case", false) || ctx.get<bool>("-f", false);
  cfg.reverse = ctx.get<bool>("--reverse", false) || ctx.get<bool>("-r", false);
  cfg.unique = ctx.get<bool>("--unique", false) || ctx.get<bool>("-u", false);
  cfg.stable = ctx.get<bool>("--stable", false) || ctx.get<bool>("-s", false);
  cfg.dictionary =
      ctx.get<bool>("--dictionary-order", false) || ctx.get<bool>("-d", false);
  cfg.ignore_nonprinting =
      ctx.get<bool>("--ignore-nonprinting", false) || ctx.get<bool>("-i", false);
  cfg.merge = ctx.get<bool>("--merge", false) || ctx.get<bool>("-m", false);
  cfg.check = ctx.get<bool>("--check", false) || ctx.get<bool>("-c", false);
  cfg.check_quiet =
      ctx.get<bool>("--check-quiet", false) || ctx.get<bool>("-C", false);
  cfg.debug = ctx.get<bool>("--debug", false);
  cfg.delimiter =
      (ctx.get<bool>("--zero-terminated", false) || ctx.get<bool>("-z", false))
          ? '\0'
          : '\n';

  cfg.output_file = ctx.get<std::string>("--output", "");
  if (cfg.output_file.empty()) cfg.output_file = ctx.get<std::string>("-o", "");

  cfg.random_source = ctx.get<std::string>("--random-source", "");

  std::string sep = ctx.get<std::string>("--field-separator", "");
  if (sep.empty()) sep = ctx.get<std::string>("-t", "");
  if (!sep.empty()) {
    if (sep == "\\0") {
      cfg.field_separator = '\0';
    } else if (sep.size() != 1) {
      return core::pipeline::unexpected(
          "field separator must be a single character");
    } else {
      cfg.field_separator = sep[0];
    }
  }

  bool has_buf = ctx.has("--buffer-size") || ctx.has("-S");
  std::string buf = ctx.get<std::string>("--buffer-size", "");
  if (buf.empty()) buf = ctx.get<std::string>("-S", "");
  if (has_buf) {
    if (!is_valid_buffer_size(buf)) {
      return core::pipeline::unexpected("invalid buffer size");
    }
  }

  // Determine ordering mode (last option wins)
  enum class SortMode {
    None,
    Numeric,
    GeneralNumeric,
    HumanNumeric,
    Version,
    Month,
    Random
  };
  SortMode final_mode = SortMode::None;

  for (auto arg : ctx.raw_args) {
    if (arg == "-n" || arg == "--numeric-sort")
      final_mode = SortMode::Numeric;
    else if (arg == "-g" || arg == "--general-numeric-sort")
      final_mode = SortMode::GeneralNumeric;
    else if (arg == "-h" || arg == "--human-numeric-sort")
      final_mode = SortMode::HumanNumeric;
    else if (arg == "-V" || arg == "--version-sort")
      final_mode = SortMode::Version;
    else if (arg == "-M" || arg == "--month-sort")
      final_mode = SortMode::Month;
    else if (arg == "-R" || arg == "--random-sort")
      final_mode = SortMode::Random;
  }

  std::string sort_mode = ctx.get<std::string>("--sort", "");
  if (!sort_mode.empty()) {
    auto lowered = to_lower_ascii(sort_mode);
    if (lowered == "numeric" || lowered == "n") {
      final_mode = SortMode::Numeric;
    } else if (lowered == "general-numeric" || lowered == "g") {
      final_mode = SortMode::GeneralNumeric;
    } else if (lowered == "human-numeric" || lowered == "h") {
      final_mode = SortMode::HumanNumeric;
    } else if (lowered == "version" || lowered == "v" ||
               lowered == "version-sort") {
      final_mode = SortMode::Version;
    } else if (lowered == "month" || lowered == "m") {
      final_mode = SortMode::Month;
    } else if (lowered == "random" || lowered == "r") {
      final_mode = SortMode::Random;
    } else {
      return core::pipeline::unexpected("invalid sort mode");
    }
  }

  switch (final_mode) {
    case SortMode::Numeric:
      cfg.numeric_sort = true;
      break;
    case SortMode::GeneralNumeric:
      cfg.general_numeric = true;
      break;
    case SortMode::HumanNumeric:
      cfg.human_numeric = true;
      break;
    case SortMode::Version:
      cfg.version_sort = true;
      break;
    case SortMode::Month:
      cfg.month_sort = true;
      break;
    case SortMode::Random:
      cfg.random_sort = true;
      break;
    default:
      break;
  }

  // Parse keys
  auto key_occurrences = ctx.string_occurrences({"-k", "--key"});
  for (const auto& occ : key_occurrences) {
    auto key = parse_key_spec(occ.value);
    if (!key) return core::pipeline::unexpected(key.error());
    cfg.keys.push_back(*key);
  }

  // Inherit global options into keys
  for (auto& key : cfg.keys) {
    key.ignore_case |= cfg.ignore_case;
    key.numeric |= cfg.numeric_sort;
    key.general_numeric |= cfg.general_numeric;
    key.human_numeric |= cfg.human_numeric;
    key.version_sort |= cfg.version_sort;
    key.month_sort |= cfg.month_sort;
    key.dictionary |= cfg.dictionary;
    key.ignore_nonprinting |= cfg.ignore_nonprinting;
  }

  // files0-from
  std::string files0 = ctx.get<std::string>("--files0-from", "");
  if (!files0.empty()) {
    auto content = read_source(files0);
    if (!content) {
      return core::pipeline::unexpected("cannot open '" + files0 + "'");
    }
    auto names = split_records(*content, '\0');
    for (auto& name : names) {
      if (!name.empty()) cfg.files.push_back(name);
    }
  }

  for (auto p : ctx.positionals) {
    std::string file_arg(p);
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

auto merge_sorted(const Config& cfg,
                  const std::vector<std::vector<std::string>>& file_lines)
    -> std::vector<std::string> {
  struct Item {
    std::string line;
    size_t file_idx;
    size_t line_idx;
  };
  auto cmp = [&](const Item& a, const Item& b) {
    int c = compare_records(a.line, b.line, cfg);
    if (cfg.reverse) return c < 0;
    return c > 0;
  };
  std::priority_queue<Item, std::vector<Item>, decltype(cmp)> pq(cmp);
  for (size_t f = 0; f < file_lines.size(); ++f) {
    if (!file_lines[f].empty()) {
      pq.push({file_lines[f][0], f, 0});
    }
  }
  std::vector<std::string> result;
  while (!pq.empty()) {
    auto item = pq.top();
    pq.pop();
    result.push_back(std::move(item.line));
    size_t f = item.file_idx;
    size_t next = item.line_idx + 1;
    if (next < file_lines[f].size()) {
      pq.push({file_lines[f][next], f, next});
    }
  }
  return result;
}

auto run(const Config& cfg) -> int {
  std::vector<std::string> records;
  for (size_t i = 0; i < cfg.files.size(); ++i) {
    auto content = read_source(cfg.files[i]);
    if (!content) {
      cp::report_error(content, L"sort");
      return 1;
    }
    auto chunk = split_records(*content, cfg.delimiter);
    for (auto& r : chunk) {
      records.push_back(std::move(r));
    }
  }

  if (cfg.check || cfg.check_quiet) {
    bool sorted = true;
    for (size_t i = 1; i < records.size(); ++i) {
      int cmp = compare_records(records[i - 1], records[i], cfg);
      if (cmp > 0 || (cfg.unique && cmp == 0)) {
        sorted = false;
        break;
      }
    }
    if (!sorted) {
      return 1;
    }
    return 0;
  }

  if (cfg.debug) {
    for (size_t i = 0; i < cfg.keys.size(); ++i) {
      std::string msg = "sort: debug: key " + std::to_string(i + 1);
      if (cfg.keys[i].numeric) msg += " numeric";
      if (cfg.keys[i].general_numeric) msg += " general-numeric";
      if (cfg.keys[i].human_numeric) msg += " human-numeric";
      if (cfg.keys[i].version_sort) msg += " version";
      if (cfg.keys[i].month_sort) msg += " month";
      if (cfg.keys[i].reverse) msg += " reverse";
      if (cfg.keys[i].ignore_case) msg += " ignore-case";
      if (cfg.keys[i].ignore_blanks) msg += " ignore-blanks";
      if (cfg.keys[i].dictionary) msg += " dictionary";
      if (cfg.keys[i].ignore_nonprinting) msg += " ignore-nonprinting";
      safeErrorPrint(msg + "\n");
    }
  }

  if (cfg.merge) {
    std::vector<std::vector<std::string>> file_lines;
    for (size_t i = 0; i < cfg.files.size(); ++i) {
      auto content = read_source(cfg.files[i]);
      if (!content) {
        cp::report_error(content, L"sort");
        return 1;
      }
      file_lines.push_back(split_records(*content, cfg.delimiter));
    }
    records = merge_sorted(cfg, file_lines);
  } else if (cfg.random_sort) {
    std::stable_sort(records.begin(), records.end(),
                     [&](const auto& a, const auto& b) {
                       std::string key_a = cfg.keys.empty()
                                               ? a
                                               : std::string(extract_key(
                                                     a, cfg.keys[0],
                                                     cfg.field_separator,
                                                     cfg.ignore_leading_blanks));
                       std::string key_b = cfg.keys.empty()
                                               ? b
                                               : std::string(extract_key(
                                                     b, cfg.keys[0],
                                                     cfg.field_separator,
                                                     cfg.ignore_leading_blanks));
                       uint64_t rank_a = compute_random_rank(key_a);
                       uint64_t rank_b = compute_random_rank(key_b);
                       if (cfg.reverse) return rank_a < rank_b;
                       return rank_a > rank_b;
                     });
  } else {
    std::stable_sort(records.begin(), records.end(),
                     [&](const auto& a, const auto& b) {
                       int cmp = compare_records(a, b, cfg);
                       if (cfg.reverse) return cmp > 0;
                       return cmp < 0;
                     });
  }

  if (cfg.unique) {
    std::vector<std::string> unique_records;
    unique_records.reserve(records.size());
    for (const auto& rec : records) {
      if (unique_records.empty()) {
        unique_records.push_back(rec);
        continue;
      }
      if (compare_records(unique_records.back(), rec, cfg) != 0) {
        unique_records.push_back(rec);
      }
    }
    records = std::move(unique_records);
  }

  std::ostream* out = &std::cout;
  std::ofstream file_out;
  if (!cfg.output_file.empty()) {
    file_out.open(cfg.output_file, std::ios::binary | std::ios::trunc);
    if (!file_out.is_open()) {
      cp::report_custom_error(L"sort", L"cannot open output file");
      return 1;
    }
    out = &file_out;
  }

  for (const auto& rec : records) {
    (*out) << rec;
    (*out) << cfg.delimiter;
  }
  out->flush();
  return 0;
}

}  // namespace sort_pipeline

REGISTER_COMMAND(sort, "sort", "sort [OPTION]... [FILE]...",
                 "Sort lines of text files.\n"
                 "With no FILE, or when FILE is -, read standard input.",
                 "  sort a.txt\n"
                 "  sort -n -r data.txt\n"
                 "  sort -u -k 1 names.txt",
                 "uniq(1), grep(1), head(1)", "WinuxCmd",
                 "Copyright © 2026 WinuxCmd", SORT_OPTIONS) {
  using namespace sort_pipeline;

  auto cfg = build_config(ctx);
  if (!cfg) {
    cp::report_error(cfg, L"sort");
    return 2;
  }

  return run(*cfg);
}
