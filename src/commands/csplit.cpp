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
 *  - File: csplit.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implementation for csplit.
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

auto constexpr CSPLIT_OPTIONS = std::array{
    OPTION("-f", "--prefix", "use PREFIX instead of 'xx'", STRING_TYPE),
    OPTION("-b", "--suffix-format", "use sprintf FORMAT instead of %02d",
           STRING_TYPE),
    OPTION("-n", "--digits", "use specified number of digits", STRING_TYPE),
    OPTION("-k", "--keep-files", "do not remove output files on errors",
           BOOL_TYPE),
    OPTION("-s", "--quiet", "do not print counts of output file sizes",
           BOOL_TYPE),
    OPTION("-z", "--elide-empty-files", "remove empty output files", BOOL_TYPE),
    OPTION("", "--suppress-matched",
           "suppress the lines matching PATTERN", BOOL_TYPE)};

namespace csplit_pipeline {
namespace cp = core::pipeline;

struct Config {
  std::string prefix = "xx";
  std::string suffix_format = "%02d";
  int digits = 2;
  bool keep_files = false;
  bool quiet = false;
  bool elide_empty = false;
  bool suppress_matched = false;
  std::string input_file;
  SmallVector<std::string, 64> patterns;
};

auto build_config(const CommandContext<CSPLIT_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;

  auto prefix_opt = ctx.get<std::string>("--prefix", "");
  if (prefix_opt.empty()) {
    prefix_opt = ctx.get<std::string>("-f", "");
  }
  if (!prefix_opt.empty()) {
    cfg.prefix = prefix_opt;
  }

  auto suffix_opt = ctx.get<std::string>("--suffix-format", "");
  if (suffix_opt.empty()) {
    suffix_opt = ctx.get<std::string>("-b", "");
  }
  if (!suffix_opt.empty()) {
    cfg.suffix_format = suffix_opt;
  }

  auto digits_opt = ctx.get<std::string>("--digits", "");
  if (digits_opt.empty()) {
    digits_opt = ctx.get<std::string>("-n", "");
  }
  if (!digits_opt.empty()) {
    try {
      cfg.digits = std::stoi(digits_opt);
    } catch (...) {
      return core::pipeline::unexpected("invalid digits value");
    }
  }

  cfg.keep_files =
      ctx.get<bool>("--keep-files", false) || ctx.get<bool>("-k", false);
  cfg.quiet = ctx.get<bool>("--quiet", false) || ctx.get<bool>("-s", false);
	  cfg.elide_empty =
	      ctx.get<bool>("--elide-empty-files", false) || ctx.get<bool>("-z", false);
	  cfg.suppress_matched = ctx.get<bool>("--suppress-matched", false);

	  // Get input file and patterns from positionals
	  if (!ctx.positionals.empty()) {
	    std::string file_arg(ctx.positionals[0]);
	    if (contains_wildcard(file_arg)) {
	      auto glob_result = glob_expand(file_arg);
	      if (glob_result.expanded && !glob_result.files.empty()) {
	        if (glob_result.files.size() > 1) {
	          safeErrorPrint("csplit: wildcard matches multiple files; "
	                         "specify exactly one file\n");
	          return core::pipeline::unexpected("ambiguous input file");
	        }
	        cfg.input_file = wstring_to_utf8(glob_result.files[0]);
	      } else {
	        cfg.input_file = file_arg;
	      }
	    } else {
	      cfg.input_file = file_arg;
	    }

    for (size_t i = 1; i < ctx.positionals.size(); ++i) {
      cfg.patterns.push_back(std::string(ctx.positionals[i]));
    }
  }

  if (cfg.input_file.empty()) {
    return core::pipeline::unexpected("missing input file");
  }

  if (cfg.patterns.empty()) {
    return core::pipeline::unexpected("missing pattern");
  }

  return cfg;
}

auto read_lines(const std::string& filename)
    -> cp::Result<SmallVector<std::string, 1024>> {
  SmallVector<std::string, 1024> lines;

  if (filename == "-") {
    std::string line;
    while (std::getline(std::cin, line)) {
      lines.push_back(line);
    }
  } else {
    std::ifstream f(filename, std::ios::binary);
    if (!f) {
      return core::pipeline::unexpected(std::string("cannot open '") + filename +
                             "' for reading");
    }

    std::string line;
    while (std::getline(f, line)) {
      // Skip UTF-8 BOM if present at the beginning of the first line
      if (lines.empty() && line.size() >= 3 &&
          static_cast<unsigned char>(line[0]) == 0xEF &&
          static_cast<unsigned char>(line[1]) == 0xBB &&
          static_cast<unsigned char>(line[2]) == 0xBF) {
        line = line.substr(3);
      }
      lines.push_back(line);
    }

    if (f.fail() && !f.eof()) {
      return core::pipeline::unexpected("error reading from file");
    }
  }

  return lines;
}

auto match_pattern(const std::string& line, const std::string& pattern)
    -> bool {
  if (pattern.empty()) return false;

  // Check if pattern is a line number (all digits)
  bool is_number = !pattern.empty() &&
                   std::all_of(pattern.begin(), pattern.end(),
                               [](unsigned char c) { return std::isdigit(c); });
  if (is_number) return false;

  // Regex pattern: /regex/
  if (pattern.size() >= 2 && pattern[0] == '/') {
    size_t end_slash = pattern.find_last_of('/');
    if (end_slash != 0 && end_slash != std::string::npos) {
      std::string regex_str = pattern.substr(1, end_slash - 1);
      // Simple regex: support character classes [a-z], [ABC], and literals
      // For now, try literal match first, then character class
      if (line.find(regex_str) != std::string::npos) return true;
      // Check if regex_str is a character class like [ABC] or [a-z]
      if (regex_str.size() >= 3 && regex_str[0] == '[' && regex_str.back() == ']') {
        std::string cls = regex_str.substr(1, regex_str.size() - 2);
        // Check if it's a range like A-C or a-z
        if (cls.size() == 3 && cls[1] == '-') {
          char lo = cls[0], hi = cls[2];
          for (char c : line) {
            if (c >= lo && c <= hi) return true;
          }
        } else {
          // Set of characters like ABC
          for (char c : line) {
            if (cls.find(c) != std::string::npos) return true;
          }
        }
      }
    }
    return false;
  }

  return line == pattern;
}

// Parse pattern spec: may include prefix %, offset +/-N, repeat {N}
struct PatternSpec {
  std::string pattern;   // "/regex/" or line number string
  int repeat = 0;        // {N}: repeat N more times
  int offset = 0;        // +/-N: offset from match
  bool skip = false;     // % prefix: discard up to and including match
};

auto parse_pattern_spec(const std::string& raw) -> PatternSpec {
  PatternSpec spec;
  std::string s = raw;

  // Check for % prefix (skip)
  if (!s.empty() && s[0] == '%') {
    spec.skip = true;
    s = s.substr(1);
    // Also strip the closing % if present
    size_t end_pct = s.find('%');
    if (end_pct != std::string::npos) {
      // Everything after % might be an offset
      std::string after = s.substr(end_pct + 1);
      s = s.substr(0, end_pct);
      if (!after.empty()) {
        if (after[0] == '+' || after[0] == '-') {
          try { spec.offset = std::stoi(after); } catch (...) {}
        }
      }
    }
  }

  // Check for {N} repeat suffix (may appear at end of entire pattern including offset)
  size_t brace = s.rfind('{');
  if (brace != std::string::npos && s.back() == '}') {
    try {
      spec.repeat = std::stoi(s.substr(brace + 1, s.size() - brace - 2));
    } catch (...) {}
    s = s.substr(0, brace);
  }

  // Check for offset suffix after closing / or %
  if (!s.empty() && (s.back() == '+' || s.back() == '-')) {
    // No explicit offset value
    spec.offset = (s.back() == '+') ? 1 : -1;
    s.pop_back();
  } else if (s.size() > 1) {
    // Look for trailing +/-N
    size_t sign_pos = s.find_last_of("+-");
    if (sign_pos != std::string::npos && sign_pos > 0 && sign_pos < s.size() - 1) {
      if (s[sign_pos - 1] == '/' || s[sign_pos - 1] == '%') {
        try {
          spec.offset = std::stoi(s.substr(sign_pos));
        } catch (...) {}
        s = s.substr(0, sign_pos);
      }
    }
  }

  spec.pattern = s;
  return spec;
}

auto run(const Config& cfg) -> int {
  auto lines_result = read_lines(cfg.input_file);
  if (!lines_result) { cp::report_error(lines_result, L"csplit"); return 1; }
  const auto& lines = *lines_result;
  bool suppress = cfg.suppress_matched;

	  // Process patterns: {N} after a pattern modifies the previous pattern's repeat
	  struct ParsedPat { PatternSpec spec; int total_repeat; };
	  SmallVector<ParsedPat, 16> parsed;

	  for (size_t pi = 0; pi < cfg.patterns.size(); ++pi) {
	    const auto& raw = cfg.patterns[pi];
	    // Check if this is a bare {N} repeat modifier
	    if (raw.size() >= 2 && raw[0] == '{' && raw.back() == '}') {
	      if (!parsed.empty()) {
	        try {
	          int n = std::stoi(raw.substr(1, raw.size() - 2));
	          parsed.back().total_repeat += n;
	        } catch (...) {}
	      }
	      continue;
	    }
	    auto spec = parse_pattern_spec(raw);
	    int total_repeat = spec.repeat + 1;
		    parsed.push_back({spec, total_repeat});
		  }

	  // Find split points
  std::vector<size_t> split_points;
  split_points.push_back(0);
  size_t current_line = 0;
  std::set<size_t> skip_lines;  // lines to skip (suppress-matched)

  for (const auto& pp : parsed) {
    bool is_line_number = !pp.spec.pattern.empty() &&
        std::all_of(pp.spec.pattern.begin(), pp.spec.pattern.end(),
                    [](unsigned char c) { return std::isdigit(c); });

    for (int rep = 0; rep < pp.total_repeat; ++rep) {
      bool found = false;

      if (is_line_number) {
        int target = std::stoi(pp.spec.pattern);
        size_t idx = static_cast<size_t>(std::max(1, target + pp.spec.offset)) - 1;
	        if (pp.spec.skip) {
	          // Skip: discard everything up to target+offset line, then start after
	          size_t start = static_cast<size_t>(
	              std::max(0, target + pp.spec.offset));
	          current_line = std::min(start, lines.size());
	          split_points.clear();
	          split_points.push_back(current_line);
	          found = true;
        } else if (idx > current_line && idx <= lines.size()) {
          split_points.push_back(idx);
          current_line = idx;
          found = true;
        }
      } else {
        // Regex pattern
        std::string regex_str = pp.spec.pattern;
        // Strip / delimiters
        if (regex_str.size() >= 2 && regex_str[0] == '/') {
          size_t end = regex_str.find_last_of('/');
          if (end != 0 && end != std::string::npos)
            regex_str = regex_str.substr(1, end - 1);
        }

        for (size_t line_idx = current_line; line_idx < lines.size(); ++line_idx) {
          // Simple match
          bool matched = lines[line_idx].find(regex_str) != std::string::npos;
          // Also check character class
          if (!matched && regex_str.size() >= 3 && regex_str[0] == '[' && regex_str.back() == ']') {
            std::string cls = regex_str.substr(1, regex_str.size()-2);
            if (cls.size() == 3 && cls[1] == '-') {
              for (char c : lines[line_idx])
                if (c >= cls[0] && c <= cls[2]) { matched = true; break; }
            } else {
              for (char c : lines[line_idx])
                if (cls.find(c) != std::string::npos) { matched = true; break; }
            }
          }
          // Check ^ anchor
          if (!matched && !regex_str.empty() && regex_str[0] == '^') {
            std::string sub = regex_str.substr(1);
            matched = (lines[line_idx].find(sub) == 0);
          }

          if (matched) {
            int split_at = static_cast<int>(line_idx) + pp.spec.offset;
            if (split_at < 0) split_at = 0;
            size_t split_idx = static_cast<size_t>(split_at);

	            if (pp.spec.skip) {
	              // Skip: discard up to match, then start at match + offset
	              // (match line is implicitly discarded, offset adds extra lines)
	              size_t start = static_cast<size_t>(
	                  std::max(0, static_cast<int>(line_idx) + pp.spec.offset));
	              current_line = std::min(start, lines.size());
	              split_points.clear();
	              split_points.push_back(current_line);
	            } else {
	              if (suppress) {
	                // --suppress-matched: omit the matched line entirely
	                skip_lines.insert(line_idx);
	                split_points.push_back(line_idx);
	                split_points.push_back(line_idx + 1);
	                current_line = line_idx + 1;
	              } else {
	                // Default: split at match+offset (matched line goes to next file)
	                if (split_idx > current_line) {
	                  split_points.push_back(split_idx);
	                }
	                current_line = split_idx;
	              }
            }
            found = true;
            break;
          }
        }
      }

      if (!found && rep < pp.total_repeat - 1) {
        if (!cfg.quiet) safeErrorPrint("csplit: pattern not found\n");
        return 1;
      }
      if (!found) break;
    }
  }

  split_points.push_back(lines.size());

  // Collect suppressed line indices
  std::set<size_t> suppressed_lines;
  if (suppress) {
    // Find segments that are exactly 1 line (the suppressed match)
    // We'll skip them during output
  }

  // Deduplicate and sort
  std::sort(split_points.begin(), split_points.end());
  split_points.erase(std::unique(split_points.begin(), split_points.end()),
                     split_points.end());

  char filename_buf[256];
  int file_count = 0;
  for (size_t i = 0; i + 1 < split_points.size(); ++i) {
    size_t start = split_points[i];
    size_t end = split_points[i + 1];
    if (start >= end) continue;

    // Skip segments that consist entirely of suppressed lines
    bool all_suppressed = suppress;
    for (size_t li = start; li < end && all_suppressed; ++li) {
      if (!skip_lines.count(li)) all_suppressed = false;
    }
    if (all_suppressed) continue;

    snprintf(filename_buf, sizeof(filename_buf), "%s%0*d", cfg.prefix.c_str(),
             cfg.digits, file_count);
    std::string filename(filename_buf);
    std::ofstream out(filename, std::ios::binary);
    if (!out) { safeErrorPrint("csplit: cannot create '" + filename + "'\n"); return 1; }
    size_t bytes = 0;
    for (size_t li = start; li < end; ++li) {
      out << lines[li] << "\n";
      bytes += lines[li].size() + 1;
    }
    out.close();
    if (!cfg.quiet) safePrint(std::to_string(bytes) + "\n");
    file_count++;
  }
  return 0;
}

}  // namespace csplit_pipeline

REGISTER_COMMAND(
    csplit, "csplit", "csplit [OPTION]... FILE PATTERN...",
    "Output pieces of FILE separated by PATTERN(s) to files 'xx00', 'xx01', "
    "...\n"
    "\n"
    "Mandatory arguments to long options are mandatory for short options too.\n"
    "\n"
    "PATTERN is a line number, or a /regex/ pattern.\n"
    "\n"
    "Note: This is a basic implementation. Advanced pattern matching\n"
    "features like line numbers and skip/repeat modifiers are not fully "
    "supported.",
    "  csplit file.txt '/pattern/'\n"
    "  csplit -f chapter file.txt '/Chapter/'\n"
    "  csplit -n 3 file.txt 100\n"
    "  csplit -z file.txt '/^Header$/' '/^Footer$/'",
    "split(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd", CSPLIT_OPTIONS) {
  using namespace csplit_pipeline;

  auto cfg_result = build_config(ctx);
  if (!cfg_result) {
    cp::report_error(cfg_result, L"csplit");
    return 1;
  }

  return run(*cfg_result);
}
