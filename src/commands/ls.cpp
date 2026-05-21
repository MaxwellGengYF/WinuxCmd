/*
 *  Copyright © 2026 [caomengxuan666]
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 *  - File: ls.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - @contributor1 caomengxuan666 2507560089@qq.com
///   - @contributor2 <email2@example.com>
///   - @contributor3 <email3@example.com>
/// @Description: Implemention for ls.
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright © 2026 WinuxCmd
// *** SIMPLIFIED IMPLEMENTATION - Some features may not be fully supported ***

#include "pch/pch.h"
// include other header after pch.h
#include "core/command_macros.h"

#pragma comment(lib, "advapi32.lib")
import std;
import core;
import utils;
import container;

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

/**
 * @brief LS command options definition
 *
 * This array defines all the options supported by the ls command.
 * Each option is described with its short form, long form, and description.
 * The implementation status is also indicated for each option.
 *
 * @par Options:
 * - @a -a, @a --all: Do not ignore entries starting with . [IMPLEMENTED]
 * - @a -A, @a --almost-all: Do not list implied . and .. [IMPLEMENTED]
 * - @a -b, @a --escape: Print C-style escapes for nongraphic characters
 * [IMPLEMENTED]
 * - @a -B, @a --ignore-backups: Do not list implied entries ending with ~
 * [IMPLEMENTED]
 * - @a -c: With -lt: sort by, and show, ctime; with -l: show ctime and sort by
 * name; otherwise: sort by ctime, newest first [TODO]
 * - @a -C: List entries by columns [IMPLEMENTED]
 * - @a -d, @a --directory: List directories themselves, not their contents
 * [IMPLEMENTED]
 * - @a -f: List all entries in directory order [IMPLEMENTED]
 * - @a -F, @a --classify: Append indicator (one of *=>@|) to entries
 * [IMPLEMENTED]
 * - @a -g: Like -l, but do not list owner [IMPLEMENTED]
 * - @a -h, @a --human-readable: With -l and -s, print sizes like 1K 234M 2G
 * etc. [IMPLEMENTED]
 * - @a -i, @a --inode: Print the index number of each file [IMPLEMENTED]
 * -
 * @a -k, @a --kibibytes: Default to 1024-byte blocks for file system usage
 * [TODO]
 * - @a -L, @a --dereference: When showing file information for a symbolic link,
 * show information for the file the link references [TODO]
 * - @a -l, @a --long-list: Use a long listing format [IMPLEMENTED]
 * - @a -m: Fill width with a comma separated list of entries [TODO]
 * - @a -n, @a --numeric-uid-gid: Like -l, but list numeric user and group IDs
 * [IMPLEMENTED]
 * - @a -N, @a --literal: Print entry names without quoting [TODO]
 * - @a -o: Like -l, but do not list group information [IMPLEMENTED]
 * - @a -p, @a --indicator-style=slash: Append / indicator to directories
 * [IMPLEMENTED]
 * - @a -q, @a --hide-control-chars: Print ? instead of nongraphic characters
 * [IMPLEMENTED]
 * - @a -Q, @a --quote-name: Enclose entry names in double quotes
 * [IMPLEMENTED]
 * - @a -r, @a --reverse: Reverse order while sorting [IMPLEMENTED]
 * - @a -R, @a --recursive: List subdirectories recursively [IMPLEMENTED]
 * - @a -s, @a --size: Print the allocated size of each file, in blocks
 *
 * [IMPLEMENTED]
 * - @a -S: Sort by file size, largest first [TODO]
 * - @a -t: Sort by time, newest first [TODO]
 * - @a -T, @a --tabsize: Assume tab stops at each COLS instead of 8
 * [IMPLEMENTED]
 * - @a -u: With -lt: sort by, and show, access time; with -l: show access time
 * and sort by name; otherwise: sort by access time, newest first [TODO]
 * - @a -U: Do not sort; list entries in directory order [IMPLEMENTED]
 * - @a -v: Natural sort of (version) numbers within text [IMPLEMENTED]
 * - @a -w, @a --width: Set output width to COLS. 0 means no limit [IMPLEMENTED]
 * - @a -x: List entries by lines instead of by columns [TODO]
 * - @a -X: Sort alphabetically by entry extension [IMPLEMENTED]
 * - @a -Z, @a --context: Print any security context of each file [TODO]
 * - @a -1: List one file per line [IMPLEMENTED]
 */
auto constexpr LS_OPTIONS = std::array{
    OPTION("-a", "--all", "do not ignore entries starting with ."),
    OPTION("-A", "--almost-all", "do not list implied . and .."),
    OPTION("-b", "--escape", "print C-style escapes for nongraphic characters"),
    OPTION("-B", "--ignore-backups",
           "do not list implied entries ending with ~"),
    OPTION("-c", "",
           "with -lt: sort by, and show, ctime; with -l: show ctime and sort "
           "by name; otherwise: sort by ctime, newest first"),
    OPTION("-C", "", "list entries by columns"),
    OPTION("-d", "--directory",
           "list directories themselves, not their contents"),
    OPTION("-f", "", "list all entries in directory order"),
    OPTION("-F", "--classify", "append indicator (one of */=>@|) to entries"),
    OPTION("-g", "", "like -l, but do not list owner"),
    OPTION("-h", "--human-readable",
           "with -l and -s, print sizes like 1K 234M 2G etc."),
    OPTION("-i", "--inode", "print the index number of each file"),
    OPTION("-k", "--kibibytes",
           "default to 1024-byte blocks for file system usage"),
    OPTION("-I", "--ignore", "ignore entries matching PATTERN", STRING_TYPE),
    OPTION("-L", "--dereference",
           "when showing file information for a symbolic link, show "
           "information for the file the link references"),
    OPTION("-l", "--long-list", "use a long listing format"),
    OPTION("-m", "", "fill width with a comma separated list of entries"),
    OPTION("-n", "--numeric-uid-gid",
           "like -l, but list numeric user and group IDs"),
    OPTION("-N", "--literal", "print entry names without quoting"),
    OPTION("-o", "", "like -l, but do not list group information"),
    OPTION("-p", "--indicator-style=slash",
           "append / indicator to directories"),
    OPTION("-q", "--hide-control-chars",
           "print ? instead of nongraphic characters"),
    OPTION("-Q", "--quote-name", "enclose entry names in double quotes"),
    OPTION("-r", "--reverse", "reverse order while sorting"),
    OPTION("-R", "--recursive", "list subdirectories recursively"),
    OPTION("-s", "--size", "print the allocated size of each file, in blocks"),
    OPTION("-S", "", "sort by file size, largest first"),
    OPTION("-t", "", "sort by time, newest first"),
    OPTION("-T", "--tabsize", "assume tab stops at each COLS instead of 8",
           INT_TYPE),
    OPTION(
        "-u", "",
        "with -lt: sort by, and show, access time; with -l: show access time "
        "and sort by name; otherwise: sort by access time, newest first"),
    OPTION("-U", "", "do not sort; list entries in directory order"),
    OPTION("-v", "", "natural sort of (version) numbers within text"),
    OPTION("-w", "--width", "set output width to COLS. 0 means no limit",
           INT_TYPE),
    OPTION("-x", "", "list entries by lines instead of by columns"),
    OPTION("-X", "--sort=extension", "sort alphabetically by entry extension"),
    OPTION("-Z", "--context", "print any security context of each file"),
    OPTION("", "--sort", "sort entries by WORD", STRING_TYPE),
    OPTION("", "--format", "set output format", STRING_TYPE),
    OPTION("", "--time", "change time style", STRING_TYPE),
    OPTION("", "--color",
           "colorize the output; WHEN can be 'always', 'auto', or 'never'",
           STRING_TYPE),
    OPTION("-1", "", "list one file per line")};

// ======================================================
// Constants
// ======================================================
namespace ls_constants {
constexpr int DEFAULT_TAB_SIZE = 8;
constexpr int DEFAULT_WIDTH = 0;

// File extensions for different types
const std::array<const wchar_t *, 10> COMPRESSED_EXTS = {
    L"zip", L"rar", L"7z",  L"tar", L"gz",
    L"bz2", L"xz",  L"iso", L"cab", L"arc"};

const std::array<const wchar_t *, 10> SCRIPT_EXTS = {
    L"sh", L"bat", L"cmd", L"py", L"pl", L"lua", L"js", L"php", L"rb", L"ps1"};
}  // namespace ls_constants

/**
 * @brief Check if a file handle is a terminal
 * @param stream Stream to check
 * @return True if stream is a terminal
 */
bool is_terminal(FILE *stream) {
  DWORD mode;
  return GetConsoleMode((HANDLE)_get_osfhandle(_fileno(stream)), &mode) != 0;
}

// ======================================================
// Pipeline components
// ======================================================
namespace ls_pipeline {
namespace cp = core::pipeline;

enum class SortMode { Name, Size, Time, Version, Extension, None };
enum class TimeMode { Modification, Access, Status, Birth };
enum class FormatMode { Columns, Across, Commas, OnePerLine, Long };

auto get_terminal_width() -> int;
auto build_listing_prefix(const std::wstring &path,
                          const WIN32_FIND_DATAW &find_data,
                          const CommandContext<LS_OPTIONS.size()> &ctx)
    -> std::string;

struct EntryInfo {
  std::wstring name;
  std::wstring full_path;
  WIN32_FIND_DATAW find_data;
};

auto get_entry_extension(const std::wstring &name) -> std::wstring {
  size_t dot_pos = name.find_last_of(L".");
  if (dot_pos == std::wstring::npos || dot_pos + 1 >= name.size()) {
    return {};
  }

  std::wstring ext = name.substr(dot_pos + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
  return ext;
}

auto is_archive_extension(std::wstring_view ext) -> bool {
  for (const auto *candidate : ls_constants::COMPRESSED_EXTS) {
    if (ext == candidate) {
      return true;
    }
  }
  return false;
}

auto is_script_extension(std::wstring_view ext) -> bool {
  for (const auto *candidate : ls_constants::SCRIPT_EXTS) {
    if (ext == candidate) {
      return true;
    }
  }
  return false;
}

auto is_executable_name(const std::wstring &name) -> bool {
  const std::wstring ext = get_entry_extension(name);
  return ext == L"exe" || ext == L"com" || ext == L"bat" || ext == L"cmd" ||
         ext == L"ps1";
}

auto should_ignore_backup(const std::wstring &name,
                          const CommandContext<LS_OPTIONS.size()> &ctx)
    -> bool {
  const bool ignore_backups =
      ctx.get<bool>("-B", false) || ctx.get<bool>("--ignore-backups", false);
  return ignore_backups && !name.empty() && name.back() == L'~';
}

auto get_ignore_pattern(const CommandContext<LS_OPTIONS.size()> &ctx)
    -> std::wstring {
  std::wstring pattern = utf8_to_wstring(ctx.get<std::string>("--ignore", ""));
  if (pattern.empty()) {
    pattern = utf8_to_wstring(ctx.get<std::string>("-I", ""));
  }
  return pattern;
}

auto should_ignore_pattern(const std::wstring &name,
                           const std::wstring &pattern) -> bool {
  if (pattern.empty()) {
    return false;
  }
  return wildcard_match(pattern, name, true);
}

auto should_show_entry(const std::wstring &name,
                       const WIN32_FIND_DATAW &find_data,
                       const CommandContext<LS_OPTIONS.size()> &ctx,
                       const std::wstring &ignore_pattern) -> bool {
  const bool show_all = ctx.get<bool>("-a", false) ||
                        ctx.get<bool>("--all", false) ||
                        ctx.get<bool>("-f", false);
  const bool almost_all =
      ctx.get<bool>("-A", false) || ctx.get<bool>("--almost-all", false);

  if (name == L"." || name == L"..") {
    return show_all;
  }

  if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !show_all &&
      !almost_all) {
    return false;
  }

  if (should_ignore_backup(name, ctx)) {
    return false;
  }

  return !should_ignore_pattern(name, ignore_pattern);
}

auto get_color_for_entry(const std::wstring &name,
                         const WIN32_FIND_DATAW &find_data)
    -> std::wstring_view {
  if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    return COLOR_DIR;
  }
  if (find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
    return COLOR_LINK;
  }

  const std::wstring ext = get_entry_extension(name);
  if (is_archive_extension(ext)) {
    return COLOR_ARCHIVE;
  }
  if (is_script_extension(ext)) {
    return COLOR_SCRIPT;
  }
  if (is_executable_name(name)) {
    return COLOR_EXEC;
  }
  return COLOR_FILE;
}

auto get_indicator_suffix(const std::wstring &name,
                          const WIN32_FIND_DATAW &find_data,
                          const CommandContext<LS_OPTIONS.size()> &ctx)
    -> std::wstring {
  const bool classify =
      ctx.get<bool>("-F", false) || ctx.get<bool>("--classify", false);
  const bool slash_indicator = ctx.get<bool>("-p", false) ||
                               ctx.get<bool>("--indicator-style=slash", false);

  if (classify) {
    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      return L"/";
    }
    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
      return L"@";
    }
    if (is_executable_name(name)) {
      return L"*";
    }
    return {};
  }

  if (slash_indicator &&
      (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
    return L"/";
  }

  return {};
}

auto escape_display_name(const std::wstring &name) -> std::wstring {
  std::wstring rendered;
  rendered.reserve(name.size() * 2);

  auto append_hex = [&](unsigned int value, int width, wchar_t prefix) {
    static constexpr wchar_t digits[] = L"0123456789abcdef";
    rendered.push_back(L'\\');
    rendered.push_back(prefix);
    for (int shift = (width - 1) * 4; shift >= 0; shift -= 4) {
      rendered.push_back(digits[(value >> shift) & 0xF]);
    }
  };

  for (wchar_t ch : name) {
    switch (ch) {
      case L'\a':
        rendered += L"\\a";
        break;
      case L'\b':
        rendered += L"\\b";
        break;
      case L'\t':
        rendered += L"\\t";
        break;
      case L'\n':
        rendered += L"\\n";
        break;
      case L'\v':
        rendered += L"\\v";
        break;
      case L'\f':
        rendered += L"\\f";
        break;
      case L'\r':
        rendered += L"\\r";
        break;
      case L'\\':
        rendered += L"\\\\";
        break;
      case L'"':
        rendered += L"\\\"";
        break;
      default:
        if (ch >= 0x20 && ch < 0x7f && std::iswprint(static_cast<wint_t>(ch))) {
          rendered.push_back(ch);
        } else if (ch <= 0xFF) {
          append_hex(static_cast<unsigned int>(ch), 2, L'x');
        } else if (ch <= 0xFFFF) {
          append_hex(static_cast<unsigned int>(ch), 4, L'u');
        } else {
          append_hex(static_cast<unsigned int>(ch), 8, L'U');
        }
        break;
    }
  }

  return rendered;
}

auto render_display_name(const std::wstring &name,
                         const WIN32_FIND_DATAW &find_data,
                         const CommandContext<LS_OPTIONS.size()> &ctx)
    -> std::wstring {
  std::wstring rendered = name;

  if (ctx.get<bool>("-N", false) || ctx.get<bool>("--literal", false)) {
    // keep literal name
  } else if (ctx.get<bool>("-b", false) || ctx.get<bool>("--escape", false)) {
    rendered = escape_display_name(rendered);
  } else if (ctx.get<bool>("-q", false) ||
             ctx.get<bool>("--hide-control-chars", false)) {
    for (auto &ch : rendered) {
      if (!std::iswprint(static_cast<wint_t>(ch))) {
        ch = L'?';
      }
    }
  }

  rendered += get_indicator_suffix(name, find_data, ctx);

  if (ctx.get<bool>("-Q", false) || ctx.get<bool>("--quote-name", false)) {
    rendered = L"\"" + rendered + L"\"";
  }

  return rendered;
}

auto compare_version_strings(std::wstring_view a, std::wstring_view b) -> int {
  size_t i = 0;
  size_t j = 0;

  auto is_digit = [](wchar_t ch) {
    return std::iswdigit(static_cast<wint_t>(ch)) != 0;
  };

  while (i < a.size() || j < b.size()) {
    const bool a_digit = i < a.size() && is_digit(a[i]);
    const bool b_digit = j < b.size() && is_digit(b[j]);

    if (a_digit && b_digit) {
      size_t a_start = i;
      size_t b_start = j;
      while (i < a.size() && is_digit(a[i])) ++i;
      while (j < b.size() && is_digit(b[j])) ++j;

      while (a_start < i && a[a_start] == L'0') ++a_start;
      while (b_start < j && b[b_start] == L'0') ++b_start;

      const size_t a_len = i - a_start;
      const size_t b_len = j - b_start;
      if (a_len < b_len) return -1;
      if (a_len > b_len) return 1;

      for (size_t k = 0; k < a_len; ++k) {
        const wchar_t ac = a[a_start + k];
        const wchar_t bc = b[b_start + k];
        if (ac < bc) return -1;
        if (ac > bc) return 1;
      }
      continue;
    }

    if (a_digit != b_digit) {
      return a_digit ? -1 : 1;
    }

    if (i < a.size() && j < b.size()) {
      const wchar_t ac = a[i];
      const wchar_t bc = b[j];
      if (ac != bc) {
        const wchar_t al = std::towlower(ac);
        const wchar_t bl = std::towlower(bc);
        if (al < bl) return -1;
        if (al > bl) return 1;
        if (ac < bc) return -1;
        if (ac > bc) return 1;
      }
      ++i;
      ++j;
      continue;
    }

    if (i < a.size()) return 1;
    if (j < b.size()) return -1;
  }

  return 0;
}

auto compare_extensions(const EntryInfo &a, const EntryInfo &b) -> bool {
  const std::wstring a_ext = get_entry_extension(a.name);
  const std::wstring b_ext = get_entry_extension(b.name);
  if (a_ext < b_ext) return true;
  if (a_ext > b_ext) return false;
  return a.name < b.name;
}

auto parse_sort_mode(std::string_view value) -> std::optional<SortMode> {
  if (value.empty() || value == "name") return SortMode::Name;
  if (value == "size") return SortMode::Size;
  if (value == "time") return SortMode::Time;
  if (value == "version") return SortMode::Version;
  if (value == "extension") return SortMode::Extension;
  if (value == "none") return SortMode::None;
  return std::nullopt;
}

auto parse_time_mode(std::string_view value) -> std::optional<TimeMode> {
  if (value.empty() || value == "mtime" || value == "modification" ||
      value == "modified") {
    return TimeMode::Modification;
  }
  if (value == "atime" || value == "access" || value == "use") {
    return TimeMode::Access;
  }
  if (value == "ctime" || value == "status") {
    return TimeMode::Status;
  }
  if (value == "birth" || value == "creation") {
    return TimeMode::Birth;
  }
  return std::nullopt;
}

auto parse_format_mode(std::string_view value) -> std::optional<FormatMode> {
  if (value.empty() || value == "vertical" || value == "columns") {
    return FormatMode::Columns;
  }
  if (value == "across" || value == "horizontal") {
    return FormatMode::Across;
  }
  if (value == "commas") {
    return FormatMode::Commas;
  }
  if (value == "single-column" || value == "single" || value == "one") {
    return FormatMode::OnePerLine;
  }
  if (value == "long") {
    return FormatMode::Long;
  }
  return std::nullopt;
}

auto get_entry_time(const WIN32_FIND_DATAW &find_data, TimeMode mode)
    -> FILETIME {
  switch (mode) {
    case TimeMode::Modification:
      return find_data.ftLastWriteTime;
    case TimeMode::Access:
      return find_data.ftLastAccessTime;
    case TimeMode::Status:
    case TimeMode::Birth:
      return find_data.ftCreationTime;
  }
  return find_data.ftLastWriteTime;
}

auto compare_time_mode(const EntryInfo &a, const EntryInfo &b, TimeMode mode)
    -> bool {
  const FILETIME ta = get_entry_time(a.find_data, mode);
  const FILETIME tb = get_entry_time(b.find_data, mode);
  const int cmp = CompareFileTime(&ta, &tb);
  if (cmp != 0) return cmp > 0;
  return a.name < b.name;
}

auto get_time_string(const WIN32_FIND_DATAW &find_data, TimeMode mode)
    -> std::string {
  FILETIME file_time = get_entry_time(find_data, mode);
  FILETIME local_ft{};
  FileTimeToLocalFileTime(&file_time, &local_ft);

  SYSTEMTIME st{};
  FileTimeToSystemTime(&local_ft, &st);

  const char *month_abbrs[] = {"",    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  char buf[64];
  snprintf(buf, sizeof(buf), "%s %2d %02d:%02d", month_abbrs[st.wMonth],
           st.wDay, st.wHour, st.wMinute);
  return std::string(buf);
}

auto resolve_color_enabled(const CommandContext<LS_OPTIONS.size()> &ctx)
    -> bool {
  std::string color_option = ctx.get<std::string>("--color", "auto");
  if (color_option == "never") return false;
  if (color_option == "always") return true;
  return is_terminal(stdout);
}

struct RenderedEntry {
  std::string text;
  size_t visible_width = 0;
};

auto render_inline_entry(const EntryInfo &entry,
                         const CommandContext<LS_OPTIONS.size()> &ctx,
                         bool color_enabled) -> RenderedEntry {
  std::wstring display_name =
      render_display_name(entry.name, entry.find_data, ctx);
  std::string prefix =
      build_listing_prefix(entry.full_path, entry.find_data, ctx);
  std::string text = prefix;
  if (color_enabled) {
    text += wstring_to_utf8(
        std::wstring(get_color_for_entry(entry.name, entry.find_data)));
  }
  text += wstring_to_utf8(display_name);
  if (color_enabled) {
    text += wstring_to_utf8(COLOR_RESET);
  }

  return {std::move(text), prefix.size() + display_name.size()};
}

auto build_rendered_entries(const std::vector<EntryInfo> &entries,
                            const CommandContext<LS_OPTIONS.size()> &ctx)
    -> std::vector<RenderedEntry> {
  const bool color_enabled = resolve_color_enabled(ctx);
  std::vector<RenderedEntry> rendered;
  rendered.reserve(entries.size());
  for (const auto &entry : entries) {
    rendered.push_back(render_inline_entry(entry, ctx, color_enabled));
  }
  return rendered;
}

auto print_rendered_entries(const std::vector<RenderedEntry> &entries,
                            size_t width) -> void {
  size_t line_width = 0;
  for (size_t i = 0; i < entries.size(); ++i) {
    if (i > 0) {
      if (line_width > 0 && line_width + 2 + entries[i].visible_width > width) {
        safePrintLn(L"");
        line_width = 0;
      } else {
        safePrint(", ");
        line_width += 2;
      }
    }
    safePrint(entries[i].text);
    line_width += entries[i].visible_width;
  }
}

auto print_grid(const std::vector<EntryInfo> &entries,
                const CommandContext<LS_OPTIONS.size()> &ctx,
                bool across_layout) -> void {
  if (entries.empty()) return;

  const auto rendered = build_rendered_entries(entries, ctx);
  size_t max_visible_width = 0;
  for (const auto &entry : rendered) {
    max_visible_width = std::max(max_visible_width, entry.visible_width);
  }

  int width = ctx.get<int>("-w", 0);
  if (width <= 0) {
    width = ctx.get<int>("--width", 0);
  }
  if (width <= 0) {
    width = get_terminal_width();
  }

  int cols = width / (static_cast<int>(max_visible_width) + 2);
  if (cols < 1) cols = 1;
  int rows = static_cast<int>((entries.size() + cols - 1) / cols);
  std::vector<size_t> col_widths(static_cast<size_t>(cols), max_visible_width);

  for (size_t idx = 0; idx < rendered.size(); ++idx) {
    size_t col = across_layout ? idx % static_cast<size_t>(cols)
                               : idx / static_cast<size_t>(rows);
    col_widths[col] = std::max(col_widths[col], rendered[idx].visible_width);
  }

  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      size_t index = across_layout ? static_cast<size_t>(row * cols + col)
                                   : static_cast<size_t>(row + col * rows);
      if (index >= rendered.size()) continue;

      safePrint(rendered[index].text);
      if (col < cols - 1) {
        size_t padding = col_widths[static_cast<size_t>(col)] -
                         rendered[index].visible_width;
        for (size_t i = 0; i < padding + 2; ++i) {
          safePrint(" ");
        }
      }
    }
    safePrintLn(L"");
  }
}

/**
 * @brief Validate arguments
 * @param ctx Command context
 * @return Result
 * with paths to process
 */
auto validate_arguments(const CommandContext<LS_OPTIONS.size()> &ctx)
    -> cp::Result<std::vector<std::string>> {
  std::vector<std::string> paths;
  for (auto arg : ctx.positionals) {
    paths.push_back(std::string(arg));
  }

  if (paths.empty()) {
    paths.push_back(".");
  }

  return paths;
}

/**
 * @brief Get file permissions string
 * @param find_data WIN32_FIND_DATAW structure
 * @return Permissions string in ls format
 */
/**
 * @brief Get file permissions string (improved: simulate real Linux
 * permissions)
 * @param find_data WIN32_FIND_DATAW structure
 * @return Permissions string in ls format
 */
auto get_permissions_string(const WIN32_FIND_DATAW &find_data) -> std::string {
  char perms[11] = "----------";
  perms[10] = '\0';

  // Set file type
  if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    perms[0] = 'd';
    // Directory: owner rwx, group rx, other rx
    perms[1] = 'r';
    perms[2] = 'w';
    perms[3] = 'x';
    perms[4] = 'r';
    perms[5] = '-';
    perms[6] = 'x';
    perms[7] = 'r';
    perms[8] = '-';
    perms[9] = 'x';
  } else if (find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
    perms[0] = 'l';  // Symbolic link
    perms[1] = 'r';
    perms[2] = 'w';
    perms[3] = 'x';
    perms[4] = 'r';
    perms[5] = 'w';
    perms[6] = 'x';
    perms[7] = 'r';
    perms[8] = 'w';
    perms[9] = 'x';
  } else {
    perms[0] = '-';
    // Check if file is executable (exe/bat/cmd/ps1)
    std::wstring filename = find_data.cFileName;
    std::wstring ext;
    size_t dot_pos = filename.find_last_of(L".");
    if (dot_pos != std::wstring::npos) {
      ext = filename.substr(dot_pos + 1);
      std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    }

    bool is_executable =
        (ext == L"exe" || ext == L"bat" || ext == L"cmd" || ext == L"ps1");

    // Regular file:
    // - Executable: owner rwx, group rx, other rx
    // - Non-executable: owner rw, group r, other r
    perms[1] = 'r';
    perms[2] = 'w';
    perms[4] = 'r';
    perms[5] = '-';
    perms[7] = 'r';
    perms[8] = '-';

    if (is_executable) {
      perms[3] = 'x';
      perms[6] = 'x';
      perms[9] = 'x';
    } else {
      perms[3] = '-';
      perms[6] = '-';
      perms[9] = '-';
    }
  }

  // Handle hidden files (optional)
  // Note: Keeping 10 characters for consistent formatting
  return std::string(perms, 10);
}

/**
 * @brief Get file size string
 * @param find_data WIN32_FIND_DATAW structure
 * @param ctx Command context
 * @return File size string
 */
auto get_file_size_string(const WIN32_FIND_DATAW &find_data,
                          const CommandContext<LS_OPTIONS.size()> &ctx)
    -> std::string {
  uint64_t fileSize = static_cast<uint64_t>(find_data.nFileSizeLow) |
                      (static_cast<uint64_t>(find_data.nFileSizeHigh) << 32);

  char buf[32];

  if (ctx.get<bool>("-h", false) || ctx.get<bool>("--human-readable", false)) {
    const char *units[] = {"B", "K", "M", "G", "T"};
    int unitIndex = 0;
    double size = static_cast<double>(fileSize);

    while (size >= 1024.0 && unitIndex < 4) {
      size /= 1024.0;
      ++unitIndex;
    }

    if (size < 10.0) {
      snprintf(buf, sizeof(buf), "%.1f%s", size, units[unitIndex]);
    } else {
      snprintf(buf, sizeof(buf), "%.0f%s", size, units[unitIndex]);
    }
  } else {
    snprintf(buf, sizeof(buf), "%llu", fileSize);
  }

  return std::string(buf);
}

auto get_file_index_string(const std::wstring &path,
                           const WIN32_FIND_DATAW &find_data,
                           const CommandContext<LS_OPTIONS.size()> &ctx)
    -> std::string {
  DWORD flags = 0;
  if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    flags |= FILE_FLAG_BACKUP_SEMANTICS;
  }

  const bool dereference =
      ctx.get<bool>("-L", false) || ctx.get<bool>("--dereference", false);
  if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
      !dereference) {
    flags |= FILE_FLAG_OPEN_REPARSE_POINT;
  }

  HANDLE handle =
      CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr, OPEN_EXISTING, flags, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    return {};
  }

  BY_HANDLE_FILE_INFORMATION info{};
  std::string result;
  if (GetFileInformationByHandle(handle, &info)) {
    ULONGLONG file_index = (static_cast<ULONGLONG>(info.nFileIndexHigh) << 32) |
                           static_cast<ULONGLONG>(info.nFileIndexLow);
    result = std::to_string(file_index);
  }
  CloseHandle(handle);
  return result;
}

auto get_link_count(const std::wstring &path, const WIN32_FIND_DATAW &find_data,
                    const CommandContext<LS_OPTIONS.size()> &ctx)
    -> unsigned long {
  DWORD flags = 0;
  if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    flags |= FILE_FLAG_BACKUP_SEMANTICS;
  }

  const bool dereference =
      ctx.get<bool>("-L", false) || ctx.get<bool>("--dereference", false);
  if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
      !dereference) {
    flags |= FILE_FLAG_OPEN_REPARSE_POINT;
  }

  HANDLE handle =
      CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr, OPEN_EXISTING, flags, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    return 1;
  }

  BY_HANDLE_FILE_INFORMATION info{};
  DWORD link_count = 1;
  if (GetFileInformationByHandle(handle, &info)) {
    link_count = std::max<DWORD>(info.nNumberOfLinks, 1);
  }
  CloseHandle(handle);
  return static_cast<unsigned long>(link_count);
}

auto get_allocated_blocks_string(const std::wstring &path,
                                 const WIN32_FIND_DATAW &find_data,
                                 const CommandContext<LS_OPTIONS.size()> &ctx)
    -> std::string {
  (void)ctx;
  ULARGE_INTEGER allocated{};
  DWORD high = 0;
  DWORD low = GetCompressedFileSizeW(path.c_str(), &high);
  if (low == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
    uint64_t logical_size =
        static_cast<uint64_t>(find_data.nFileSizeLow) |
        (static_cast<uint64_t>(find_data.nFileSizeHigh) << 32);
    allocated.QuadPart = logical_size;
  } else {
    allocated.LowPart = low;
    allocated.HighPart = high;
  }

  uint64_t blocks = (allocated.QuadPart + 1023ULL) / 1024ULL;
  return std::to_string(blocks);
}

auto build_listing_prefix(const std::wstring &path,
                          const WIN32_FIND_DATAW &find_data,
                          const CommandContext<LS_OPTIONS.size()> &ctx)
    -> std::string {
  std::string prefix;
  if (ctx.get<bool>("-i", false) || ctx.get<bool>("--inode", false)) {
    prefix = get_file_index_string(path, find_data, ctx);
    if (!prefix.empty()) {
      prefix.push_back(' ');
    }
  }
  if (ctx.get<bool>("-s", false) || ctx.get<bool>("--size", false)) {
    prefix += get_allocated_blocks_string(path, find_data, ctx);
    prefix.push_back(' ');
  }
  return prefix;
}

/**
 * @brief Get file modification time string (improved: timezone support)
 *
 * @param find_data WIN32_FIND_DATAW structure
 * @param use_utc Whether to use
 * UTC time (default: false = local time)
 * @return Modification time string
 */
auto get_modification_time_string(const WIN32_FIND_DATAW &find_data,
                                  bool use_utc = false) -> std::string {
  if (use_utc) {
    FILETIME file_time = find_data.ftLastWriteTime;
    SYSTEMTIME st{};
    FileTimeToSystemTime(&file_time, &st);
    const char *month_abbrs[] = {"",    "Jan", "Feb", "Mar", "Apr",
                                 "May", "Jun", "Jul", "Aug", "Sep",
                                 "Oct", "Nov", "Dec"};
    char buf[64];
    snprintf(buf, sizeof(buf), "%s %2d %02d:%02d", month_abbrs[st.wMonth],
             st.wDay, st.wHour, st.wMinute);
    return std::string(buf);
  }
  return get_time_string(find_data, TimeMode::Modification);
}

/**
 * @brief Get file owner and group information (improved: support numeric ID)
 * @param use_numeric Whether to return numeric UID/GID (-n option)
 * @return Pair of (owner, group) strings
 */
auto get_file_owner_and_group(bool use_numeric = false)
    -> std::pair<std::string, std::string> {
  if (use_numeric) {
    // Get Windows SID (simulate UID/GID)
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
      DWORD bufferSize = 0;
      GetTokenInformation(hToken, TokenUser, nullptr, 0, &bufferSize);
      std::vector<BYTE> buffer(bufferSize);
      PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(buffer.data());
      if (GetTokenInformation(hToken, TokenUser, pTokenUser, bufferSize,
                              &bufferSize)) {
        LPWSTR sidStr = nullptr;
        if (ConvertSidToStringSidW(pTokenUser->User.Sid, &sidStr)) {
          // Extract numeric part from SID (simulate UID 197121)
          std::wstring sid(sidStr,
                           wcslen(sidStr));  // Construct from known length
          LocalFree(sidStr);

          size_t lastDash = sid.find_last_of(L'-');
          std::wstring uid_wstr = (lastDash != std::wstring::npos)
                                      ? sid.substr(lastDash + 1)
                                      : L"197121";

          // Convert wide string to UTF-8
          std::string uid = wstring_to_utf8(uid_wstr);
          CloseHandle(hToken);
          return {uid, uid};  // UID = GID (Windows default)
        }
      }
      CloseHandle(hToken);
    }
    return {"197121", "197121"};  // Fallback
  }

  // Return username using ANSI version for efficiency
  char username[UNLEN + 1];
  DWORD username_len = UNLEN + 1;
  if (!GetUserNameA(username, &username_len)) {
    return {"user", "group"};
  }

  std::string username_str = username;
  size_t pos = username_str.find('\\');
  if (pos != std::string::npos) {
    username_str = username_str.substr(pos + 1);
  }
  return {username_str, username_str};
}

/**
 * @brief Get terminal width
 * @return Terminal width in columns
 */
auto get_terminal_width() -> int {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
  }
  return 80;  // Default to 80 columns if we can't get terminal width
}

/**
 * @brief Get string display width (simplified, assumes 1 character = 1 column)
 * @param str String to measure
 * @return Display width in columns
 */
auto string_display_width(const std::wstring &str) -> size_t {
  return str.length();
}

/**
 * @brief Calculate optimal column layout
 * @param entries List of entries
 * @param terminal_width Terminal width in columns
 * @return Number of columns and number of rows
 */
auto calculate_layout(const std::vector<std::wstring> &entries,
                      int terminal_width) -> std::pair<int, int> {
  if (entries.empty()) {
    return {0, 0};
  }

  // Find the longest entry display width
  size_t max_display_width = 0;
  for (const auto &entry : entries) {
    max_display_width =
        std::max(max_display_width, string_display_width(entry));
  }

  // Minimum column width = max display width + 2 spaces padding
  int min_column_width = static_cast<int>(max_display_width) + 2;
  if (min_column_width <= 0) {
    min_column_width = 1;
  }

  // Calculate maximum possible columns with minimum width
  int max_cols = terminal_width / min_column_width;
  if (max_cols < 1) {
    max_cols = 1;
  }

  // Try to find optimal column width that fills the terminal
  int best_cols = max_cols;
  int best_column_width = min_column_width;

  // If we can fit more than 1 column, try to adjust column width to fill the
  // screen
  if (max_cols > 1) {
    // Calculate how much space is left with minimum column width
    int remaining_space = terminal_width % min_column_width;

    // If there's remaining space, distribute it among columns
    if (remaining_space > 0) {
      // Calculate how much extra space per column
      int extra_per_column = remaining_space / max_cols;

      // New column width with extra space
      int new_column_width = min_column_width + extra_per_column;

      // Calculate new number of columns with adjusted width
      int new_cols = terminal_width / new_column_width;
      if (new_cols > 0) {
        best_cols = new_cols;
        best_column_width = new_column_width;
      }
    }
  }

  // Calculate number of rows
  int rows = (entries.size() + best_cols - 1) / best_cols;

  return {best_cols, rows};
}

/**
 * @brief Print entries in column format
 * @param entries List of entries
 * @param ctx Command context
 */
auto print_columns(const std::vector<EntryInfo> &entries,
                   const CommandContext<LS_OPTIONS.size()> &ctx) {
  if (entries.empty()) {
    return;
  }

  std::vector<std::string> prefixes;
  std::vector<std::wstring> display_names;
  prefixes.reserve(entries.size());
  display_names.reserve(entries.size());
  for (const auto &entry : entries) {
    prefixes.push_back(
        build_listing_prefix(entry.full_path, entry.find_data, ctx));
    display_names.push_back(
        render_display_name(entry.name, entry.find_data, ctx));
  }

  // Check if color is enabled based on --color option
  bool color_enabled = true;  // Default to enabled
  std::string color_option = ctx.get<std::string>("--color", "auto");
  if (color_option == "never") {
    color_enabled = false;
  } else if (color_option == "auto") {
    color_enabled = is_terminal(stdout);
  }

  // Get terminal width or use specified width
  int width = ctx.get<int>("-w", 0);
  if (width <= 0) {
    width = ctx.get<int>("--width", 0);
  }
  if (width <= 0) {
    width = get_terminal_width();
  }

  // Calculate column widths to fill the terminal
  size_t max_display_width = 0;
  for (size_t i = 0; i < display_names.size(); ++i) {
    const auto &entry = display_names[i];
    max_display_width = std::max(
        max_display_width, prefixes[i].size() + string_display_width(entry));
  }

  // Calculate base column width
  int base_col_width = static_cast<int>(max_display_width) + 2;
  if (base_col_width <= 0) {
    base_col_width = 1;
  }

  int cols = width / base_col_width;
  if (cols < 1) cols = 1;
  int rows = (entries.size() + cols - 1) / cols;

  // Calculate total width used by all columns
  int total_used_width = cols * base_col_width;
  int remaining_space = width - total_used_width;

  // Distribute remaining space among columns
  std::vector<int> col_widths(cols, base_col_width);
  if (remaining_space > 0 && cols > 0) {
    int extra_per_col = remaining_space / cols;
    int extra_remaining = remaining_space % cols;

    for (int i = 0; i < cols; ++i) {
      col_widths[i] += extra_per_col;
      if (i < extra_remaining) {
        col_widths[i] += 1;
      }
    }
  }

  // Print entries in columns
  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      size_t index = row + col * rows;
      if (index < entries.size()) {
        const auto &entry = entries[index];
        const auto &prefix = prefixes[index];
        const auto &display_name = display_names[index];

        safePrint(std::string_view(prefix));
        if (color_enabled) {
          safePrint(get_color_for_entry(entry.name, entry.find_data));
        }

        safePrint(wstring_to_utf8(display_name));
        if (color_enabled) {
          safePrint(COLOR_RESET);
        }

        if (col < cols - 1) {
          size_t current_width =
              prefix.size() + string_display_width(display_name);
          int spaces_needed = col_widths[col] - static_cast<int>(current_width);
          if (spaces_needed > 0) {
            for (int i = 0; i < spaces_needed; ++i) {
              safePrint(L" ");
            }
          } else {
            safePrint(L"  ");
          }
        }
      }
    }
    safePrintLn(L"");
  }
}

/**
 * @brief List a single file (not a directory) - forward declaration
 * @param path Path to file
 * @param ctx Command context
 * @return Result with success status
 */
auto list_file(const std::string &path,
               const CommandContext<LS_OPTIONS.size()> &ctx)
    -> cp::Result<bool>;

/**
 * @brief List directory contents
 * @param path Path to directory
 * @param ctx Command context
 * @return Result with success status
 */
auto list_directory(const std::string &path,
                    const CommandContext<LS_OPTIONS.size()> &ctx)
    -> cp::Result<bool> {
  std::wstring wpath = utf8_to_wstring(path);

  // Check -d option: list directories themselves, not their contents
  bool list_dir_only =
      ctx.get<bool>("-d", false) || ctx.get<bool>("--directory", false);

  if (list_dir_only) {
    // Get directory attributes
    WIN32_FIND_DATAW dir_data;
    HANDLE hFind = FindFirstFileW(wpath.c_str(), &dir_data);

    if (hFind == INVALID_HANDLE_VALUE) {
      return std::unexpected("cannot access '" + path +
                             "': No such file or directory");
    }

    // Display directory itself as a file
    FindClose(hFind);
    return list_file(path, ctx);
  }

  // Normal directory listing
  std::wstring search_path = wpath + L"\\*";
  const std::wstring ignore_pattern = get_ignore_pattern(ctx);

  WIN32_FIND_DATAW find_data;
  HANDLE hFind = FindFirstFileW(search_path.c_str(), &find_data);

  if (hFind == INVALID_HANDLE_VALUE) {
    return std::unexpected("cannot access '" + path +
                           "': No such file or directory");
  }

  std::vector<EntryInfo> entries;
  do {
    std::wstring filename = find_data.cFileName;
    if (!should_show_entry(filename, find_data, ctx, ignore_pattern)) {
      continue;
    }
    entries.push_back({filename, wpath + L"\\" + filename, find_data});
  } while (FindNextFileW(hFind, &find_data) != 0);

  FindClose(hFind);

  std::string sort_option = ctx.get<std::string>("--sort", "");
  std::string format_option = ctx.get<std::string>("--format", "");
  std::string time_option = ctx.get<std::string>("--time", "");

  FormatMode format_mode = FormatMode::Columns;
  if (!format_option.empty()) {
    auto parsed = parse_format_mode(format_option);
    if (parsed) {
      format_mode = *parsed;
    }
  } else if (ctx.get<bool>("-m", false)) {
    format_mode = FormatMode::Commas;
  } else if (ctx.get<bool>("-x", false)) {
    format_mode = FormatMode::Across;
  } else if (ctx.get<bool>("-1", false)) {
    format_mode = FormatMode::OnePerLine;
  } else if (ctx.get<bool>("-C", false)) {
    format_mode = FormatMode::Columns;
  }
  if (ctx.get<bool>("-l", false) || ctx.get<bool>("--long-list", false)) {
    format_mode = FormatMode::Long;
  }

  TimeMode time_mode = TimeMode::Modification;
  if (!time_option.empty()) {
    auto parsed = parse_time_mode(time_option);
    if (parsed) {
      time_mode = *parsed;
    }
  } else if (ctx.get<bool>("-u", false)) {
    time_mode = TimeMode::Access;
  } else if (ctx.get<bool>("-c", false)) {
    time_mode = TimeMode::Status;
  }

  SortMode sort_mode = SortMode::Name;
  bool explicit_sort = false;
  if (!sort_option.empty()) {
    auto parsed = parse_sort_mode(sort_option);
    if (parsed) {
      sort_mode = *parsed;
      explicit_sort = true;
    }
  } else if (ctx.get<bool>("-U", false) || ctx.get<bool>("-f", false)) {
    sort_mode = SortMode::None;
    explicit_sort = true;
  } else if (ctx.get<bool>("-S", false)) {
    sort_mode = SortMode::Size;
    explicit_sort = true;
  } else if (ctx.get<bool>("-t", false)) {
    sort_mode = SortMode::Time;
    explicit_sort = true;
  } else if (ctx.get<bool>("-v", false)) {
    sort_mode = SortMode::Version;
    explicit_sort = true;
  } else if (ctx.get<bool>("-X", false)) {
    sort_mode = SortMode::Extension;
    explicit_sort = true;
  }

  if (!explicit_sort && format_mode != FormatMode::Long &&
      time_mode != TimeMode::Modification) {
    sort_mode = SortMode::Time;
  }

  if (sort_mode != SortMode::None) {
    switch (sort_mode) {
      case SortMode::Time:
        std::sort(entries.begin(), entries.end(),
                  [&](const EntryInfo &a, const EntryInfo &b) {
                    return compare_time_mode(a, b, time_mode);
                  });
        break;
      case SortMode::Size:
        std::sort(
            entries.begin(), entries.end(),
            [](const EntryInfo &a, const EntryInfo &b) {
              uint64_t size_a =
                  static_cast<uint64_t>(a.find_data.nFileSizeLow) |
                  (static_cast<uint64_t>(a.find_data.nFileSizeHigh) << 32);
              uint64_t size_b =
                  static_cast<uint64_t>(b.find_data.nFileSizeLow) |
                  (static_cast<uint64_t>(b.find_data.nFileSizeHigh) << 32);
              if (size_a != size_b) {
                return size_a > size_b;
              }
              return a.name < b.name;
            });
        break;
      case SortMode::Version:
        std::sort(entries.begin(), entries.end(),
                  [](const EntryInfo &a, const EntryInfo &b) {
                    int cmp = compare_version_strings(a.name, b.name);
                    if (cmp != 0) return cmp < 0;
                    return a.name < b.name;
                  });
        break;
      case SortMode::Extension:
        std::sort(entries.begin(), entries.end(), compare_extensions);
        break;
      case SortMode::Name:
        std::sort(entries.begin(), entries.end(),
                  [](const EntryInfo &a, const EntryInfo &b) {
                    return a.name < b.name;
                  });
        break;
      case SortMode::None:
        break;
    }

    if (ctx.get<bool>("-r", false) || ctx.get<bool>("--reverse", false)) {
      std::reverse(entries.begin(), entries.end());
    }
  }

  bool long_format = format_mode == FormatMode::Long;
  bool use_numeric =
      ctx.get<bool>("-n", false) || ctx.get<bool>("--numeric-uid-gid", false);

  if (long_format) {
    const bool show_inode =
        ctx.get<bool>("-i", false) || ctx.get<bool>("--inode", false);
    const bool show_blocks =
        ctx.get<bool>("-s", false) || ctx.get<bool>("--size", false);

    struct FileInfo {
      std::wstring name;
      WIN32_FIND_DATAW find_data;
      std::wstring full_path;
      std::string perms;
      std::string inode;
      std::string blocks;
      std::string link_count;
      std::string size;
      std::string mtime;
      std::string owner;
      std::string group;
    };

    // Collect file information (we already have find_data from entries)
    std::vector<FileInfo> files;
    for (const auto &entry : entries) {
      FileInfo info;
      info.name = entry.name;
      info.find_data = entry.find_data;
      info.full_path = entry.full_path;
      info.perms = get_permissions_string(entry.find_data);
      if (show_inode) {
        info.inode =
            get_file_index_string(entry.full_path, entry.find_data, ctx);
      }
      if (show_blocks) {
        info.blocks =
            get_allocated_blocks_string(entry.full_path, entry.find_data, ctx);
      }
      info.link_count =
          std::to_string(get_link_count(entry.full_path, entry.find_data, ctx));
      info.size = get_file_size_string(entry.find_data, ctx);
      info.mtime = get_time_string(entry.find_data, time_mode);

      // Get owner and group
      auto [owner, group] = get_file_owner_and_group(use_numeric);
      info.owner = owner;
      info.group = group;

      files.push_back(info);
    }

    // Calculate maximum widths for alignment
    size_t max_owner_len = 0;
    size_t max_group_len = 0;
    size_t max_inode_len = 0;
    size_t max_blocks_len = 0;
    size_t max_link_len = 0;
    size_t max_size_len = 0;
    for (const auto &file : files) {
      max_owner_len = std::max(max_owner_len, file.owner.length());
      max_group_len = std::max(max_group_len, file.group.length());
      if (show_inode) {
        max_inode_len = std::max(max_inode_len, file.inode.length());
      }
      if (show_blocks) {
        max_blocks_len = std::max(max_blocks_len, file.blocks.length());
      }
      max_link_len = std::max(max_link_len, file.link_count.length());
      max_size_len = std::max(max_size_len, file.size.length());
    }

    // Set minimum widths to avoid empty values
    if (max_owner_len == 0) max_owner_len = 1;
    if (max_group_len == 0) max_group_len = 1;
    if (show_inode && max_inode_len == 0) max_inode_len = 1;
    if (show_blocks && max_blocks_len == 0) max_blocks_len = 1;
    if (max_link_len == 0) max_link_len = 1;
    if (max_size_len == 0) max_size_len = 1;

    // Long format output
    for (const auto &file_info : files) {
      std::wstring display_name =
          render_display_name(file_info.name, file_info.find_data, ctx);

      if (show_inode) {
        int inode_padding = static_cast<int>(max_inode_len) -
                            static_cast<int>(file_info.inode.length());
        for (int i = 0; i < inode_padding; ++i) {
          safePrint(" ");
        }
        safePrint(std::string_view(file_info.inode));
        safePrint(" ");
      }

      if (show_blocks) {
        int blocks_padding = static_cast<int>(max_blocks_len) -
                             static_cast<int>(file_info.blocks.length());
        for (int i = 0; i < blocks_padding; ++i) {
          safePrint(" ");
        }
        safePrint(std::string_view(file_info.blocks));
        safePrint(" ");
      }

      // 1. Permissions and link count
      safePrint(std::string_view(file_info.perms));
      safePrint(" ");
      int link_padding = static_cast<int>(max_link_len) -
                         static_cast<int>(file_info.link_count.length());
      for (int i = 0; i < link_padding; i++) {
        safePrint(" ");
      }
      safePrint(std::string_view(file_info.link_count));
      safePrint(" ");

      // 2. Owner (left-aligned)
      safePrint(std::string_view(file_info.owner));
      int owner_padding = static_cast<int>(max_owner_len) -
                          static_cast<int>(file_info.owner.length());
      for (int i = 0; i < owner_padding; i++) {
        safePrint(" ");
      }
      safePrint(" ");

      // 3. Group (left-aligned)
      safePrint(std::string_view(file_info.group));
      int group_padding = static_cast<int>(max_group_len) -
                          static_cast<int>(file_info.group.length());
      for (int i = 0; i < group_padding; i++) {
        safePrint(" ");
      }
      safePrint(" ");

      // 4. File size (right-aligned)
      int size_padding = static_cast<int>(max_size_len) -
                         static_cast<int>(file_info.size.length());
      for (int i = 0; i < size_padding; i++) {
        safePrint(" ");
      }
      safePrint(std::string_view(file_info.size));
      safePrint(" ");

      // 5. Modification time
      safePrint(std::string_view(file_info.mtime));
      safePrint(" ");

      bool color_enabled = resolve_color_enabled(ctx);

      if (color_enabled) {
        safePrint(get_color_for_entry(file_info.name, file_info.find_data));
      }

      safePrint(wstring_to_utf8(display_name));
      if (color_enabled) {
        safePrint(COLOR_RESET);
      }
      safePrintLn(L"");
    }
  } else if (format_mode == FormatMode::Commas) {
    auto rendered = build_rendered_entries(entries, ctx);
    int width = ctx.get<int>("-w", 0);
    if (width <= 0) {
      width = ctx.get<int>("--width", 0);
    }
    if (width <= 0) {
      width = get_terminal_width();
    }
    print_rendered_entries(rendered, static_cast<size_t>(width));
    safePrintLn(L"");
  } else if (format_mode == FormatMode::OnePerLine) {
    auto rendered = build_rendered_entries(entries, ctx);
    for (const auto &entry : rendered) {
      safePrint(entry.text);
      safePrintLn(L"");
    }
  } else if (format_mode == FormatMode::Across) {
    print_grid(entries, ctx, true);
  } else {
    print_grid(entries, ctx, false);
  }

  return true;
}

/**
 * @brief List a single file (not a directory)
 * @param path Path to file
 * @param ctx Command context
 * @return Result with success status
 */
auto list_file(const std::string &path,
               const CommandContext<LS_OPTIONS.size()> &ctx)
    -> cp::Result<bool> {
  std::wstring wpath = utf8_to_wstring(path);

  // Extract just the filename for display
  WIN32_FIND_DATAW find_data;
  HANDLE hFind = FindFirstFileW(wpath.c_str(), &find_data);

  if (hFind == INVALID_HANDLE_VALUE) {
    return std::unexpected("cannot access '" + path +
                           "': No such file or directory");
  }

  std::wstring filename = find_data.cFileName;
  FindClose(hFind);

  std::string format_option = ctx.get<std::string>("--format", "");
  std::string time_option = ctx.get<std::string>("--time", "");

  FormatMode format_mode = FormatMode::Columns;
  if (!format_option.empty()) {
    auto parsed = parse_format_mode(format_option);
    if (parsed) {
      format_mode = *parsed;
    }
  } else if (ctx.get<bool>("-m", false)) {
    format_mode = FormatMode::Commas;
  } else if (ctx.get<bool>("-x", false)) {
    format_mode = FormatMode::Across;
  } else if (ctx.get<bool>("-1", false)) {
    format_mode = FormatMode::OnePerLine;
  }
  if (ctx.get<bool>("-l", false) || ctx.get<bool>("--long-list", false)) {
    format_mode = FormatMode::Long;
  }

  TimeMode time_mode = TimeMode::Modification;
  if (!time_option.empty()) {
    auto parsed = parse_time_mode(time_option);
    if (parsed) {
      time_mode = *parsed;
    }
  } else if (ctx.get<bool>("-u", false)) {
    time_mode = TimeMode::Access;
  } else if (ctx.get<bool>("-c", false)) {
    time_mode = TimeMode::Status;
  }

  bool long_format = format_mode == FormatMode::Long;
  bool use_numeric =
      ctx.get<bool>("-n", false) || ctx.get<bool>("--numeric-uid-gid", false);
  bool show_inode =
      ctx.get<bool>("-i", false) || ctx.get<bool>("--inode", false);
  bool show_blocks =
      ctx.get<bool>("-s", false) || ctx.get<bool>("--size", false);

  if (long_format) {
    // Long format output for single file
    auto perms = get_permissions_string(find_data);
    auto inode = show_inode ? get_file_index_string(wpath, find_data, ctx) : "";
    auto blocks =
        show_blocks ? get_allocated_blocks_string(wpath, find_data, ctx) : "";
    auto link_count = std::to_string(get_link_count(wpath, find_data, ctx));
    auto size = get_file_size_string(find_data, ctx);
    auto mtime = get_time_string(find_data, time_mode);
    auto [owner, group] = get_file_owner_and_group(use_numeric);
    std::wstring display_name = render_display_name(filename, find_data, ctx);

    if (!inode.empty()) {
      safePrint(std::string_view(inode));
      safePrint(" ");
    }
    if (!blocks.empty()) {
      safePrint(std::string_view(blocks));
      safePrint(" ");
    }

    // 1. Permissions and link count
    safePrint(std::string_view(perms));
    safePrint(" ");
    safePrint(std::string_view(link_count));
    safePrint(" ");

    // 2. Owner (left-aligned, with padding to match column width)
    safePrint(std::string_view(owner));
    safePrint(" ");

    // 3. Group (left-aligned)
    safePrint(std::string_view(group));
    safePrint(" ");

    // 4. File size (right-aligned, pad to at least 8 chars)
    if (size.length() < 8) {
      for (size_t i = 0; i < 8 - size.length(); i++) {
        safePrint(" ");
      }
    }
    safePrint(std::string_view(size));
    safePrint(" ");

    // 5. Modification time
    safePrint(std::string_view(mtime));
    safePrint(" ");

    bool color_enabled = resolve_color_enabled(ctx);

    if (color_enabled) {
      safePrint(get_color_for_entry(filename, find_data));
    }

    safePrint(wstring_to_utf8(display_name));
    if (color_enabled) {
      safePrint(COLOR_RESET);
    }
    safePrintLn(L"");
  } else {
    auto rendered = render_inline_entry({filename, wpath, find_data}, ctx,
                                        resolve_color_enabled(ctx));
    safePrint(rendered.text);
    safePrintLn(L"");
  }

  return true;
}

/**
 * @brief List directory recursively
 * @param path Path to directory
 * @param ctx Command context
 * @param depth Current recursion depth
 * @return Result with success status
 */
auto list_directory_recursive(const std::string &path,
                              const CommandContext<LS_OPTIONS.size()> &ctx,
                              int depth = 0) -> cp::Result<bool> {
  // Print header for subdirectories
  if (depth > 0) {
    safePrintLn(std::wstring(path.begin(), path.end()) + L":");
  }

  // List current directory
  auto result = list_directory(path, ctx);
  if (!result) {
    return result;
  }

  // Collect subdirectories for recursion
  std::wstring wpath = utf8_to_wstring(path);
  std::wstring search_path = wpath + L"\\*";

  WIN32_FIND_DATAW find_data;
  HANDLE hFind = FindFirstFileW(search_path.c_str(), &find_data);

  if (hFind == INVALID_HANDLE_VALUE) {
    return true;  // No entries
  }

  std::vector<std::string> subdirs;
  const std::wstring ignore_pattern = get_ignore_pattern(ctx);
  do {
    std::wstring filename = find_data.cFileName;

    if (filename == L"." || filename == L"..") {
      continue;
    }

    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (!should_show_entry(filename, find_data, ctx, ignore_pattern)) {
        continue;
      }
      std::wstring full_path = wpath + L"\\" + filename;
      subdirs.push_back(wstring_to_utf8(full_path));
    }
  } while (FindNextFileW(hFind, &find_data) != 0);

  FindClose(hFind);

  // Recursively list subdirectories
  for (const auto &subdir : subdirs) {
    auto subdir_result = list_directory_recursive(subdir, ctx, depth + 1);
    if (!subdir_result) {
      return subdir_result;
    }

    // Add newline between directories
    if (&subdir != &subdirs.back()) {
      safePrintLn(L"");
    }
  }

  return true;
}

/**
 * @brief Process all paths
 * @param paths Paths to process
 * @param ctx Command context
 * @return Result with success status
 */
auto process_paths(const std::vector<std::string> &paths,
                   const CommandContext<LS_OPTIONS.size()> &ctx)
    -> cp::Result<bool> {
  bool success = true;
  for (size_t i = 0; i < paths.size(); ++i) {
    const auto &path = paths[i];

    // Check if path exists and determine its type
    DWORD attr = INVALID_FILE_ATTRIBUTES;
    std::wstring wpath = utf8_to_wstring(path);
    attr = GetFileAttributesW(wpath.c_str());

    // Print path header if multiple paths
    if (paths.size() > 1) {
      safePrintLn(std::wstring(path.begin(), path.end()) + L":");
    }

    // If path doesn't exist, try to treat it as a pattern
    if (attr == INVALID_FILE_ATTRIBUTES) {
      // Use smart glob expansion from utils:wildcard module
      auto glob_result = glob_expand(wpath);

      if (glob_result.expanded && !glob_result.files.empty()) {
        // Pattern matched, process each match
        std::vector<std::string> matched_files;
        for (const auto &file : glob_result.files) {
          matched_files.push_back(wstring_to_utf8(file));
        }

        // Process matched files
        if (!matched_files.empty()) {
          if (matched_files.size() == 1) {
            // Single file, display it directly
            auto result = list_file(matched_files[0], ctx);
            if (!result) {
              safePrintLn(
                  std::wstring(L"ls: ") +
                  std::wstring(result.error().begin(), result.error().end()));
              success = false;
            }
          } else {
            // Multiple files, determine if they're all in same directory
            bool all_same_dir = true;
            std::wstring first_dir;
            for (const auto &f : matched_files) {
              std::wstring fw = utf8_to_wstring(f);
              size_t last_sep = fw.find_last_of(L"\\/");
              std::wstring dir;
              if (last_sep != std::wstring::npos) {
                dir = fw.substr(0, last_sep);
              } else {
                dir = L".";
              }
              if (first_dir.empty()) {
                first_dir = dir;
              } else if (dir != first_dir) {
                all_same_dir = false;
                break;
              }
            }

            if (all_same_dir && !first_dir.empty()) {
              // All files in same directory, list each file individually
              for (const auto &f : matched_files) {
                auto result = list_file(f, ctx);
                if (!result) {
                  safePrintLn(std::wstring(L"ls: ") +
                              std::wstring(result.error().begin(),
                                           result.error().end()));
                  success = false;
                }
              }
            } else {
              // Files in different directories, list each one
              for (const auto &f : matched_files) {
                auto result = list_file(f, ctx);
                if (!result) {
                  safePrintLn(std::wstring(L"ls: ") +
                              std::wstring(result.error().begin(),
                                           result.error().end()));
                  success = false;
                }
              }
            }
          }
        }
      } else {
        // Not a pattern and doesn't exist, or pattern didn't match
        safePrintLn(std::wstring(L"ls: cannot access '") +
                    std::wstring(path.begin(), path.end()) +
                    L"': No such file or directory");
        success = false;
      }
    } else if (attr & FILE_ATTRIBUTE_DIRECTORY) {
      // Path is a directory
      bool directory_only =
          ctx.get<bool>("-d", false) || ctx.get<bool>("--directory", false);
      if (directory_only) {
        auto result = list_file(path, ctx);
        if (!result) {
          safePrintLn(
              std::wstring(L"ls: ") +
              std::wstring(result.error().begin(), result.error().end()));
          success = false;
        }
      } else {
        bool recursive =
            ctx.get<bool>("-R", false) || ctx.get<bool>("--recursive", false);

        if (recursive) {
          // Recursive listing
          auto result =
              list_directory_recursive(path, ctx, paths.size() > 1 ? 1 : 0);
          if (!result) {
            safePrintLn(
                std::wstring(L"ls: ") +
                std::wstring(result.error().begin(), result.error().end()));
            success = false;
          }
        } else {
          // Normal directory listing
          auto result = list_directory(path, ctx);
          if (!result) {
            safePrintLn(
                std::wstring(L"ls: ") +
                std::wstring(result.error().begin(), result.error().end()));
            success = false;
          }
        }
      }
    } else {
      // Path is a file, list it
      auto result = list_file(path, ctx);
      if (!result) {
        safePrintLn(std::wstring(L"ls: ") +
                    std::wstring(result.error().begin(), result.error().end()));
        success = false;
      }
    }

    // Add newline between paths
    if (i < paths.size() - 1) {
      safePrintLn(L"");
    }
  }
  return success;
}

/**
 * @brief Main pipeline
 * @param ctx Command context
 * @return Result with success status
 */
template <size_t N>
auto process_command(const CommandContext<N> &ctx) -> cp::Result<bool> {
  return validate_arguments(ctx).and_then(
      [&](const std::vector<std::string> &paths) {
        return process_paths(paths, ctx);
      });
}

}  // namespace ls_pipeline

// ======================================================
// Command registration
// ======================================================

REGISTER_COMMAND(
    ls,
    /* cmd_name */ "ls",
    /* cmd_synopsis */ "list directory contents",
    /* cmd_desc */
    "List information about the FILEs (the current directory by default).\n"
    "Sort entries alphabetically if none of -cftuvSUX nor --sort is "
    "specified.\n"
    "\n"
    "With no FILE, list the current directory contents. With a FILE that is a\n"
    "directory, list the files and subdirectories inside that directory.\n"
    "With a FILE that is not a directory, list just that file.\n",
    /* examples */
    "  ls                      List files in current directory\n"
    "  ls -l                   Long listing format\n"
    "  ls -la                  Long listing format including hidden files\n"
    "  ls -lh                  Long listing format with human-readable sizes",
    /* see_also */ "find(1), grep(1), sort(1), wc(1)",
    /* author */ "caomengxuan666",
    /* copyright */ "Copyright © 2026 WinuxCmd",
    /* options */
    LS_OPTIONS) {
  using namespace ls_pipeline;
  using namespace core::pipeline;

  auto result = process_command(ctx);
  if (!result) {
    report_error(result, L"ls");
    return 1;
  }

  return *result ? 0 : 1;
}
