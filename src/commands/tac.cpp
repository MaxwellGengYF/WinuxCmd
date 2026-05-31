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
 *  - File: tac.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implementation for tac.
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

auto constexpr TAC_OPTIONS = std::array{
    OPTION("-b", "--before", "attach separator to next record", BOOL_TYPE),
    OPTION("-s", "--separator", "use STRING as separator", STRING_TYPE),
    OPTION("-r", "--regex", "separator is a regular expression", BOOL_TYPE)};

namespace tac_pipeline {
namespace cp = core::pipeline;

struct Config {
  SmallVector<std::string, 64> files;
  std::string separator;
  bool before = false;
  bool regex = false;
};

auto build_config(const CommandContext<TAC_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;

  cfg.separator = ctx.get<std::string>("--separator", "");
  if (cfg.separator.empty()) cfg.separator = ctx.get<std::string>("-s", "");
  cfg.before = ctx.get<bool>("--before", false) || ctx.get<bool>("-b", false);
  cfg.regex = ctx.get<bool>("--regex", false) || ctx.get<bool>("-r", false);

  // Empty separator with -s means NUL
  if (cfg.separator.empty() && (ctx.has("--separator") || ctx.has("-s"))) {
    cfg.separator = std::string(1, '\0');
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

auto read_and_print_reversed(const std::string& file, const Config& cfg) -> int {
  // Read entire file content
  std::string content;
  if (file == "-") {
    std::string line;
    while (std::getline(std::cin, line)) {
      content += line + "\n";
    }
  } else {
    std::ifstream f(file, std::ios::binary);
    if (!f) {
      cp::Result<int> result = core::pipeline::unexpected("cannot open for reading");
      cp::report_error(result, L"tac");
      return 1;
    }
    content.assign(std::istreambuf_iterator<char>(f),
                   std::istreambuf_iterator<char>());
  }

  if (cfg.separator.empty()) {
    // Default: reverse by lines (newline-separated)
    std::vector<std::string> lines;
    size_t pos = 0;
    while (pos < content.size()) {
      size_t nl = content.find('\n', pos);
      if (nl == std::string::npos) {
        if (pos < content.size()) lines.push_back(content.substr(pos));
        break;
      }
      lines.push_back(content.substr(pos, nl - pos + 1));
      pos = nl + 1;
    }
    for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
      safePrint(*it);
    }
	  } else {
	    // Custom separator: split by separator, reverse parts, join back
	    std::vector<std::string> parts;
	    const std::string& sep = cfg.separator;

	    if (cfg.regex) {
	      // Regex mode: find matches of the regex pattern
	      // Simple regex support: [0-9]+, [a-z]+, literal strings
	      size_t pos = 0;
	      while (pos <= content.size()) {
	        // Find the next match using simple regex matching
	        size_t match_start = std::string::npos;
	        size_t match_end = 0;
	        for (size_t i = pos; i < content.size(); ++i) {
	          char c = content[i];
	          bool matches = false;
		          // Simple character class [XYZ] or [X-Y]
		          if (sep.size() >= 3 && sep[0] == '[' && (sep.back() == ']' || (sep.size() > 3 && sep[sep.size()-2] == ']' && sep.back() == '+'))) {
		            bool plus = (sep.back() == '+');
		            size_t cls_end = plus ? sep.size() - 2 : sep.size() - 1;
		            std::string cls = sep.substr(1, cls_end);
	            if (cls.size() >= 3 && cls[1] == '-' && plus) {
	              // [X-Y]+ range
	              char lo = cls[0], hi = cls[2];
	              matches = (c >= lo && c <= hi);
	            } else if (plus) {
	              matches = (cls.find(c) != std::string::npos);
	            } else {
	              matches = (cls.find(c) != std::string::npos);
	            }
	            if (matches && match_start == std::string::npos) {
	              match_start = i;
	            }
	            if (!matches && match_start != std::string::npos) {
	              match_end = i;
	              break;
	            }
	          } else {
	            // Literal: check if substring starting at i matches sep
	            if (content.substr(i, sep.size()) == sep) {
	              match_start = i;
	              match_end = i + sep.size();
	              break;
	            }
	          }
	        }
	        if (match_start == std::string::npos) {
	          if (pos < content.size()) parts.push_back(content.substr(pos));
	          break;
	        }
	        if (match_end == 0) match_end = content.size();
	        // Push the part before the match, then the match itself
	        if (cfg.before) {
	          if (pos < match_start) parts.push_back(content.substr(pos, match_start - pos));
	          parts.push_back(content.substr(match_start, match_end - match_start));
	        } else {
	          parts.push_back(content.substr(pos, match_end - pos));
	        }
	        pos = match_end;
	      }
	    } else if (cfg.before) {
      // -b: separator is prepended to the record after it
      // Split at each separator, include sep at start of following segment
      size_t pos = 0;
      size_t found = content.find(sep, pos);
      // First segment before any separator
      if (found != std::string::npos && found > 0) {
        parts.push_back(content.substr(0, found));
      } else if (found == std::string::npos) {
        parts.push_back(content);
      }
      while (found != std::string::npos) {
        pos = found + sep.size();
        size_t next = content.find(sep, pos);
        if (next != std::string::npos) {
          parts.push_back(sep + content.substr(pos, next - pos));
        } else {
          // Final segment: sep + whatever follows (may be empty)
          parts.push_back(sep + content.substr(pos));
          break;
        }
        found = next;
      }
    } else {
      // Default: separator attached to end of record
      size_t pos = 0;
      while (pos <= content.size()) {
        size_t found = content.find(sep, pos);
        if (found == std::string::npos) {
          if (pos < content.size()) parts.push_back(content.substr(pos));
          break;
        }
        parts.push_back(content.substr(pos, found - pos + sep.size()));
        pos = found + sep.size();
      }
    }
    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
      safePrint(*it);
    }
  }

  return 0;
}

auto run(const Config& cfg) -> int {
  for (const auto& file : cfg.files) {
    int rc = read_and_print_reversed(file, cfg);
    if (rc != 0) return rc;
  }
  return 0;
}

}  // namespace tac_pipeline

REGISTER_COMMAND(
    tac, "tac", "tac [OPTION]... [FILE]...",
    "Concatenate and print files in reverse.\n"
    "\n"
    "Write each FILE to standard output, last line first.\n"
    "With no FILE, or when FILE is -, read standard input.\n"
    "\n"
    "Note: This is the reverse of 'cat'.\n"
    "Advanced features like custom separators are not implemented.",
    "  tac file.txt\n"
    "  echo -e 'line1\\nline2\\nline3' | tac",
    "cat(1), rev(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd", TAC_OPTIONS) {
  using namespace tac_pipeline;

  auto cfg_result = build_config(ctx);
  if (!cfg_result) {
    cp::report_error(cfg_result, L"tac");
    return 1;
  }

  return run(*cfg_result);
}
