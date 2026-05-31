/// @Author: WinuxCmd
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implemention for cut.
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright © 2026 WinuxCmd
// include other header after pch.h
#include "core/command_macros.h"

#include "../core/core.h"
#include "../utils/utils.h"

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

auto constexpr CUT_OPTIONS = std::array{
    OPTION("-b", "--bytes", "select only these bytes", STRING_TYPE),
    OPTION("-c", "--characters", "select only these characters",
           STRING_TYPE),
    OPTION("-d", "--delimiter", "use DELIM instead of TAB for field delimiter",
           STRING_TYPE),
    OPTION("-f", "--fields", "select only these fields", STRING_TYPE),
    OPTION("-s", "--only-delimited",
           "do not print lines not containing delimiter"),
    OPTION("-n", "", "do not split multibyte characters (with -b)"),
    OPTION("", "--complement", "complement the set of selected bytes, characters or fields"),
    OPTION("-O", "--output-delimiter",
           "use STRING as the output delimiter", STRING_TYPE),
    OPTION("-z", "--zero-terminated", "line delimiter is NUL, not newline")};

namespace cut_pipeline {
namespace cp = core::pipeline;

struct Range {
  int start;
  int end;  // inclusive, INT_MAX means open-ended
};

struct Config {
  char delimiter = '\t';
  bool only_delimited = false;
  bool zero_terminated = false;
  bool byte_mode = false;
  bool char_mode = false;
  bool no_split = false;
  bool complement = false;
  std::string output_delimiter;
  std::vector<Range> ranges;
  std::vector<std::string> files;
};

auto parse_range_token(std::string_view tok) -> cp::Result<Range> {
  auto pos = tok.find('-');
  if (pos == std::string_view::npos) {
    int v = 0;
    auto [ptr, ec] = std::from_chars(tok.data(), tok.data() + tok.size(), v);
    if (ec != std::errc() || ptr != tok.data() + tok.size() || v <= 0) {
      return core::pipeline::unexpected("invalid range");
    }
    return Range{v, v};
  }

  std::string_view left = tok.substr(0, pos);
  std::string_view right = tok.substr(pos + 1);

  int start = 1;
  int end = std::numeric_limits<int>::max();

  if (!left.empty()) {
    auto [ptr, ec] =
        std::from_chars(left.data(), left.data() + left.size(), start);
    if (ec != std::errc() || ptr != left.data() + left.size() || start <= 0) {
      return core::pipeline::unexpected("invalid range");
    }
  }

  if (!right.empty()) {
    auto [ptr, ec] =
        std::from_chars(right.data(), right.data() + right.size(), end);
    if (ec != std::errc() || ptr != right.data() + right.size() || end <= 0) {
      return core::pipeline::unexpected("invalid range");
    }
  }

  if (start > end) return core::pipeline::unexpected("invalid range");
  return Range{start, end};
}

auto parse_fields(std::string_view list) -> cp::Result<std::vector<Range>> {
  if (list.empty()) return core::pipeline::unexpected("missing fields list");

  std::vector<Range> ranges;
  size_t start = 0;
  while (start <= list.size()) {
    size_t pos = list.find(',', start);
    std::string_view tok = (pos == std::string_view::npos)
                               ? list.substr(start)
                               : list.substr(start, pos - start);
    auto r = parse_range_token(tok);
    if (!r) return core::pipeline::unexpected(r.error());
    ranges.push_back(*r);
    if (pos == std::string_view::npos) break;
    start = pos + 1;
  }
  return ranges;
}

auto is_selected(int index, const std::vector<Range>& ranges) -> bool {
  for (const auto& r : ranges) {
    if (index >= r.start && index <= r.end) return true;
  }
  return false;
}

// Merge overlapping or adjacent ranges and return sorted disjoint ranges
auto merge_ranges(std::vector<Range> ranges) -> std::vector<Range> {
  if (ranges.empty()) return {};
  std::sort(ranges.begin(), ranges.end(),
            [](const Range& a, const Range& b) {
              if (a.start != b.start) return a.start < b.start;
              return a.end < b.end;
            });
  std::vector<Range> merged;
  merged.push_back(ranges[0]);
  for (size_t i = 1; i < ranges.size(); ++i) {
    auto& last = merged.back();
    if (ranges[i].start <= last.end + 1) {
      if (ranges[i].end > last.end) last.end = ranges[i].end;
    } else {
      merged.push_back(ranges[i]);
    }
  }
  return merged;
}

// For complement mode, compute the inverse ranges up to max_len
auto complement_ranges(const std::vector<Range>& ranges, int max_len)
    -> std::vector<Range> {
  if (ranges.empty()) {
    std::vector<Range> result;
    for (int i = 1; i <= max_len; ++i) result.push_back({i, i});
    return result;
  }
  auto merged = merge_ranges(ranges);
  std::vector<Range> result;
  int pos = 1;
  for (const auto& r : merged) {
    if (pos < r.start) {
      result.push_back({pos, std::min(r.start - 1, max_len)});
    }
    pos = r.end + 1;
    if (pos > max_len) break;
  }
  if (pos <= max_len) {
    result.push_back({pos, max_len});
  }
  return result;
}

auto split_records(std::string_view content, char record_delim)
    -> std::vector<std::string> {
  std::vector<std::string> out;
  size_t start = 0;
  for (size_t i = 0; i < content.size(); ++i) {
    if (content[i] == record_delim) {
      out.emplace_back(content.substr(start, i - start));
      start = i + 1;
    }
  }
  if (start < content.size()) out.emplace_back(content.substr(start));
  return out;
}

auto read_source(std::string_view path) -> cp::Result<std::string> {
  if (path == "-") {
    return std::string(std::istreambuf_iterator<char>(std::cin),
                       std::istreambuf_iterator<char>());
  }
  std::ifstream in(std::string(path), std::ios::binary);
  if (!in.is_open()) {
    return core::pipeline::unexpected("cannot open '" + std::string(path) + "'");
  }
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

auto build_config(const CommandContext<CUT_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;
  // Delimiter: -d STRING (empty means NUL)
  bool has_delim = ctx.has("--delimiter") || ctx.has("-d");
  if (has_delim) {
    std::string delim = ctx.get<std::string>("--delimiter", "");
    if (delim.empty()) delim = ctx.get<std::string>("-d", "");
    if (delim.size() == 0) {
      cfg.delimiter = '\0';
    } else if (delim.size() != 1) {
      return core::pipeline::unexpected("delimiter must be one char");
    } else {
      cfg.delimiter = delim[0];
    }
  }

  cfg.only_delimited =
      ctx.get<bool>("--only-delimited", false) || ctx.get<bool>("-s", false);
  cfg.zero_terminated =
      ctx.get<bool>("--zero-terminated", false) || ctx.get<bool>("-z", false);
  cfg.no_split = ctx.get<bool>("-n", false);
  cfg.complement = ctx.get<bool>("--complement", false);

  // Output delimiter
  cfg.output_delimiter = ctx.get<std::string>("--output-delimiter", "");
  if (cfg.output_delimiter.empty()) {
    cfg.output_delimiter = ctx.get<std::string>("-O", "");
  }

  // Parse field/byte/char selection (at least one required)
  std::string fields = ctx.get<std::string>("--fields", "");
  if (fields.empty()) fields = ctx.get<std::string>("-f", "");
  std::string bytes = ctx.get<std::string>("--bytes", "");
  if (bytes.empty()) bytes = ctx.get<std::string>("-b", "");
  std::string chars = ctx.get<std::string>("--characters", "");
  if (chars.empty()) chars = ctx.get<std::string>("-c", "");

  if (!bytes.empty()) {
    cfg.byte_mode = true;
    auto br = parse_fields(bytes);
    if (!br) return core::pipeline::unexpected(br.error());
    cfg.ranges = *br;
  } else if (!chars.empty()) {
    cfg.char_mode = true;
    auto cr = parse_fields(chars);
    if (!cr) return core::pipeline::unexpected(cr.error());
    cfg.ranges = *cr;
  } else if (!fields.empty()) {
    auto ranges = parse_fields(fields);
    if (!ranges) return core::pipeline::unexpected(ranges.error());
    cfg.ranges = *ranges;
  } else {
    return core::pipeline::unexpected("you must specify a list of bytes, characters, or fields");
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
  if (cfg.files.empty()) cfg.files.emplace_back("-");

  return cfg;
}

auto cut_line(std::string_view line, const Config& cfg) -> std::string {
  std::vector<std::string_view> fields;
  size_t start = 0;
  bool has_delim = false;
  for (size_t i = 0; i < line.size(); ++i) {
    if (line[i] == cfg.delimiter) {
      has_delim = true;
      fields.emplace_back(line.substr(start, i - start));
      start = i + 1;
    }
  }
  fields.emplace_back(line.substr(start));

  if (!has_delim && cfg.only_delimited) return {};
  if (!has_delim) return std::string(line);

  std::vector<Range> effective_ranges = cfg.ranges;
  if (cfg.complement) {
    effective_ranges = complement_ranges(cfg.ranges, static_cast<int>(fields.size()));
  }

  std::string out;
  bool first = true;
  for (int idx = 1; idx <= static_cast<int>(fields.size()); ++idx) {
    if (is_selected(idx, effective_ranges)) {
      if (!first) {
        if (!cfg.output_delimiter.empty()) {
          out.append(cfg.output_delimiter);
        } else {
          out.push_back(cfg.delimiter);
        }
      }
      out.append(fields[idx - 1]);
      first = false;
    }
  }
  return out;
}

// Get the length of a UTF-8 character starting at position i
// Returns 0 if invalid/malformed
auto utf8_char_length(const std::string& s, size_t i) -> size_t {
  if (i >= s.size()) return 0;
  unsigned char c = static_cast<unsigned char>(s[i]);
  if (c < 0x80) return 1;
  if ((c & 0xE0) == 0xC0) {
    if (i + 1 < s.size() && (static_cast<unsigned char>(s[i+1]) & 0xC0) == 0x80) return 2;
    return 0;
  }
  if ((c & 0xF0) == 0xE0) {
    if (i + 2 < s.size() && (static_cast<unsigned char>(s[i+1]) & 0xC0) == 0x80 &&
        (static_cast<unsigned char>(s[i+2]) & 0xC0) == 0x80) return 3;
    return 0;
  }
  if ((c & 0xF8) == 0xF0) {
    if (i + 3 < s.size() && (static_cast<unsigned char>(s[i+1]) & 0xC0) == 0x80 &&
        (static_cast<unsigned char>(s[i+2]) & 0xC0) == 0x80 &&
        (static_cast<unsigned char>(s[i+3]) & 0xC0) == 0x80) return 4;
    return 0;
  }
  return 0;
}

// Check if all bytes [start, end] (1-based) are selected
auto all_bytes_selected(int start, int end, const std::vector<Range>& ranges) -> bool {
  for (int b = start; b <= end; ++b) {
    if (!is_selected(b, ranges)) return false;
  }
  return true;
}

auto cut_bytes_or_chars(const std::string& rec, const Config& cfg) -> std::string {
  std::string out;
  std::vector<Range> effective_ranges = cfg.ranges;
  if (cfg.complement) {
    effective_ranges = complement_ranges(cfg.ranges, static_cast<int>(rec.size()));
  }

  auto merged = merge_ranges(effective_ranges);

  if (cfg.byte_mode && cfg.no_split) {
    // UTF-8 aware: only output complete characters whose bytes are all selected
    size_t i = 0;
    while (i < rec.size()) {
      size_t char_len = utf8_char_length(rec, i);
      if (char_len == 0) char_len = 1;
      int byte_start = static_cast<int>(i) + 1;
      int byte_end = static_cast<int>(i + char_len);
      if (all_bytes_selected(byte_start, byte_end, effective_ranges)) {
        out.append(rec.substr(i, char_len));
      }
      i += char_len;
    }
  } else if (cfg.char_mode) {
    // Character mode: treat each Unicode character as one position
    std::vector<std::string> chars;
    size_t i = 0;
    while (i < rec.size()) {
      size_t char_len = utf8_char_length(rec, i);
      if (char_len == 0) char_len = 1;
      chars.push_back(rec.substr(i, char_len));
      i += char_len;
    }

    std::vector<Range> char_ranges = effective_ranges;
    if (cfg.complement) {
      char_ranges = complement_ranges(cfg.ranges, static_cast<int>(chars.size()));
    }
    auto char_merged = merge_ranges(char_ranges);

    bool first = true;
    for (const auto& r : char_merged) {
      if (!first && !cfg.output_delimiter.empty()) {
        out.append(cfg.output_delimiter);
      }
      int end = std::min(r.end, static_cast<int>(chars.size()));
      for (int i = r.start; i <= end; ++i) {
        if (i > 0 && i <= static_cast<int>(chars.size())) {
          out.append(chars[i - 1]);
        }
      }
      first = false;
    }
  } else {
    // Byte mode without -n
    bool first = true;
    for (const auto& r : merged) {
      if (!first && !cfg.output_delimiter.empty()) {
        out.append(cfg.output_delimiter);
      }
      int end = std::min(r.end, static_cast<int>(rec.size()));
      for (int i = r.start; i <= end; ++i) {
        if (i > 0 && i <= static_cast<int>(rec.size())) {
          out.push_back(rec[i - 1]);
        }
      }
      first = false;
    }
  }
  return out;
}

auto run_file(const std::string& path, const Config& cfg) -> int {
  auto content = read_source(path);
  if (!content) {
    cp::report_error(content, L"cut");
    return 1;
  }

  char record_delim = cfg.zero_terminated ? '\0' : '\n';

  std::vector<std::string> records;
  size_t start = 0;
  for (size_t i = 0; i < content->size(); ++i) {
    if ((*content)[i] == record_delim) {
      records.push_back(content->substr(start, i - start));
      start = i + 1;
    }
  }
  if (start < content->size()) {
    records.push_back(content->substr(start));
  }

  for (const auto& rec : records) {
    std::string out;
    if (cfg.byte_mode || cfg.char_mode) {
      out = cut_bytes_or_chars(rec, cfg);
    } else {
      out = cut_line(rec, cfg);
    }
    if (out.empty() && cfg.only_delimited &&
        !cfg.byte_mode && !cfg.char_mode &&
        rec.find(cfg.delimiter) == std::string::npos) {
      continue;
    }
    safePrint(out);
    if (cfg.zero_terminated) {
      safePrint(char{'\0'});
    } else {
      safePrint("\n");
    }
  }
  return 0;
}

auto run(const Config& cfg) -> int {
  for (const auto& f : cfg.files) {
    int rc = run_file(f, cfg);
    if (rc != 0) return rc;
  }
  return 0;
}

}  // namespace cut_pipeline

REGISTER_COMMAND(cut, "cut", "cut OPTION... [FILE]...",
                 "Print selected parts of lines from each FILE to standard "
                 "output.",
                 "  cut -f1,3 file.txt\n"
                 "  cut -d, -f2 data.csv\n"
                 "  cut -z -f1 -d: list.txt",
                 "paste(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd",
                 CUT_OPTIONS) {
  using namespace cut_pipeline;

  auto cfg = build_config(ctx);
  if (!cfg) {
    cp::report_error(cfg, L"cut");
    return 1;
  }

  return run(*cfg);
}
