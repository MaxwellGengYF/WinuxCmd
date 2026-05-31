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
 *  - File: tail.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
// include other header after pch.h

#include "core/command_macros.h"

#include "../core/core.h"

#include "../utils/utils.h"

#include "../container/container.h"

#include <chrono>
#include <deque>
#include <thread>

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

auto constexpr TAIL_OPTIONS = std::array{
    OPTION("-c", "--bytes",
           "output the last NUM bytes; or use -c +NUM to output\n"
           "starting with byte NUM of each file",
           STRING_TYPE),
    OPTION("-f", "--follow", "output appended data as the file grows"),
    OPTION("-F", "", "same as --follow=name --retry"),
    OPTION("-n", "--lines",
           "output the last NUM lines, instead of the last 10; or\n"
           "use -n +NUM to skip NUM-1 lines at the start",
           STRING_TYPE),
    OPTION("", "--max-unchanged-stats",
           "with --follow=name, reopen a FILE which has not changed\n"
           "size after N iterations to see if it has been renamed",
           INT_TYPE),
    OPTION("", "--pid",
           "with -f, terminate after process ID, PID dies",
           INT_TYPE),
    OPTION("-q", "--quiet", "never output headers giving file names"),
    OPTION("", "--silent", "never output headers giving file names"),
    OPTION("", "--retry",
           "keep trying to open a file if it is inaccessible"),
    OPTION("-s", "--sleep-interval",
           "with -f, sleep for approximately N seconds between iterations",
           STRING_TYPE),
    OPTION("-v", "--verbose", "always output headers giving file names"),
    OPTION("-z", "--zero-terminated", "line delimiter is NUL, not newline"),
    OPTION("", "--follow=name", "follow file by name, not descriptor"),
    OPTION("", "--debug", "print debug info to stderr")};

namespace tail_pipeline {
namespace cp = core::pipeline;

struct CountSpec {
  std::uintmax_t value = 10;
  bool from_start = false;
};

struct TailConfig {
  bool by_bytes = false;
  CountSpec spec;
  bool quiet = false;
  bool verbose = false;
  bool follow = false;
  bool follow_name = false;
  bool retry = false;
  bool debug = false;
  int max_unchanged_stats = -1;
  char delimiter = '\n';
  std::chrono::milliseconds sleep_interval{1000};
  std::vector<DWORD> pids;
};

auto stream_all(std::istream& in) -> void {
  std::array<char, 8192> buffer{};
  while (in.good()) {
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    auto got = in.gcount();
    if (got <= 0) break;
    safePrint(std::string_view(buffer.data(), static_cast<size_t>(got)));
  }
}

auto suffix_multiplier(std::string_view suffix)
    -> std::optional<std::uintmax_t> {
  static constexpr auto kMultipliers =
      make_constexpr_map<std::string_view, std::uintmax_t>(
          std::array<std::pair<std::string_view, std::uintmax_t>, 35>{
              std::pair{std::string_view{"b"}, 512ULL},
              std::pair{std::string_view{"KB"}, 1000ULL},
              std::pair{std::string_view{"K"}, 1024ULL},
              std::pair{std::string_view{"KiB"}, 1024ULL},
              std::pair{std::string_view{"MB"}, 1000ULL * 1000ULL},
              std::pair{std::string_view{"M"}, 1024ULL * 1024ULL},
              std::pair{std::string_view{"MiB"}, 1024ULL * 1024ULL},
              std::pair{std::string_view{"GB"}, 1000ULL * 1000ULL * 1000ULL},
              std::pair{std::string_view{"G"}, 1024ULL * 1024ULL * 1024ULL},
              std::pair{std::string_view{"GiB"}, 1024ULL * 1024ULL * 1024ULL},
              std::pair{std::string_view{"TB"},
                        1000ULL * 1000ULL * 1000ULL * 1000ULL},
              std::pair{std::string_view{"T"},
                        1024ULL * 1024ULL * 1024ULL * 1024ULL},
              std::pair{std::string_view{"TiB"},
                        1024ULL * 1024ULL * 1024ULL * 1024ULL},
              std::pair{std::string_view{"PB"},
                        1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL},
              std::pair{std::string_view{"P"},
                        1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL},
              std::pair{std::string_view{"PiB"},
                        1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL},
              std::pair{
                  std::string_view{"EB"},
                  1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL},
              std::pair{std::string_view{"E"}, 1024ULL * 1024ULL * 1024ULL *
                                                   1024ULL * 1024ULL * 1024ULL},
              std::pair{
                  std::string_view{"EiB"},
                  1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL},
              std::pair{std::string_view{"ZB"}, 0ULL},
              std::pair{std::string_view{"Z"}, 0ULL},
              std::pair{std::string_view{"ZiB"}, 0ULL},
              std::pair{std::string_view{"YB"}, 0ULL},
              std::pair{std::string_view{"Y"}, 0ULL},
              std::pair{std::string_view{"YiB"}, 0ULL},
              std::pair{std::string_view{"RB"}, 0ULL},
              std::pair{std::string_view{"R"}, 0ULL},
              std::pair{std::string_view{"RiB"}, 0ULL},
              std::pair{std::string_view{"QB"}, 0ULL},
              std::pair{std::string_view{"Q"}, 0ULL},
              std::pair{std::string_view{"QiB"}, 0ULL},
              std::pair{std::string_view{"l"}, 1ULL},
              std::pair{std::string_view{"L"}, 1ULL}});

  if (suffix.empty()) return 1;

  if (auto it = kMultipliers.find(suffix); it != kMultipliers.end()) {
    return it->second;
  }

  return std::nullopt;
}

auto parse_numeric_with_suffix(std::string_view text)
    -> std::optional<std::uintmax_t> {
  if (text.empty()) return std::nullopt;

  size_t i = 0;
  while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
    ++i;
  }
  if (i == 0) return std::nullopt;

  std::uintmax_t base = 0;
  auto [ptr, ec] = std::from_chars(text.data(), text.data() + i, base);
  if (ec != std::errc() || ptr != text.data() + i) return std::nullopt;

  auto mult = suffix_multiplier(text.substr(i));
  if (!mult.has_value()) return std::nullopt;

  if (*mult == 0) {
    if (base == 0) return 0;
    return std::nullopt;
  }

  if (base > (std::numeric_limits<std::uintmax_t>::max() / *mult)) {
    return std::nullopt;
  }

  return base * *mult;
}

auto parse_count_spec(std::string spec_text, std::string_view opt_name)
    -> cp::Result<CountSpec> {
  if (spec_text.empty()) {
    return core::pipeline::unexpected("invalid number of " + std::string(opt_name));
  }

  CountSpec spec;
  if (spec_text[0] == '+') {
    spec.from_start = true;
    spec_text = spec_text.substr(1);
  } else if (spec_text[0] == '-') {
    // Negative count means the same as positive for tail
    spec_text = spec_text.substr(1);
  }

  // Strip trailing 'l' or 'L' for line counts (obsolete syntax)
  if (opt_name == "lines" && !spec_text.empty()) {
    if (spec_text.back() == 'l' || spec_text.back() == 'L') {
      spec_text.pop_back();
    }
  }

  auto parsed = parse_numeric_with_suffix(spec_text);
  if (!parsed.has_value()) {
    return core::pipeline::unexpected("invalid number of " + std::string(opt_name));
  }

  spec.value = *parsed;
  return spec;
}

auto parse_sleep_interval(std::string_view text)
    -> std::optional<std::chrono::milliseconds> {
  if (text.empty()) return std::nullopt;
  std::string s(text);
  char* end = nullptr;
  errno = 0;
  double seconds = std::strtod(s.c_str(), &end);
  if (errno != 0 || end != s.c_str() + s.size() || seconds < 0.0) {
    return std::nullopt;
  }
  auto millis = static_cast<long long>(seconds * 1000.0);
  if (millis < 0) millis = 0;
  return std::chrono::milliseconds(millis);
}

auto output_tail(std::istream& in, const TailConfig& config) -> void {
  if (config.by_bytes) {
    size_t n = static_cast<size_t>(config.spec.value);
    if (config.spec.from_start) {
      size_t skip = n > 0 ? n - 1 : 0;
      std::array<char, 8192> discard{};
      while (skip > 0 && in.good()) {
        size_t chunk = std::min(skip, discard.size());
        in.read(discard.data(), static_cast<std::streamsize>(chunk));
        auto got = in.gcount();
        if (got <= 0) return;
        skip -= static_cast<size_t>(got);
      }
      stream_all(in);
      return;
    }

    if (n == 0) return;
    std::deque<char> trailing;
    std::array<char, 8192> buffer{};
    while (in.good()) {
      in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      auto got = in.gcount();
      if (got <= 0) break;
      for (std::streamsize i = 0; i < got; ++i) {
        trailing.push_back(buffer[static_cast<size_t>(i)]);
        if (trailing.size() > n) trailing.pop_front();
      }
    }
    if (!trailing.empty()) {
      std::string out;
      out.reserve(trailing.size());
      for (char ch : trailing) out.push_back(ch);
      safePrint(out);
    }
    return;
  }

  size_t n = static_cast<size_t>(config.spec.value);
  if (config.spec.from_start) {
    size_t start = n > 0 ? n - 1 : 0;
    if (start == 0) {
      stream_all(in);
      return;
    }

    size_t record_index = 0;
    std::string current;
    char ch = '\0';
    while (in.get(ch)) {
      current.push_back(ch);
      if (ch == config.delimiter) {
        if (record_index >= start) safePrint(current);
        current.clear();
        ++record_index;
      }
    }
    if (!current.empty() && record_index >= start) {
      safePrint(current);
    }
    return;
  }

  if (n == 0) return;
  std::deque<std::string> trailing_records;
  std::string current;
  char ch = '\0';
  while (in.get(ch)) {
    current.push_back(ch);
    if (ch == config.delimiter) {
      trailing_records.push_back(std::move(current));
      current.clear();
      if (trailing_records.size() > n) trailing_records.pop_front();
    }
  }
  if (!current.empty()) {
    trailing_records.push_back(std::move(current));
    if (trailing_records.size() > n) trailing_records.pop_front();
  }
  for (const auto& rec : trailing_records) safePrint(rec);
}

// File state for follow=name
struct FileState {
  bool valid = false;
  std::uintmax_t size = 0;
  DWORD volume_serial = 0;
  DWORD file_index_high = 0;
  DWORD file_index_low = 0;
};

auto get_file_state(const std::string& path) -> FileState {
  FileState state;
  HANDLE h = CreateFileA(path.c_str(), GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return state;

  BY_HANDLE_FILE_INFORMATION info{};
  if (GetFileInformationByHandle(h, &info)) {
    state.valid = true;
    state.size = (static_cast<std::uintmax_t>(info.nFileSizeHigh) << 32) |
                 info.nFileSizeLow;
    state.volume_serial = info.dwVolumeSerialNumber;
    state.file_index_high = info.nFileIndexHigh;
    state.file_index_low = info.nFileIndexLow;
  }
  CloseHandle(h);
  return state;
}

auto file_states_equal(const FileState& a, const FileState& b) -> bool {
  return a.valid == b.valid && a.volume_serial == b.volume_serial &&
         a.file_index_high == b.file_index_high &&
         a.file_index_low == b.file_index_low;
}

auto any_pid_alive(const std::vector<DWORD>& pids) -> bool {
  if (pids.empty()) return true;
  for (auto pid : pids) {
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (h == nullptr) continue;
    DWORD exit_code = 0;
    BOOL ok = GetExitCodeProcess(h, &exit_code);
    CloseHandle(h);
    if (ok && exit_code == STILL_ACTIVE) return true;
  }
  return false;
}

// Read file data using Win32 API with FILE_SHARE_DELETE to allow rename while reading
auto read_file_data_win32(const std::string& path, std::uintmax_t& read_pos,
                          bool is_multi, const TailConfig& config,
                          bool& first_print) -> bool {
  HANDLE h = CreateFileA(path.c_str(), GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return false;

  LARGE_INTEGER size{};
  if (!GetFileSizeEx(h, &size)) {
    CloseHandle(h);
    return false;
  }

  auto end_pos = static_cast<std::uintmax_t>(size.QuadPart);
  if (end_pos > read_pos) {
    LARGE_INTEGER pos{};
    pos.QuadPart = static_cast<LONGLONG>(read_pos);
    if (SetFilePointerEx(h, pos, nullptr, FILE_BEGIN)) {
      if (is_multi) {
        if (!first_print) {
          if (config.delimiter == '\0') {
            safePrint(std::string(1, '\0'));
          } else {
            safePrint("\n");
          }
        }
        safePrint("==> ");
        safePrint(path);
        safePrint(" <==");
        if (config.delimiter == '\0') {
          safePrint(std::string(1, '\0'));
        } else {
          safePrint("\n");
        }
        first_print = false;
      }

      std::array<char, 8192> buffer{};
      DWORD bytes_read = 0;
      std::uintmax_t total_read = 0;
      while (ReadFile(h, buffer.data(), static_cast<DWORD>(buffer.size()),
                      &bytes_read, nullptr) &&
             bytes_read > 0) {
        safePrint(std::string_view(buffer.data(), bytes_read));
        total_read += bytes_read;
      }
      read_pos += total_read;
    }
  } else if (end_pos < read_pos) {
    read_pos = 0;
  }

  CloseHandle(h);
  return true;
}

auto follow_single_file(const std::string& file, const TailConfig& config,
                        bool is_multi, bool& first_print) -> bool {
  std::ifstream fs;
  bool fs_open = false;

  auto open_file = [&]() -> bool {
    fs.close();
    fs.open(file, std::ios::binary);
    if (fs.is_open()) {
      fs_open = true;
      return true;
    }
    fs_open = false;
    return false;
  };

  if (!config.follow_name) {
    if (!open_file()) {
      if (!config.retry) {
        safeErrorPrint("tail: cannot open '");
        safeErrorPrint(file);
        safeErrorPrint("'\n");
        return false;
      }
    }
  }

  auto current_state = get_file_state(file);
  std::uintmax_t read_pos = 0;
  if (fs_open) {
    fs.seekg(0, std::ios::end);
    auto rp = fs.tellg();
    read_pos = (rp < 0) ? 0 : static_cast<std::uintmax_t>(rp);
  } else if (config.follow_name && current_state.valid) {
    read_pos = current_state.size;
  }
  int unchanged_iterations = 0;

  if (config.debug) {
    safeErrorPrint("tail: polling follow implementation\n");
    safeErrorPrint("tail: following by ");
    safeErrorPrint(config.follow_name ? "name\n" : "descriptor\n");
  }

  while (true) {
    if (!any_pid_alive(config.pids)) break;

    // For follow_name, close file before sleeping to allow rename/delete
    if (config.follow_name && fs_open) {
      fs.close();
      fs_open = false;
    }

    std::this_thread::sleep_for(config.sleep_interval);

    if (config.follow_name) {
      FileState new_state = get_file_state(file);

      if (!new_state.valid) {
        // File missing - wait for retry
        if (!config.retry) break;
        continue;
      }

      if (!current_state.valid || !file_states_equal(current_state, new_state)) {
        // File was renamed/recreated - read from beginning
        if (config.debug) {
          safeErrorPrint("tail: reopening '\"");
          safeErrorPrint(file);
          safeErrorPrint("\"'\n");
        }
        read_pos = 0;
        current_state = new_state;
        unchanged_iterations = 0;
        // Read new data immediately
        read_file_data_win32(file, read_pos, is_multi, config, first_print);
        continue;
      }

      // Same file - check if size changed
      if (new_state.size != static_cast<std::uintmax_t>(read_pos)) {
        unchanged_iterations = 0;
        // Read new data
        read_file_data_win32(file, read_pos, is_multi, config, first_print);
      } else {
        ++unchanged_iterations;
        if (config.max_unchanged_stats >= 0 &&
            unchanged_iterations >= config.max_unchanged_stats) {
          // Force recheck state to detect rename
          if (config.debug) {
            safeErrorPrint("tail: reopening '\"");
            safeErrorPrint(file);
            safeErrorPrint("\"'\n");
          }
          FileState reopened_state = get_file_state(file);
          if (!reopened_state.valid ||
              !file_states_equal(current_state, reopened_state)) {
            // File was renamed/recreated
            read_pos = 0;
            current_state = reopened_state;
            unchanged_iterations = 0;
            read_file_data_win32(file, read_pos, is_multi, config, first_print);
          } else {
            // Same file, just reset counter
            unchanged_iterations = 0;
          }
          continue;
        }
      }
    } else {
      // Descriptor mode
      if (!fs_open) {
        if (open_file()) {
          read_pos = 0;
          current_state = get_file_state(file);
        } else if (!config.retry) {
          break;
        } else {
          continue;
        }
      }

      // Check if file still accessible and not truncated
      fs.clear();
      fs.seekg(0, std::ios::end);
      auto end_pos = fs.tellg();
      if (end_pos < 0) {
        // File was deleted or became inaccessible
        fs.close();
        fs_open = false;
        if (!config.retry) break;
        continue;
      }
      auto end_pos_u = static_cast<std::uintmax_t>(end_pos);
      if (end_pos_u < read_pos) {
        // File truncated
        read_pos = 0;
      }

      // Read new data
      fs.clear();
      fs.seekg(static_cast<std::streamoff>(read_pos));
      auto current_pos = fs.tellg();
      fs.seekg(0, std::ios::end);
      end_pos = fs.tellg();
      end_pos_u = static_cast<std::uintmax_t>(end_pos);

      if (end_pos > current_pos) {
        fs.seekg(current_pos);
        if (is_multi) {
          if (!first_print) {
            if (config.delimiter == '\0') {
              safePrint(std::string(1, '\0'));
            } else {
              safePrint("\n");
            }
          }
          safePrint("==> ");
          safePrint(file);
          safePrint(" <==");
          if (config.delimiter == '\0') {
            safePrint(std::string(1, '\0'));
          } else {
            safePrint("\n");
          }
          first_print = false;
        }
        std::array<char, 8192> buffer{};
        while (fs.good()) {
          fs.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
          auto got = fs.gcount();
          if (got <= 0) break;
          safePrint(std::string(buffer.data(), static_cast<size_t>(got)));
        }
        auto rp = fs.tellg();
        read_pos = (rp < 0) ? end_pos_u : static_cast<std::uintmax_t>(rp);
      } else if (end_pos < current_pos) {
        // Truncated
        read_pos = 0;
      }
    }
  }

  return true;
}

template <size_t N>
auto build_config(const CommandContext<N>& ctx) -> cp::Result<TailConfig> {
  TailConfig config;
  config.delimiter = ctx.get<bool>("--zero-terminated", false) ? '\0' : '\n';

  // Determine last count option by iterating occurrences in order
  enum class CountType { None, Bytes, Lines };
  CountType last_count = CountType::None;
  std::string last_count_value;

  // Determine last header option
  enum class HeaderMode { Default, Quiet, Verbose };
  HeaderMode header_mode = HeaderMode::Default;

  for (const auto& occ : ctx.options.occurrences()) {
    if (occ.index >= N) continue;
    const auto& meta = (*ctx.metas)[occ.index];

    if (meta.short_name == "-c" || meta.long_name == "--bytes") {
      if (auto p = std::get_if<std::string>(&occ.value)) {
        last_count = CountType::Bytes;
        last_count_value = *p;
      }
    } else if (meta.short_name == "-n" || meta.long_name == "--lines") {
      if (auto p = std::get_if<std::string>(&occ.value)) {
        last_count = CountType::Lines;
        last_count_value = *p;
      }
    } else if (meta.short_name == "-q" || meta.long_name == "--quiet" ||
               meta.long_name == "--silent") {
      header_mode = HeaderMode::Quiet;
    } else if (meta.short_name == "-v" || meta.long_name == "--verbose") {
      header_mode = HeaderMode::Verbose;
    }
  }

  config.quiet = (header_mode == HeaderMode::Quiet);
  config.verbose = (header_mode == HeaderMode::Verbose);

  // Follow options
  config.follow = ctx.get<bool>("-f", false) || ctx.get<bool>("--follow", false) ||
                  ctx.get<bool>("--follow=name", false);
  config.follow_name = ctx.get<bool>("--follow=name", false);
  if (ctx.get<bool>("-F", false)) {
    config.follow = true;
    config.follow_name = true;
    config.retry = true;
  }
  config.retry = config.retry || ctx.get<bool>("--retry", false);
  config.debug = ctx.get<bool>("--debug", false);

  // Sleep interval
  std::string sleep_arg = ctx.get<std::string>("--sleep-interval", "");
  if (sleep_arg.empty()) sleep_arg = ctx.get<std::string>("-s", "");
  if (!sleep_arg.empty()) {
    auto parsed_sleep = parse_sleep_interval(sleep_arg);
    if (!parsed_sleep) {
      return core::pipeline::unexpected("invalid sleep interval");
    }
    config.sleep_interval = *parsed_sleep;
  }

  // Pid
  auto pid_values = ctx.get_all<int>("--pid");
  for (auto pid : pid_values) {
    if (pid > 0) config.pids.push_back(static_cast<DWORD>(pid));
  }

  // Max unchanged stats
  int max_unchanged = ctx.get<int>("--max-unchanged-stats", -1);
  if (max_unchanged >= 0) config.max_unchanged_stats = max_unchanged;

  // Parse count
  if (last_count != CountType::None) {
    auto spec = parse_count_spec(last_count_value,
                                 last_count == CountType::Bytes ? "bytes" : "lines");
    if (!spec) return core::pipeline::unexpected(spec.error());
    config.by_bytes = (last_count == CountType::Bytes);
    config.spec = *spec;
  }

  return config;
}

}  // namespace tail_pipeline

REGISTER_COMMAND(
    tail, "tail", "tail [OPTION]... [FILE]...",
    "Print the last 10 lines of each FILE to standard output.\n"
    "With more than one FILE, precede each with a header giving the file "
    "name.\n"
    "\n"
    "With no FILE, or when FILE is -, read standard input.",
    "  tail file.txt\n"
    "  tail -n 20 file.txt\n"
    "  tail -20 file.txt\n"
    "  tail -n +5 file.txt\n"
    "  tail +5 file.txt\n"
    "  tail -c 64 file.txt\n"
    "  tail -v a.txt b.txt",
    "head(1), cat(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd", TAIL_OPTIONS) {
  using namespace tail_pipeline;

  auto config_result = build_config(ctx);
  if (!config_result) {
    cp::report_error(config_result, L"tail");
    return 1;
  }
  auto config = *config_result;

  // Use SmallVector for files (max 64 files) - all stack-allocated
  SmallVector<std::string, 64> files{};
  for (auto p : ctx.positionals) {
    std::string file_arg(p);
    if (contains_wildcard(file_arg)) {
      auto glob_result = glob_expand(file_arg);
      if (glob_result.expanded) {
        for (const auto& file : glob_result.files) {
          files.push_back(wstring_to_utf8(file));
        }
        continue;
      }
    }
    files.push_back(file_arg);
  }
  if (files.empty()) files.push_back("-");

  bool any_error = false;
  bool first_print = true;
  bool multi = files.size() > 1;

  // Files to follow after initial tail output
  std::vector<std::string> follow_files;

  for (size_t i = 0; i < files.size(); ++i) {
    const auto& file = files[i];

    bool show_header = config.verbose || (multi && !config.quiet);
    if (show_header) {
      if (!first_print) {
        if (config.delimiter == '\0') {
          safePrint(std::string(1, '\0'));
        } else {
          safePrint("\n");
        }
      }
      safePrint("==> ");
      safePrint(file == "-" ? "standard input" : file);
      safePrint(" <==");
      if (config.delimiter == '\0') {
        safePrint(std::string(1, '\0'));
      } else {
        safePrint("\n");
      }
    }

    if (file == "-") {
      output_tail(std::cin, config);
      if (std::cin.bad()) {
        safeErrorPrint("tail: error reading '-'\n");
        any_error = true;
      }
    } else {
      std::ifstream input(file, std::ios::binary);
      if (!input.is_open()) {
        safeErrorPrint("tail: cannot open '");
        safeErrorPrint(file);
        safeErrorPrint("'\n");
        if (!(config.follow && config.retry)) {
          any_error = true;
        }
        if (config.follow) {
          follow_files.push_back(file);
        }
        first_print = false;
        continue;
      }

      output_tail(input, config);
      if (input.bad()) {
        safeErrorPrint("tail: error reading '");
        safeErrorPrint(file);
        safeErrorPrint("'\n");
        any_error = true;
      }
      input.close();

      if (config.follow) {
        follow_files.push_back(file);
      }
    }

    first_print = false;
  }

  // Follow mode for all files
  if (config.follow && !follow_files.empty()) {
    bool follow_first_print = first_print;

    auto print_header = [&](const std::string& file) {
      if (!follow_first_print) {
        if (config.delimiter == '\0') {
          safePrint(std::string(1, '\0'));
        } else {
          safePrint("\n");
        }
      }
      safePrint("==> ");
      safePrint(file);
      safePrint(" <==");
      if (config.delimiter == '\0') {
        safePrint(std::string(1, '\0'));
      } else {
        safePrint("\n");
      }
      follow_first_print = false;
    };

    if (follow_files.size() == 1) {
      bool ok = follow_single_file(follow_files[0], config, multi, follow_first_print);
      if (!ok) any_error = true;
    } else {
      // Multi-file follow: round-robin monitoring
      struct FollowState {
        std::string path;
        FileState state;
        std::ifstream fs;
        std::streampos read_pos = 0;
        int unchanged_iterations = 0;
        bool valid = false;
      };

      std::vector<FollowState> states;
      for (const auto& f : follow_files) {
        FollowState s;
        s.path = f;
        states.push_back(std::move(s));
      }

      // Initial open with retry
      for (auto& s : states) {
        while (true) {
          s.fs.open(s.path, std::ios::binary);
          if (s.fs.is_open()) {
            s.fs.seekg(0, std::ios::end);
            s.read_pos = s.fs.tellg();
            s.state = get_file_state(s.path);
            s.valid = true;
            break;
          }
          if (!config.retry) {
            safeErrorPrint("tail: cannot open '");
            safeErrorPrint(s.path);
            safeErrorPrint("'\n");
            any_error = true;
            break;
          }
          if (!any_pid_alive(config.pids)) break;
          std::this_thread::sleep_for(config.sleep_interval);
        }
      }

      if (config.debug) {
        safeErrorPrint("tail: polling follow implementation\n");
        safeErrorPrint("tail: following by ");
        safeErrorPrint(config.follow_name ? "name\n" : "descriptor\n");
      }

      while (true) {
        if (!any_pid_alive(config.pids)) break;
        std::this_thread::sleep_for(config.sleep_interval);

        for (auto& s : states) {
          if (!s.valid) {
            if (config.retry) {
              s.fs.open(s.path, std::ios::binary);
              if (s.fs.is_open()) {
                s.fs.seekg(0, std::ios::end);
                s.read_pos = s.fs.tellg();
                s.state = get_file_state(s.path);
                s.valid = true;
                s.unchanged_iterations = 0;
              }
            }
            continue;
          }

          bool need_reopen = false;

          if (config.follow_name) {
            FileState new_state = get_file_state(s.path);
            if (!new_state.valid) {
              need_reopen = true;
            } else if (!file_states_equal(s.state, new_state)) {
              need_reopen = true;
            } else if (new_state.size == static_cast<std::uintmax_t>(s.read_pos)) {
              ++s.unchanged_iterations;
              if (config.max_unchanged_stats >= 0 &&
                  s.unchanged_iterations >= config.max_unchanged_stats) {
                need_reopen = true;
                s.unchanged_iterations = 0;
              }
            } else {
              s.unchanged_iterations = 0;
            }
          }

          if (need_reopen) {
            s.fs.close();
            s.fs.open(s.path, std::ios::binary);
            if (s.fs.is_open()) {
              s.read_pos = 0;
              s.state = get_file_state(s.path);
              s.valid = true;
              s.unchanged_iterations = 0;
            } else {
              s.valid = false;
              if (!config.retry) continue;
            }
          }

          if (!s.valid) continue;

          s.fs.clear();
          s.fs.seekg(s.read_pos);
          auto current_pos = s.fs.tellg();
          s.fs.seekg(0, std::ios::end);
          auto end_pos = s.fs.tellg();

          if (end_pos > current_pos) {
            s.fs.seekg(current_pos);
            print_header(s.path);
            std::array<char, 8192> buffer{};
            while (s.fs.good()) {
              s.fs.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
              auto got = s.fs.gcount();
              if (got <= 0) break;
              safePrint(std::string(buffer.data(), static_cast<size_t>(got)));
            }
            s.read_pos = s.fs.tellg();
            if (s.read_pos < 0) s.read_pos = end_pos;
          } else if (end_pos < current_pos) {
            // Truncated
            s.read_pos = 0;
          }
        }
      }
    }
  }

  return any_error ? 1 : 0;
}
