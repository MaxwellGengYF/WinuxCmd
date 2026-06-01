/*
 *  Copyright (c) 2026 [caomengxuan666]
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
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 *
 *  - File: main.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
// src/main.cpp
// Main entry point for WinuxCmd
#include "../core/core.h"
#include "../utils/utils.h"
#include "readline.h"
#include "native_completion.h"
#include "../version/version.h"
#include <TlHelp32.h>
#include <clocale>
#include <cstdlib>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <chrono>
#include <thread>

namespace {
static std::string g_repl_executable_path;
enum class FallbackShell { Cmd, PowerShell };
static FallbackShell g_repl_fallback_shell = FallbackShell::Cmd;
static std::wstring g_repl_powershell_exe = L"powershell.exe";

static std::vector<CompletionItem> getCommandCompletions(
    std::string_view prefix) {
  std::vector<CompletionItem> items;
  auto all = CommandRegistry::getAllCommands();
  items.reserve(all.size() + 64);

  std::unordered_set<std::string> seen;
  seen.reserve(all.size() * 2 + 128);

  for (auto &[name, desc] : all) {
    if (!startsWithCaseInsensitive(name, prefix)) continue;
    std::string key(name);
    if (seen.insert(toLowerAscii(key)).second)
      items.push_back({std::move(key), std::string(desc)});
  }

  bool includePowerShell = (g_repl_fallback_shell == FallbackShell::PowerShell);
  auto native =
      queryNativeCommandCompletionsForShell(prefix, includePowerShell);
  for (const auto &item : native) {
    if (seen.insert(toLowerAscii(item.text)).second)
      items.push_back({item.text, item.hint});
  }

  std::ranges::sort(items, {}, &CompletionItem::text);
  return items;
}

static std::vector<CompletionItem> getWindowsOptionCompletions(
    std::string_view cmd_name, std::string_view prefix) {
  std::vector<CompletionItem> items;
  bool includePowerShell = (g_repl_fallback_shell == FallbackShell::PowerShell);
  auto native =
      queryNativeOptionCompletionsForShell(cmd_name, prefix, includePowerShell);
  items.reserve(native.size());
  for (const auto &item : native) {
    items.push_back({item.text, item.hint});
  }
  return items;
}

static std::vector<CompletionItem> getPathCompletions(
    std::string_view token_prefix) {
  std::vector<CompletionItem> items;
  std::string prefix(token_prefix);

  std::filesystem::path base_dir = ".";
  std::string name_prefix = prefix;
  bool has_dir_part = false;

  // Support both '/' and '\' separators on Windows input.
  size_t slash_pos = prefix.find_last_of("/\\");
  if (slash_pos != std::string::npos) {
    has_dir_part = true;
    std::string dir_part = prefix.substr(0, slash_pos + 1);
    name_prefix = prefix.substr(slash_pos + 1);
    std::error_code ec;
    std::filesystem::path p = std::filesystem::path(dir_part);
    if (p.empty())
      base_dir = ".";
    else
      base_dir = p;
    if (!std::filesystem::exists(base_dir, ec) || ec) return items;
  }

  std::error_code ec;
  if (!std::filesystem::exists(base_dir, ec) || ec) return items;
  if (!std::filesystem::is_directory(base_dir, ec) || ec) return items;

  for (const auto &entry : std::filesystem::directory_iterator(
           base_dir, std::filesystem::directory_options::skip_permission_denied,
           ec)) {
    if (ec) break;
    auto name = entry.path().filename().string();
    if (!startsWithCaseInsensitive(name, name_prefix)) continue;

    std::string candidate;
    if (has_dir_part) {
      candidate = prefix.substr(0, slash_pos + 1) + name;
    } else {
      candidate = name;
    }
    bool is_dir = entry.is_directory(ec) && !ec;
    if (is_dir) candidate += "\\";
    items.push_back({candidate, is_dir ? "Directory" : "File"});
  }

  std::ranges::sort(items, {}, &CompletionItem::text);
  return items;
}

static std::optional<std::wstring> findBashOnWindows() {
  return std::nullopt;
}

static int g_last_exit_code = 0;
static std::unordered_map<std::string, std::string> g_shell_vars;
static std::unordered_map<std::string, std::vector<std::string>> g_shell_arrays;
static std::string g_exit_trap;

struct RedirectInfo;
static int runWithRedirects(const std::string& cmd, const RedirectInfo& redirects,
                            const std::wstring& pipe_input = L"");
static int executeShellLine(const std::string& line);
static HANDLE openFileForRedirect(const std::wstring& file, bool append);
static int runBashFallback(const std::string& line) noexcept;
static bool isWordAt(const std::string& str, size_t pos, const std::string& word);
static size_t findKeyword(const std::string& str, size_t start, const std::string& keyword);
static size_t findMatchingKeyword(const std::string& str, size_t start, const std::string& open, const std::string& close);
static std::string translateUnixPath(std::string arg);
static bool matchGlob(std::string_view text, std::string_view pattern);
static std::vector<std::string> expandGlobs(const std::vector<std::string>& tokens);
static size_t findArithmeticClose(const std::string& str, size_t start);
static long long evalArithmeticExpr(const std::string& expr);
static int executeIfStatement(const std::string& line);
static int executeForLoop(const std::string& line);
static int executeWhileLoop(const std::string& line);

static std::string getEnvVar(const std::string& name) {
  if (name == "HOME") {
    const char* val = std::getenv("HOME");
    if (val) return val;
    val = std::getenv("USERPROFILE");
    if (val) return val;
    return "";
  }
  if (name == "USER") {
    const char* val = std::getenv("USER");
    if (val) return val;
    val = std::getenv("USERNAME");
    if (val) return val;
    return "";
  }
  const char* val = std::getenv(name.c_str());
  if (val) return val;
  auto it = g_shell_vars.find(name);
  if (it != g_shell_vars.end()) return it->second;
  return "";
}

static std::string expandBraceVar(const std::string& expr) {
  if (expr.empty()) return "";
  if (expr[0] == '#') {
    std::string name = expr.substr(1);
    std::string val = getEnvVar(name);
    return std::to_string(val.size());
  }
  size_t bracket = expr.find('[');
  if (bracket != std::string::npos && expr.back() == ']') {
    std::string name = expr.substr(0, bracket);
    std::string index_str = expr.substr(bracket + 1, expr.size() - bracket - 2);
    int index = 0;
    try { index = std::stoi(index_str); } catch (...) {}
    auto it = g_shell_arrays.find(name);
    if (it != g_shell_arrays.end()) {
      if (index >= 0 && index < static_cast<int>(it->second.size())) {
        return it->second[index];
      }
    }
    return "";
  }
  size_t colon = expr.find(':');
  if (colon != std::string::npos) {
    std::string name = expr.substr(0, colon);
    std::string rest = expr.substr(colon + 1);
    size_t colon2 = rest.find(':');
    int offset = 0, length = -1;
    try {
      if (colon2 != std::string::npos) {
        offset = std::stoi(rest.substr(0, colon2));
        length = std::stoi(rest.substr(colon2 + 1));
      } else {
        offset = std::stoi(rest);
      }
    } catch (...) {}
    std::string val = getEnvVar(name);
    if (offset < 0) offset = 0;
    if (offset >= static_cast<int>(val.size())) return "";
    if (length < 0) return val.substr(offset);
    return val.substr(offset, length);
  }
  return getEnvVar(expr);
}

static std::string expandEnvVars(const std::string& str) {
  std::string result;
  result.reserve(str.size() * 2);
  bool in_sq = false, in_dq = false;
  for (size_t i = 0; i < str.size(); ++i) {
    char c = str[i];
    if (c == '\'' && !in_dq) { in_sq = !in_sq; result += c; continue; }
    if (c == '"' && !in_sq) { in_dq = !in_dq; result += c; continue; }
    if (c == '$' && i + 1 < str.size()) {
      if (str[i + 1] == '(' && i + 2 < str.size() && str[i + 2] == '(' && !in_sq) {
        size_t close = findArithmeticClose(str, i + 3);
        if (close != std::string::npos) {
          std::string expr = str.substr(i + 3, close - (i + 3));
          long long val = evalArithmeticExpr(expr);
          result += std::to_string(val);
          i = close + 1;
          continue;
        }
      }
      if (str[i + 1] == '{' && !in_sq) {
        size_t end = str.find('}', i + 2);
        if (end != std::string::npos) {
          std::string var_expr = str.substr(i + 2, end - i - 2);
          result += expandBraceVar(var_expr);
          i = end;
          continue;
        }
      }
      if (!in_sq) {
        if (i + 1 >= str.size() ||
            (!std::isalpha(static_cast<unsigned char>(str[i + 1])) && str[i + 1] != '_')) {
          result += c;
          continue;
        }
        size_t start = i + 1;
        size_t end = start;
        while (end < str.size() &&
               (std::isalnum(static_cast<unsigned char>(str[end])) || str[end] == '_'))
          ++end;
        std::string var = str.substr(start, end - start);
        result += getEnvVar(var);
        i = end - 1;
        continue;
      }
    }
    result += c;
  }
  return result;
}

static std::string expandDollarQuestion(const std::string& str) {
  std::string result;
  bool in_sq = false, in_dq = false;
  for (size_t i = 0; i < str.size(); ++i) {
    char c = str[i];
    if (c == '\'' && !in_dq) { in_sq = !in_sq; result += c; continue; }
    if (c == '"' && !in_sq) { in_dq = !in_dq; result += c; continue; }
    if (c == '$' && i + 1 < str.size() && str[i + 1] == '?' && !in_sq) {
      result += std::to_string(g_last_exit_code);
      i += 1;
      continue;
    }
    result += c;
  }
  return result;
}

static size_t findMatchingParen(const std::string& str, size_t start) {
  int depth = 1;
  bool in_sq = false, in_dq = false;
  for (size_t i = start; i < str.size(); ++i) {
    if (str[i] == '\'' && !in_dq) { in_sq = !in_sq; continue; }
    if (str[i] == '"' && !in_sq) { in_dq = !in_dq; continue; }
    if (in_sq || in_dq) continue;
    if (str[i] == '(') { ++depth; continue; }
    if (str[i] == ')') {
      --depth;
      if (depth == 0) return i;
    }
  }
  return std::string::npos;
}

static size_t findArithmeticClose(const std::string& str, size_t start) {
  int depth = 0;
  bool in_sq = false, in_dq = false;
  for (size_t i = start; i < str.size(); ++i) {
    if (str[i] == '\'' && !in_dq) { in_sq = !in_sq; continue; }
    if (str[i] == '"' && !in_sq) { in_dq = !in_dq; continue; }
    if (in_sq || in_dq) continue;
    if (str[i] == '(') { ++depth; continue; }
    if (str[i] == ')') {
      if (depth > 0) { --depth; continue; }
      if (i + 1 < str.size() && str[i + 1] == ')') return i;
      return std::string::npos;
    }
  }
  return std::string::npos;
}

static long long evalArithmeticExpr(const std::string& expr) {
  struct Parser {
    std::string s;
    size_t pos = 0;
    void skip() {
      while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) ++pos;
    }
    long long parse() {
      long long val = parseAddSub();
      skip();
      return val;
    }
    long long parseAddSub() {
      long long val = parseMulDiv();
      while (true) {
        skip();
        if (pos >= s.size()) break;
        char op = s[pos];
        if (op != '+' && op != '-') break;
        ++pos;
        long long rhs = parseMulDiv();
        if (op == '+') val += rhs;
        else val -= rhs;
      }
      return val;
    }
    long long parseMulDiv() {
      long long val = parsePrimary();
      while (true) {
        skip();
        if (pos >= s.size()) break;
        char op = s[pos];
        if (op != '*' && op != '/' && op != '%') break;
        ++pos;
        long long rhs = parsePrimary();
        if (op == '*') val *= rhs;
        else if (op == '/') val = rhs != 0 ? val / rhs : 0;
        else val = rhs != 0 ? val % rhs : 0;
      }
      return val;
    }
    long long parsePrimary() {
      skip();
      if (pos < s.size() && s[pos] == '(') {
        ++pos;
        long long val = parseAddSub();
        skip();
        if (pos < s.size() && s[pos] == ')') ++pos;
        return val;
      }
      long long val = 0;
      bool neg = false;
      if (pos < s.size() && s[pos] == '-') {
        neg = true;
        ++pos;
      }
      if (pos < s.size() && std::isalpha(static_cast<unsigned char>(s[pos]))) {
        size_t start = pos;
        while (pos < s.size() && (std::isalnum(static_cast<unsigned char>(s[pos])) || s[pos] == '_')) ++pos;
        std::string var = s.substr(start, pos - start);
        std::string var_val = getEnvVar(var);
        try { val = std::stoll(var_val); } catch (...) { val = 0; }
        return neg ? -val : val;
      }
      while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
        val = val * 10 + (s[pos] - '0');
        ++pos;
      }
      return neg ? -val : val;
    }
  };
  Parser p{expr};
  return p.parse();
}

static std::string expandBraces(const std::string& str) {
  size_t brace_open = std::string::npos;
  size_t brace_close = std::string::npos;
  bool in_sq = false, in_dq = false;
  int brace_depth = 0;
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '\'' && !in_dq) { in_sq = !in_sq; continue; }
    if (str[i] == '"' && !in_sq) { in_dq = !in_dq; continue; }
    if (in_sq || in_dq) continue;
    if (str[i] == '{') {
      if (brace_depth == 0) brace_open = i;
      brace_depth++;
    } else if (str[i] == '}') {
      brace_depth--;
      if (brace_depth == 0) {
        brace_close = i;
        break;
      }
    }
  }
  if (brace_open == std::string::npos || brace_close == std::string::npos) return str;
  std::string inner = str.substr(brace_open + 1, brace_close - brace_open - 1);
  bool has_comma = false;
  int inner_depth = 0;
  in_sq = false; in_dq = false;
  for (size_t i = 0; i < inner.size(); ++i) {
    if (inner[i] == '\'' && !in_dq) { in_sq = !in_sq; continue; }
    if (inner[i] == '"' && !in_sq) { in_dq = !in_dq; continue; }
    if (in_sq || in_dq) continue;
    if (inner[i] == '{') { inner_depth++; continue; }
    if (inner[i] == '}') { inner_depth--; continue; }
    if (inner[i] == ',' && inner_depth == 0) { has_comma = true; break; }
  }
  if (!has_comma) return str;
  std::vector<std::string> alts;
  std::string current;
  inner_depth = 0;
  in_sq = false; in_dq = false;
  for (size_t i = 0; i < inner.size(); ++i) {
    if (inner[i] == '\'' && !in_dq) { in_sq = !in_sq; current += inner[i]; continue; }
    if (inner[i] == '"' && !in_sq) { in_dq = !in_dq; current += inner[i]; continue; }
    if (in_sq || in_dq) { current += inner[i]; continue; }
    if (inner[i] == '{') { inner_depth++; current += inner[i]; continue; }
    if (inner[i] == '}') { inner_depth--; current += inner[i]; continue; }
    if (inner[i] == ',' && inner_depth == 0) {
      alts.push_back(current);
      current.clear();
      continue;
    }
    current += inner[i];
  }
  alts.push_back(current);
  std::string prefix = str.substr(0, brace_open);
  std::string suffix = str.substr(brace_close + 1);
  std::string result;
  for (size_t i = 0; i < alts.size(); ++i) {
    if (i > 0) result += ' ';
    result += prefix + alts[i] + suffix;
  }
  return result;
}

static std::string captureCommandOutput(const std::string& cmd) {
  try {
    auto temp_dir = std::filesystem::temp_directory_path();
    static int temp_counter = 0;
    std::wstring temp_file =
        (temp_dir / (L"winuxcmd_cap_" + std::to_wstring(GetCurrentProcessId()) + L"_" +
                     std::to_wstring(++temp_counter) + L".tmp"))
            .wstring();
    HANDLE hOut = openFileForRedirect(temp_file, false);
    if (hOut == INVALID_HANDLE_VALUE) return "";
    set_output_capture_handles(hOut, GetStdHandle(STD_ERROR_HANDLE), true);
    int rc = runBashFallback(cmd);
    (void)rc;
    set_output_capture_handles(nullptr, nullptr, false);
    CloseHandle(hOut);
    std::string output;
    {
      std::ifstream ifs(temp_file, std::ios::binary);
      if (ifs) {
        std::ostringstream oss;
        oss << ifs.rdbuf();
        output = oss.str();
      }
    }
    std::error_code ec;
    std::filesystem::remove(temp_file, ec);
    if (!output.empty() && output.back() == '\n') {
      output.pop_back();
      if (!output.empty() && output.back() == '\r') output.pop_back();
    }
    return output;
  } catch (...) {
    return "";
  }
}

static std::string expandBracesInWords(const std::string& str) {
  std::string result;
  std::string word;
  for (size_t i = 0; i < str.size(); ++i) {
    char c = str[i];
    if (std::isspace(static_cast<unsigned char>(c))) {
      if (!word.empty()) {
        result += expandBraces(word);
        word.clear();
      }
      result += c;
    } else {
      word += c;
    }
  }
  if (!word.empty()) {
    result += expandBraces(word);
  }
  return result;
}

static std::string expandAll(std::string str) {
  while (true) {
    std::string next = expandBracesInWords(str);
    if (next == str) break;
    str = next;
  }
  while (true) {
    bool changed = false;
    // Process substitution <(...)
    size_t proc_sub = str.find("<(");
    if (proc_sub != std::string::npos) {
      size_t close = findMatchingParen(str, proc_sub + 2);
      if (close != std::string::npos) {
        std::string inner = str.substr(proc_sub + 2, close - (proc_sub + 2));
        auto temp_dir = std::filesystem::temp_directory_path();
        static int ps_counter = 0;
        std::wstring temp_file = (temp_dir / (L"winuxcmd_ps_" + std::to_wstring(GetCurrentProcessId()) + L"_" + std::to_wstring(++ps_counter) + L".tmp")).wstring();
        HANDLE hOut = openFileForRedirect(temp_file, false);
        if (hOut != INVALID_HANDLE_VALUE) {
          set_output_capture_handles(hOut, GetStdHandle(STD_ERROR_HANDLE), true);
          runBashFallback(inner);
          set_output_capture_handles(nullptr, nullptr, false);
          CloseHandle(hOut);
          str = str.substr(0, proc_sub) + wstring_to_utf8(temp_file) + str.substr(close + 1);
          changed = true;
        }
      }
    }
    if (changed) continue;
    size_t bt = str.find('`');
    if (bt != std::string::npos) {
      size_t close = str.find('`', bt + 1);
      if (close != std::string::npos) {
        std::string inner = str.substr(bt + 1, close - bt - 1);
        std::string out = captureCommandOutput(inner);
        str = str.substr(0, bt) + out + str.substr(close + 1);
        changed = true;
      }
    }
    if (changed) continue;
    size_t arith = str.find("$((");
    if (arith != std::string::npos) {
      size_t close = findArithmeticClose(str, arith + 3);
      if (close != std::string::npos) {
        std::string expr = str.substr(arith + 3, close - (arith + 3));
        long long val = evalArithmeticExpr(expr);
        str = str.substr(0, arith) + std::to_string(val) + str.substr(close + 2);
        changed = true;
      }
    }
    if (changed) continue;
    size_t cmdsub = str.find("$(");
    while (cmdsub != std::string::npos) {
      if (cmdsub + 2 < str.size() && str[cmdsub + 2] == '(') {
        cmdsub = str.find("$(", cmdsub + 1);
        continue;
      }
      size_t close = findMatchingParen(str, cmdsub + 2);
      if (close != std::string::npos) {
        std::string inner = str.substr(cmdsub + 2, close - (cmdsub + 2));
        std::string out = captureCommandOutput(inner);
        str = str.substr(0, cmdsub) + out + str.substr(close + 1);
        changed = true;
      }
      break;
    }
    if (changed) continue;
    break;
  }
  return str;
}

static bool hasUnhandledBashSyntax(std::string_view line) {
  bool in_sq = false, in_dq = false;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (c == '\'' && !in_dq) { in_sq = !in_sq; continue; }
    if (c == '"' && !in_sq) { in_dq = !in_dq; continue; }
    if (in_sq || in_dq) continue;
    // Backticks and $(...) are now handled
  }
  size_t start = 0;
  while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) ++start;
  auto end = line.find_first_of(" \t;|&<>(){=", start);
  std::string_view keyword = line.substr(start, end - start);
  static const std::unordered_set<std::string_view> keywords = {
      "until", "case", "select"};
  return keywords.count(keyword);
}

static std::vector<std::string> tokenizeCommand(const std::string& line) {
  std::vector<std::string> tokens;
  std::string current;
  bool in_sq = false, in_dq = false;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (c == '\'' && !in_dq) {
      in_sq = !in_sq;
      current += c;
    } else if (c == '"' && !in_sq) {
      in_dq = !in_dq;
      current += c;
    } else if (std::isspace(static_cast<unsigned char>(c)) && !in_sq && !in_dq) {
      if (!current.empty()) {
        tokens.push_back(std::move(current));
        current.clear();
      }
    } else {
      current += c;
    }
  }
  if (!current.empty()) tokens.push_back(std::move(current));
  return tokens;
}

struct RedirectInfo {
  std::wstring stdout_file;
  bool stdout_append = false;
  std::wstring stderr_file;
  bool stderr_append = false;
  std::wstring stdin_file;
  std::wstring here_doc_temp;
};

static RedirectInfo parseRedirects(std::string& cmd) {
  RedirectInfo info;
  std::string result;
  result.reserve(cmd.size());

  auto is_word_boundary = [&](size_t pos) -> bool {
    if (pos == 0) return true;
    return std::isspace(static_cast<unsigned char>(cmd[pos - 1]));
  };

  for (size_t i = 0; i < cmd.size(); ++i) {
    if (cmd[i] == '\'' || cmd[i] == '"') {
      char quote = cmd[i];
      result += cmd[i];
      ++i;
      while (i < cmd.size() && cmd[i] != quote) {
        result += cmd[i];
        ++i;
      }
      if (i < cmd.size()) result += cmd[i];
      continue;
    }

    // Here-string <<<
    if (cmd[i] == '<' && i + 2 < cmd.size() && cmd[i + 1] == '<' && cmd[i + 2] == '<') {
      size_t j = i + 3;
      while (j < cmd.size() && std::isspace(static_cast<unsigned char>(cmd[j]))) ++j;
      std::string word;
      if (j < cmd.size() && (cmd[j] == '"' || cmd[j] == '\'')) {
        char quote = cmd[j];
        ++j;
        while (j < cmd.size() && cmd[j] != quote) {
          word += cmd[j];
          ++j;
        }
        if (j < cmd.size()) ++j;
      } else {
        while (j < cmd.size() && !std::isspace(static_cast<unsigned char>(cmd[j]))) {
          word += cmd[j];
          ++j;
        }
      }
      auto temp_dir = std::filesystem::temp_directory_path();
      static int here_counter = 0;
      std::wstring temp_file =
          (temp_dir / (L"winuxcmd_here_" + std::to_wstring(GetCurrentProcessId()) + L"_" +
                       std::to_wstring(++here_counter) + L".tmp"))
              .wstring();
      {
        std::ofstream ofs(temp_file, std::ios::binary);
        if (ofs) ofs.write(word.data(), static_cast<std::streamsize>(word.size()));
      }
      info.stdin_file = temp_file;
      info.here_doc_temp = temp_file;
      i = j - 1;
      continue;
    }

    // Here-doc << (but not <<<)
    if (cmd[i] == '<' && i + 1 < cmd.size() && cmd[i + 1] == '<' &&
        (i + 2 >= cmd.size() || cmd[i + 2] != '<')) {
      size_t j = i + 2;
      while (j < cmd.size() && std::isspace(static_cast<unsigned char>(cmd[j]))) ++j;
      std::string delimiter;
      if (j < cmd.size() && (cmd[j] == '"' || cmd[j] == '\'')) {
        char quote = cmd[j];
        ++j;
        while (j < cmd.size() && cmd[j] != quote) {
          delimiter += cmd[j];
          ++j;
        }
        if (j < cmd.size()) ++j;
      } else {
        while (j < cmd.size() && !std::isspace(static_cast<unsigned char>(cmd[j]))) {
          delimiter += cmd[j];
          ++j;
        }
      }
      std::string rest = cmd.substr(j);
      size_t content_start = 0;
      while (content_start < rest.size() && rest[content_start] != '\n') {
        if (!std::isspace(static_cast<unsigned char>(rest[content_start]))) break;
        ++content_start;
      }
      if (content_start < rest.size() && rest[content_start] == '\n') ++content_start;

      std::string content;
      size_t search = content_start;
      size_t content_end = std::string::npos;
      while (search < rest.size()) {
        size_t nl = rest.find('\n', search);
        std::string line;
        if (nl == std::string::npos) {
          line = rest.substr(search);
          search = rest.size();
        } else {
          line = rest.substr(search, nl - search);
          search = nl + 1;
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == delimiter) {
          content_end = search;
          break;
        }
        if (!content.empty()) content += '\n';
        content += line;
      }

      auto temp_dir = std::filesystem::temp_directory_path();
      static int here_counter = 0;
      std::wstring temp_file =
          (temp_dir / (L"winuxcmd_here_" + std::to_wstring(GetCurrentProcessId()) + L"_" +
                       std::to_wstring(++here_counter) + L".tmp"))
              .wstring();
      {
        std::ofstream ofs(temp_file, std::ios::binary);
        if (ofs) {
          ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
          if (!content.empty()) ofs.write("\n", 1);
        }
      }
      info.stdin_file = temp_file;
      info.here_doc_temp = temp_file;
      cmd = trimAscii(result);
      i = cmd.size();
      continue;
    }

    bool is_stderr = false;
    bool is_stdout = false;
    bool is_stdin = false;
    bool append = false;
    size_t redirect_start = i;

    if (cmd[i] == '<') {
      is_stdin = true;
    } else if (cmd[i] == '>') {
      is_stdout = true;
    } else if (cmd[i] == '2' && i + 1 < cmd.size() && cmd[i + 1] == '>' &&
               is_word_boundary(i)) {
      is_stderr = true;
    } else {
      result += cmd[i];
      continue;
    }

    size_t j = redirect_start + (is_stderr ? 2 : 1);
    if (j < cmd.size() && cmd[j] == '>') {
      append = true;
      ++j;
    }

    while (j < cmd.size() && std::isspace(static_cast<unsigned char>(cmd[j]))) ++j;

    std::string filename;
    if (j < cmd.size() && (cmd[j] == '"' || cmd[j] == '\'')) {
      char quote = cmd[j];
      ++j;
      while (j < cmd.size() && cmd[j] != quote) {
        filename += cmd[j];
        ++j;
      }
      if (j < cmd.size()) ++j;
    } else {
      while (j < cmd.size() && !std::isspace(static_cast<unsigned char>(cmd[j]))) {
        filename += cmd[j];
        ++j;
      }
    }

    if (is_stderr) {
      info.stderr_file = utf8_to_wstring(translateUnixPath(filename));
      info.stderr_append = append;
    } else if (is_stdout) {
      info.stdout_file = utf8_to_wstring(translateUnixPath(filename));
      info.stdout_append = append;
    } else if (is_stdin) {
      info.stdin_file = utf8_to_wstring(translateUnixPath(filename));
    }

    i = j - 1;
  }

  cmd = trimAscii(result);
  return info;
}


static bool isWordAt(const std::string& str, size_t pos, const std::string& word) {
  if (pos + word.size() > str.size()) return false;
  if (str.compare(pos, word.size(), word) != 0) return false;
  if (pos > 0) {
    char before = str[pos - 1];
    if (!std::isspace(static_cast<unsigned char>(before)) && before != ';') return false;
  }
  if (pos + word.size() < str.size()) {
    char after = str[pos + word.size()];
    if (!std::isspace(static_cast<unsigned char>(after)) && after != ';') return false;
  }
  return true;
}

static size_t findKeyword(const std::string& str, size_t start, const std::string& keyword) {
  bool in_sq = false, in_dq = false;
  int depth = 0;
  for (size_t i = start; i + keyword.size() <= str.size(); ++i) {
    if (str[i] == '\'' && !in_dq) { in_sq = !in_sq; continue; }
    if (str[i] == '"' && !in_sq) { in_dq = !in_dq; continue; }
    if (in_sq || in_dq) continue;
    if (isWordAt(str, i, "if") || isWordAt(str, i, "for") || isWordAt(str, i, "while")) {
      depth++;
    } else if (isWordAt(str, i, "fi") || isWordAt(str, i, "done")) {
      depth--;
    }
    if (depth == 0 && isWordAt(str, i, keyword)) {
      return i;
    }
  }
  return std::string::npos;
}

static size_t findMatchingKeyword(const std::string& str, size_t start, const std::string& open, const std::string& close) {
  bool in_sq = false, in_dq = false;
  int depth = 1;
  for (size_t i = start; i + close.size() <= str.size(); ++i) {
    if (str[i] == '\'' && !in_dq) { in_sq = !in_sq; continue; }
    if (str[i] == '"' && !in_sq) { in_dq = !in_dq; continue; }
    if (in_sq || in_dq) continue;
    if (isWordAt(str, i, open)) {
      depth++;
    } else if (isWordAt(str, i, close)) {
      depth--;
      if (depth == 0) return i;
    }
  }
  return std::string::npos;
}

static std::string translateUnixPath(std::string arg) {
  if (arg.size() >= 2 && arg[0] == '/' && std::isalpha(static_cast<unsigned char>(arg[1]))) {
    if (arg.size() == 2 || arg[2] == '/') {
      char drive = static_cast<char>(std::toupper(static_cast<unsigned char>(arg[1])));
      std::string win_path = std::string(1, drive) + ":";
      if (arg.size() > 2) {
        win_path += arg.substr(2);
      }
      std::replace(win_path.begin(), win_path.end(), '/', '\\');
      return win_path;
    }
  }
  return arg;
}

static bool matchGlob(std::string_view text, std::string_view pattern) {
  size_t ti = 0, pi = 0;
  size_t star = std::string::npos, tstar = 0;
  while (ti < text.size()) {
    if (pi < pattern.size() && (pattern[pi] == '?' || pattern[pi] == text[ti])) {
      ++ti; ++pi;
    } else if (pi < pattern.size() && pattern[pi] == '*') {
      star = pi++;
      tstar = ti;
    } else if (star != std::string::npos) {
      pi = star + 1;
      ti = ++tstar;
    } else {
      return false;
    }
  }
  while (pi < pattern.size() && pattern[pi] == '*') ++pi;
  return pi == pattern.size();
}

static std::vector<std::string> expandGlobs(const std::vector<std::string>& tokens) {
  std::vector<std::string> result;
  for (const auto& tok : tokens) {
    if (tok.find('*') == std::string::npos && tok.find('?') == std::string::npos) {
      result.push_back(tok);
      continue;
    }
    std::filesystem::path p(tok);
    std::filesystem::path dir = p.parent_path();
    std::string pattern = p.filename().string();
    if (dir.empty()) dir = ".";
    bool found = false;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
      if (ec) break;
      std::string name = entry.path().filename().string();
      if (matchGlob(name, pattern)) {
        result.push_back(entry.path().string());
        found = true;
      }
    }
    if (!found) result.push_back(tok);
  }
  return result;
}

static int executeIfStatement(const std::string& line) {
  size_t fi_pos = findMatchingKeyword(line, 2, "if", "fi");
  if (fi_pos == std::string::npos) {
    safeErrorPrintLn("winuxcmd: syntax error: unmatched if");
    return 2;
  }
  std::string body = trimAscii(line.substr(2, fi_pos - 2));
  size_t then_pos = findKeyword(body, 0, "then");
  if (then_pos == std::string::npos) {
    safeErrorPrintLn("winuxcmd: syntax error: missing then");
    return 2;
  }
  std::string cond = trimAscii(body.substr(0, then_pos));
  std::string rest = trimAscii(body.substr(then_pos + 4));
  size_t else_pos = findKeyword(rest, 0, "else");
  std::string then_branch, else_branch;
  if (else_pos != std::string::npos) {
    then_branch = trimAscii(rest.substr(0, else_pos));
    else_branch = trimAscii(rest.substr(else_pos + 4));
  } else {
    then_branch = rest;
  }
  int cond_rc = executeShellLine(expandEnvVars(cond));
  if (cond_rc == 0) {
    return executeShellLine(then_branch);
  } else if (!else_branch.empty()) {
    return executeShellLine(else_branch);
  }
  return 0;
}

static int executeForLoop(const std::string& line) {
  size_t done_pos = findMatchingKeyword(line, 3, "for", "done");
  if (done_pos == std::string::npos) {
    safeErrorPrintLn("winuxcmd: syntax error: unmatched for");
    return 2;
  }
  std::string body = trimAscii(line.substr(3, done_pos - 3));
  size_t in_pos = findKeyword(body, 0, "in");
  if (in_pos == std::string::npos) {
    safeErrorPrintLn("winuxcmd: syntax error: missing in");
    return 2;
  }
  size_t do_pos = findKeyword(body, in_pos + 2, "do");
  if (do_pos == std::string::npos) {
    safeErrorPrintLn("winuxcmd: syntax error: missing do");
    return 2;
  }
  std::string var_part = trimAscii(body.substr(0, in_pos));
  std::string list_part = expandEnvVars(trimAscii(body.substr(in_pos + 2, do_pos - (in_pos + 2))));
  std::string loop_body = trimAscii(body.substr(do_pos + 2));
  if (!loop_body.empty() && loop_body.back() == ';') loop_body.pop_back();
  auto items = tokenizeCommand(list_part);
  for (auto item : items) {
    if (item.size() >= 2 &&
        ((item.front() == '"' && item.back() == '"') ||
         (item.front() == '\'' && item.back() == '\''))) {
      item = item.substr(1, item.size() - 2);
    }
    g_shell_vars[var_part] = item;
    int rc = executeShellLine(loop_body);
    (void)rc;
  }
  return 0;
}

static int executeWhileLoop(const std::string& line) {
  size_t done_pos = findMatchingKeyword(line, 5, "while", "done");
  if (done_pos == std::string::npos) {
    safeErrorPrintLn("winuxcmd: syntax error: unmatched while");
    return 2;
  }
  std::string body = trimAscii(line.substr(5, done_pos - 5));
  size_t do_pos = findKeyword(body, 0, "do");
  if (do_pos == std::string::npos) {
    safeErrorPrintLn("winuxcmd: syntax error: missing do");
    return 2;
  }
  std::string cond = trimAscii(body.substr(0, do_pos));
  std::string loop_body = trimAscii(body.substr(do_pos + 2));
  if (!loop_body.empty() && loop_body.back() == ';') loop_body.pop_back();
  int rc = 0;
  int iterations = 0;
  const int MAX_ITERATIONS = 10000;
  while (iterations < MAX_ITERATIONS) {
    int cond_rc = executeShellLine(expandEnvVars(cond));
    if (cond_rc != 0) break;
    rc = executeShellLine(loop_body);
    iterations++;
  }
  return rc;
}

static int executeSimpleCommand(const std::string& cmd) {
  std::string expanded = expandDollarQuestion(cmd);

  // Handle assignments directly (e.g. arr=(one two three) or VAR=value)
  std::string trimmed = trimAscii(expanded);
  if (!trimmed.empty()) {
    size_t eq = trimmed.find('=');
    if (eq != std::string::npos && eq > 0) {
      bool looks_like_assign = true;
      for (size_t i = 0; i < eq; ++i) {
        if (!std::isalnum(static_cast<unsigned char>(trimmed[i])) && trimmed[i] != '_') {
          looks_like_assign = false;
          break;
        }
      }
      if (looks_like_assign) {
        std::string name = trimmed.substr(0, eq);
        std::string rest = trimAscii(trimmed.substr(eq + 1));
        if (!rest.empty() && rest.front() == '(' && rest.back() == ')') {
          std::string inner = rest.substr(1, rest.size() - 2);
          auto arr_tokens = tokenizeCommand(inner);
          for (auto& t : arr_tokens) {
            if (t.size() >= 2 &&
                ((t.front() == '"' && t.back() == '"') ||
                 (t.front() == '\'' && t.back() == '\''))) {
              t = t.substr(1, t.size() - 2);
            }
          }
          g_shell_arrays[name] = arr_tokens;
          g_last_exit_code = 0;
          return 0;
        } else {
          // Only treat as a pure assignment if every token is an assignment.
          // Otherwise fall through to the inline env-override logic below.
          auto test_tokens = tokenizeCommand(trimmed);
          size_t assign_count = 0;
          for (size_t i = 0; i < test_tokens.size(); ++i) {
            size_t t_eq = test_tokens[i].find('=');
            if (t_eq != std::string::npos && t_eq > 0) {
              bool valid = true;
              for (size_t k = 0; k < t_eq; ++k) {
                if (!std::isalnum(static_cast<unsigned char>(test_tokens[i][k])) &&
                    test_tokens[i][k] != '_') {
                  valid = false;
                  break;
                }
              }
              if (valid) {
                assign_count++;
                continue;
              }
            }
            break;
          }
          if (assign_count == test_tokens.size()) {
            std::string value = expandEnvVars(rest);
            if (value.size() >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                 (value.front() == '\'' && value.back() == '\''))) {
              value = value.substr(1, value.size() - 2);
            }
            g_shell_vars[name] = value;
            g_last_exit_code = 0;
            return 0;
          }
        }
      }
    }
  }

  auto tokens = tokenizeCommand(expanded);
  if (tokens.empty()) return 0;

  // Translate Unix-style paths to Windows paths
  for (auto& t : tokens) {
    t = translateUnixPath(t);
  }

  // Expand globs (skip quoted tokens)
  {
    std::vector<std::string> processed;
    for (auto& tok : tokens) {
      if (tok.size() >= 2 &&
          ((tok.front() == '"' && tok.back() == '"') ||
           (tok.front() == '\'' && tok.back() == '\''))) {
        processed.push_back(tok.substr(1, tok.size() - 2));
      } else {
        auto globs = expandGlobs(std::vector<std::string>{tok});
        for (auto& g : globs) processed.push_back(g);
      }
    }
    tokens = std::move(processed);
  }

  size_t cmd_start = 0;
  std::vector<std::pair<std::string, std::string>> env_overrides;
  for (size_t i = 0; i < tokens.size(); ++i) {
    auto& tok = tokens[i];
    size_t eq = tok.find('=');
    if (eq != std::string::npos && eq > 0) {
      bool valid_name = true;
      for (size_t k = 0; k < eq; ++k) {
        if (!std::isalnum(static_cast<unsigned char>(tok[k])) && tok[k] != '_') {
          valid_name = false;
          break;
        }
      }
      if (valid_name) {
        std::string name = tok.substr(0, eq);
        std::string value = tok.substr(eq + 1);
        if (value.size() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
          value = value.substr(1, value.size() - 2);
        }
        env_overrides.push_back({name, value});
        cmd_start = i + 1;
        continue;
      }
    }
    break;
  }

  if (cmd_start > 0) {
    if (cmd_start >= tokens.size()) {
      for (auto& [name, value] : env_overrides) {
        g_shell_vars[name] = value;
      }
      return 0;
    }
    std::vector<std::pair<std::string, std::string>> old_values;
    for (auto& [name, value] : env_overrides) {
      const char* old = std::getenv(name.c_str());
      if (old) old_values.push_back({name, old});
      else old_values.push_back({name, ""});
      _putenv((name + "=" + value).c_str());
    }
    std::string new_cmd;
    for (size_t i = cmd_start; i < tokens.size(); ++i) {
      if (i > cmd_start) new_cmd += " ";
      new_cmd += tokens[i];
    }
    int rc = executeSimpleCommand(new_cmd);
    for (auto& [name, old_val] : old_values) {
      if (old_val.empty()) {
        _putenv((name + "=").c_str());
      } else {
        _putenv((name + "=" + old_val).c_str());
      }
    }
    return rc;
  }

  std::string cmd_name = tokens[0];
  std::string lowered = toLowerAscii(cmd_name);

  if (lowered == "true") return 0;
  if (lowered == "false") return 1;

  // Intercept bash/sh
  if (lowered == "bash" || lowered == "sh") {
    if (tokens.size() >= 3 && tokens[1] == "-c") {
      // Extract script from original cmd string to preserve quotes
      size_t c_pos = cmd.find("-c");
      if (c_pos != std::string::npos) {
        size_t script_start = c_pos + 2;
        while (script_start < cmd.size() && std::isspace(static_cast<unsigned char>(cmd[script_start]))) ++script_start;
        std::string script = cmd.substr(script_start);
        if (script.size() >= 2 &&
            ((script.front() == '"' && script.back() == '"') ||
             (script.front() == '\'' && script.back() == '\''))) {
          script = script.substr(1, script.size() - 2);
        }
        return executeShellLine(script);
      }
    }
    if (tokens.size() == 2) {
      std::string file = tokens[1];
      if (file.size() >= 2 &&
          ((file.front() == '"' && file.back() == '"') ||
           (file.front() == '\'' && file.back() == '\''))) {
        file = file.substr(1, file.size() - 2);
      }
      std::ifstream ifs(file);
      if (!ifs) {
        safeErrorPrintLn("bash: " + file + ": No such file or directory");
        return 1;
      }
      std::string script((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      return executeShellLine(script);
    }
    return 0;
  }

  if (lowered == "cd" || lowered == "chdir") {
    std::string arg;
    if (tokens.size() > 1) {
      arg = tokens[1];
      if (arg.size() >= 2 &&
          ((arg.front() == '"' && arg.back() == '"') ||
           (arg.front() == '\'' && arg.back() == '\''))) {
        arg = arg.substr(1, arg.size() - 2);
      }
    }
    if (arg.empty()) {
      safePrintLn(std::filesystem::current_path().wstring());
      return 0;
    }
    std::wstring warg = utf8_to_wstring(arg);
    if (!SetCurrentDirectoryW(warg.c_str())) {
      safePrintLn(L"cd: cannot change directory: " + warg);
      return 1;
    }
    // Update PWD environment variable with Unix-style path
    try {
      auto cwd = std::filesystem::current_path();
      std::string pwd_val = wstring_to_utf8(cwd.wstring());
      if (arg == "/") {
        pwd_val = "/";
      } else {
        // Convert C:\ → /, C:\foo → /c/foo
        if (pwd_val.size() >= 2 && pwd_val[1] == ':') {
          char drive = static_cast<char>(std::tolower(static_cast<unsigned char>(pwd_val[0])));
          if (pwd_val.size() == 2) {
            pwd_val = "/" + std::string(1, drive);
          } else {
            std::string rest = pwd_val.substr(2);
            std::replace(rest.begin(), rest.end(), '\\', '/');
            pwd_val = "/" + std::string(1, drive) + rest;
          }
        } else {
          std::replace(pwd_val.begin(), pwd_val.end(), '\\', '/');
        }
      }
      _putenv(("PWD=" + pwd_val).c_str());
    } catch (...) {}
    return 0;
  }

  if (lowered == "pwd") {
    const char* pwd = std::getenv("PWD");
    if (pwd) {
      safePrintLn(utf8_to_wstring(pwd));
      return 0;
    }
  }

  if (cmd_name == "[") {
    if (!tokens.empty() && tokens.back() == "]") {
      tokens.pop_back();
    }
    if (!tokens.empty()) tokens[0] = "test";
    cmd_name = "test";
    lowered = "test";
  }

  // Workaround for test.cpp option-parsing bug: handle numeric comparisons
  // directly so that [ 1 -eq 1 ] and test 1 -eq 1 work correctly.
  if (lowered == "test" && tokens.size() == 4) {
    const std::string& a = tokens[1];
    const std::string& op = tokens[2];
    const std::string& b = tokens[3];
    if (op == "-eq" || op == "-ne" || op == "-lt" || op == "-le" ||
        op == "-gt" || op == "-ge") {
      try {
        long long va = std::stoll(a);
        long long vb = std::stoll(b);
        if (op == "-eq") return va == vb ? 0 : 1;
        if (op == "-ne") return va != vb ? 0 : 1;
        if (op == "-lt") return va < vb ? 0 : 1;
        if (op == "-le") return va <= vb ? 0 : 1;
        if (op == "-gt") return va > vb ? 0 : 1;
        if (op == "-ge") return va >= vb ? 0 : 1;
      } catch (...) {
        return 2;
      }
    }
  }

  // Missing built-ins
  if (lowered == "exit") {
    int code = 0;
    if (tokens.size() > 1) {
      try { code = std::stoi(tokens[1]); } catch (...) {}
    }
    return code;
  }
  if (lowered == "wait") {
    return 0;
  }
  if (lowered == "trap") {
    if (tokens.size() >= 3) {
      std::string action = tokens[1];
      std::string signal = tokens[2];
      // Strip outer quotes from action
      if (action.size() >= 2 && ((action.front() == '\'' && action.back() == '\'') ||
                                 (action.front() == '"' && action.back() == '"'))) {
        action = action.substr(1, action.size() - 2);
      }
      if (signal == "EXIT" || signal == "0") {
        g_exit_trap = action;
      }
    }
    return 0;
  }
  if (lowered == "sleep") {
    int seconds = 1;
    if (tokens.size() > 1) {
      try { seconds = std::stoi(tokens[1]); } catch (...) {}
    }
    if (seconds > 0) {
      std::this_thread::sleep_for(std::chrono::seconds(seconds));
    }
    return 0;
  }
  if (lowered == "seq") {
    int start = 1, end = 1, step = 1;
    if (tokens.size() > 1) {
      try { end = std::stoi(tokens[1]); } catch (...) {}
    }
    if (tokens.size() > 2) {
      try { start = std::stoi(tokens[1]); end = std::stoi(tokens[2]); } catch (...) {}
    }
    if (tokens.size() > 3) {
      try { start = std::stoi(tokens[1]); end = std::stoi(tokens[2]); step = std::stoi(tokens[3]); } catch (...) {}
    }
    for (int i = start; i <= end; i += step) {
      safePrintLn(std::to_string(i));
    }
    return 0;
  }
  if (lowered == "mktemp") {
    auto temp_dir = std::filesystem::temp_directory_path();
    static int mktemp_counter = 0;
    int n = ++mktemp_counter;
    std::wstring wpath = (temp_dir / (L"winuxcmd_tmp_" + std::to_wstring(n))).wstring();
    HANDLE h = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    std::string fake_path = "/tmp/winuxcmd_tmp_" + std::to_string(n);
    safePrintLn(utf8_to_wstring(fake_path));
    return 0;
  }
  if (lowered == "tee") {
    std::vector<std::string> files;
    for (size_t i = 1; i < tokens.size(); ++i) {
      files.push_back(tokens[i]);
    }
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(std::cin, line)) {
      lines.push_back(line);
    }
    for (const auto& l : lines) {
      safePrintLn(utf8_to_wstring(l));
    }
    for (const auto& f : files) {
      std::ofstream ofs(f, std::ios::binary);
      if (ofs) {
        for (size_t i = 0; i < lines.size(); ++i) {
          if (i > 0) ofs << '\n';
          ofs << lines[i];
        }
      }
    }
    std::cin.clear();
    return 0;
  }
  if (lowered == "uniq") {
    std::string line, prev;
    while (std::getline(std::cin, line)) {
      if (line != prev) {
        safePrintLn(utf8_to_wstring(line));
        prev = line;
      }
    }
    std::cin.clear();
    return 0;
  }
  if (lowered == "tr") {
    if (tokens.size() < 3) {
      safeErrorPrintLn("tr: missing operands");
      return 1;
    }
    std::string set1 = tokens[1];
    std::string set2 = tokens.size() > 2 ? tokens[2] : "";
    bool delete_mode = false;
    if (set1 == "-d") {
      delete_mode = true;
      set1 = set2;
      set2 = "";
    }
    auto expandSet = [](const std::string& s) -> std::string {
      std::string out;
      for (size_t i = 0; i < s.size(); ++i) {
        if (i + 2 < s.size() && s[i+1] == '-') {
          char start = s[i];
          char end = s[i+2];
          for (char c = start; c <= end; ++c) out += c;
          i += 2;
        } else {
          out += s[i];
        }
      }
      return out;
    };
    std::string es1 = expandSet(set1);
    std::string es2 = expandSet(set2);
    std::string line;
    while (std::getline(std::cin, line)) {
      std::string out;
      for (char c : line) {
        if (delete_mode) {
          if (es1.find(c) == std::string::npos) out += c;
        } else {
          size_t pos = es1.find(c);
          if (pos != std::string::npos && pos < es2.size()) {
            out += es2[pos];
          } else {
            out += c;
          }
        }
      }
      safePrintLn(utf8_to_wstring(out));
    }
    std::cin.clear();
    return 0;
  }
  if (lowered == "xargs") {
    int narg = 0;
    size_t cmd_idx = 1;
    if (tokens.size() > 2 && tokens[1] == "-n1") {
      narg = 1;
      cmd_idx = 2;
    } else if (tokens.size() > 2 && tokens[1] == "-n") {
      narg = 1;
      cmd_idx = 3;
    }
    std::string xcmd;
    for (size_t i = cmd_idx; i < tokens.size(); ++i) {
      if (i > cmd_idx) xcmd += " ";
      xcmd += tokens[i];
    }
    if (xcmd.empty()) xcmd = "echo";
    std::string input;
    char c;
    while (std::cin.get(c)) {
      input += c;
    }
    std::cin.clear();
    auto words = tokenizeCommand(input);
    if (narg == 1) {
      for (const auto& w : words) {
        int rc = executeSimpleCommand(xcmd + " " + w);
        (void)rc;
      }
    } else {
      std::string all;
      for (const auto& w : words) {
        if (!all.empty()) all += " ";
        all += w;
      }
      if (!all.empty()) {
        return executeSimpleCommand(xcmd + " " + all);
      }
    }
    return 0;
  }
  if (lowered == "sed") {
    if (tokens.size() < 2) {
      safeErrorPrintLn("sed: missing script");
      return 1;
    }
    std::string script = tokens[1];
    if (script.size() >= 2 &&
        ((script.front() == '"' && script.back() == '"') ||
         (script.front() == '\'' && script.back() == '\''))) {
      script = script.substr(1, script.size() - 2);
    }
    std::string old_str, new_str;
    if (script.size() > 2 && script[0] == 's') {
      char delim = script[1];
      size_t p1 = script.find(delim, 2);
      size_t p2 = script.find(delim, p1 + 1);
      if (p1 != std::string::npos && p2 != std::string::npos) {
        old_str = script.substr(2, p1 - 2);
        new_str = script.substr(p1 + 1, p2 - p1 - 1);
      }
    }
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!old_str.empty()) {
        size_t pos = 0;
        while ((pos = line.find(old_str, pos)) != std::string::npos) {
          line = line.substr(0, pos) + new_str + line.substr(pos + old_str.size());
          pos += new_str.size();
        }
      }
      safePrintLn(utf8_to_wstring(line));
    }
    std::cin.clear();
    return 0;
  }
  if (lowered == "awk") {
    int field = 0;
    std::string prog = tokens.size() > 1 ? tokens[1] : "";
    if (prog.size() >= 2 &&
        ((prog.front() == '"' && prog.back() == '"') ||
         (prog.front() == '\'' && prog.back() == '\''))) {
      prog = prog.substr(1, prog.size() - 2);
    }
    size_t dollar = prog.find('$');
    if (dollar != std::string::npos) {
      try { field = std::stoi(prog.substr(dollar + 1)); } catch (...) {}
    }
    std::string line;
    while (std::getline(std::cin, line)) {
      auto fields = tokenizeCommand(line);
      if (field > 0 && field <= static_cast<int>(fields.size())) {
        safePrintLn(utf8_to_wstring(fields[field - 1]));
      }
    }
    std::cin.clear();
    return 0;
  }
  if (lowered == "type") {
    if (tokens.size() < 2) {
      safeErrorPrintLn("type: missing operand");
      return 1;
    }
    std::string file = tokens[1];
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs) {
      safeErrorPrintLn("type: cannot open " + file);
      return 1;
    }
    std::string line;
    while (std::getline(ifs, line)) {
      safePrintLn(utf8_to_wstring(line));
    }
    return 0;
  }
  if (lowered == "set") {
    return executeSimpleCommand("env");
  }
  if (lowered == "findstr") {
    if (tokens.size() < 2) {
      safeErrorPrintLn("findstr: missing pattern");
      return 1;
    }
    std::string pattern = tokens[1];
    std::string line;
    bool found = false;
    while (std::getline(std::cin, line)) {
      if (line.find(pattern) != std::string::npos) {
        safePrintLn(utf8_to_wstring(line));
        found = true;
      }
    }
    std::cin.clear();
    return found ? 0 : 1;
  }

  std::string_view name = cmd_name;
  if (!CommandRegistry::hasCommand(name) && CommandRegistry::hasCommand(lowered)) {
    name = lowered;
  }

  if (CommandRegistry::hasCommand(name)) {
    std::vector<std::string_view> args;
    args.reserve(tokens.size());
    for (auto& t : tokens) args.emplace_back(t);
    return CommandRegistry::dispatch(name, std::span<std::string_view>(args.data() + 1, args.size() - 1));
  }

  safeErrorPrintLn("winuxcmd: command not found: " + cmd_name);
  return 127;
}

static int redirectStdin(const std::wstring& file, int& old_stdin_fd) {
  old_stdin_fd = -1;
  HANDLE hFile = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    return -1;
  }
  int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hFile), _O_RDONLY);
  if (fd == -1) {
    CloseHandle(hFile);
    return -1;
  }
  old_stdin_fd = _dup(_fileno(stdin));
  if (_dup2(fd, _fileno(stdin)) == -1) {
    _close(fd);
    old_stdin_fd = -1;
    return -1;
  }
  _close(fd);
  return 0;
}

static void restoreStdin(int old_stdin_fd) {
  if (old_stdin_fd != -1) {
    _dup2(old_stdin_fd, _fileno(stdin));
    _close(old_stdin_fd);
  }
}

static int redirectStdout(HANDLE hFile, int& old_stdout_fd) {
  old_stdout_fd = -1;
  HANDLE hDup;
  if (!DuplicateHandle(GetCurrentProcess(), hFile, GetCurrentProcess(), &hDup,
                       0, FALSE, DUPLICATE_SAME_ACCESS)) {
    return -1;
  }
  int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hDup), _O_WRONLY | _O_BINARY);
  if (fd == -1) {
    CloseHandle(hDup);
    return -1;
  }
  std::cout.flush();
  old_stdout_fd = _dup(_fileno(stdout));
  if (_dup2(fd, _fileno(stdout)) == -1) {
    _close(fd);
    old_stdout_fd = -1;
    return -1;
  }
  _close(fd);
  return 0;
}

static void restoreStdout(int old_stdout_fd) {
  if (old_stdout_fd != -1) {
    std::cout.flush();
    _dup2(old_stdout_fd, _fileno(stdout));
    _close(old_stdout_fd);
  }
}

static int redirectStderr(HANDLE hFile, int& old_stderr_fd) {
  old_stderr_fd = -1;
  HANDLE hDup;
  if (!DuplicateHandle(GetCurrentProcess(), hFile, GetCurrentProcess(), &hDup,
                       0, FALSE, DUPLICATE_SAME_ACCESS)) {
    return -1;
  }
  int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hDup), _O_WRONLY | _O_BINARY);
  if (fd == -1) {
    CloseHandle(hDup);
    return -1;
  }
  std::cerr.flush();
  old_stderr_fd = _dup(_fileno(stderr));
  if (_dup2(fd, _fileno(stderr)) == -1) {
    _close(fd);
    old_stderr_fd = -1;
    return -1;
  }
  _close(fd);
  return 0;
}

static void restoreStderr(int old_stderr_fd) {
  if (old_stderr_fd != -1) {
    std::cerr.flush();
    _dup2(old_stderr_fd, _fileno(stderr));
    _close(old_stderr_fd);
  }
}

static HANDLE openFileForRedirect(const std::wstring& file, bool append) {
  DWORD access = GENERIC_WRITE;
  DWORD creation = append ? OPEN_ALWAYS : CREATE_ALWAYS;
  HANDLE h = CreateFileW(file.c_str(), access, FILE_SHARE_READ, nullptr, creation,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;
  if (append) {
    SetFilePointer(h, 0, nullptr, FILE_END);
  }
  return h;
}

static int runWithRedirects(const std::string& cmd, const RedirectInfo& redirects,
                            const std::wstring& pipe_input) {
  HANDLE hOut = INVALID_HANDLE_VALUE;
  HANDLE hErr = INVALID_HANDLE_VALUE;
  int old_stdin_fd = -1;
  int old_stdout_fd = -1;
  int old_stderr_fd = -1;
  bool capture_set = false;

  if (!redirects.stdout_file.empty()) {
    hOut = openFileForRedirect(redirects.stdout_file, redirects.stdout_append);
    if (hOut == INVALID_HANDLE_VALUE) {
      safeErrorPrintLn("winuxcmd: cannot open output file");
      return 1;
    }
  }
  if (!redirects.stderr_file.empty()) {
    hErr = openFileForRedirect(redirects.stderr_file, redirects.stderr_append);
    if (hErr == INVALID_HANDLE_VALUE) {
      if (hOut != INVALID_HANDLE_VALUE) CloseHandle(hOut);
      safeErrorPrintLn("winuxcmd: cannot open error file");
      return 1;
    }
  }

  if (hOut != INVALID_HANDLE_VALUE || hErr != INVALID_HANDLE_VALUE) {
    set_output_capture_handles(
        hOut != INVALID_HANDLE_VALUE ? hOut : GetStdHandle(STD_OUTPUT_HANDLE),
        hErr != INVALID_HANDLE_VALUE ? hErr : GetStdHandle(STD_ERROR_HANDLE),
        true);
    capture_set = true;
  }

  if (hOut != INVALID_HANDLE_VALUE) {
    redirectStdout(hOut, old_stdout_fd);
  }
  if (hErr != INVALID_HANDLE_VALUE) {
    redirectStderr(hErr, old_stderr_fd);
  }

  std::wstring stdin_source = pipe_input.empty() ? redirects.stdin_file : pipe_input;
  if (!stdin_source.empty()) {
    if (redirectStdin(stdin_source, old_stdin_fd) != 0) {
      safeErrorPrintLn("winuxcmd: cannot open input file");
      restoreStderr(old_stderr_fd);
      restoreStdout(old_stdout_fd);
      if (capture_set) {
        set_output_capture_handles(nullptr, nullptr, false);
      }
      if (hOut != INVALID_HANDLE_VALUE) CloseHandle(hOut);
      if (hErr != INVALID_HANDLE_VALUE) CloseHandle(hErr);
      return 1;
    }
    std::cin.clear();
    std::cin.sync();
  }

  int rc = executeSimpleCommand(cmd);

  restoreStdin(old_stdin_fd);
  restoreStderr(old_stderr_fd);
  restoreStdout(old_stdout_fd);
  if (capture_set) {
    set_output_capture_handles(nullptr, nullptr, false);
  }
  if (hOut != INVALID_HANDLE_VALUE) CloseHandle(hOut);
  if (hErr != INVALID_HANDLE_VALUE) CloseHandle(hErr);

  if (!redirects.here_doc_temp.empty()) {
    std::error_code ec;
    std::filesystem::remove(redirects.here_doc_temp, ec);
  }

  return rc;
}

static std::vector<std::string> splitPipes(const std::string& cmd) {
  std::vector<std::string> segments;
  std::string current;
  bool in_sq = false, in_dq = false;
  for (char c : cmd) {
    if (c == '\'' && !in_dq) { in_sq = !in_sq; current += c; continue; }
    if (c == '"' && !in_sq) { in_dq = !in_dq; current += c; continue; }
    if (in_sq || in_dq) { current += c; continue; }
    if (c == '|') {
      segments.push_back(trimAscii(current));
      current.clear();
      continue;
    }
    current += c;
  }
  if (!current.empty()) segments.push_back(trimAscii(current));
  return segments;
}

static int executePipeline(const std::vector<std::string>& segments) {
  try {
    auto temp_dir = std::filesystem::temp_directory_path();
    std::wstring temp_a =
        (temp_dir / (L"winuxcmd_pipe_" + std::to_wstring(GetCurrentProcessId()) + L"_a.tmp"))
            .wstring();
    std::wstring temp_b =
        (temp_dir / (L"winuxcmd_pipe_" + std::to_wstring(GetCurrentProcessId()) + L"_b.tmp"))
            .wstring();

    std::wstring* input_file = nullptr;
    std::wstring* output_file = &temp_a;

    int last_rc = 0;

    for (size_t i = 0; i < segments.size(); ++i) {
      std::string cmd = segments[i];
      RedirectInfo redirects = parseRedirects(cmd);
      bool is_last = (i == segments.size() - 1);

      if (input_file) {
        redirects.stdin_file = *input_file;
      }

      if (!is_last) {
        redirects.stdout_file = *output_file;
        redirects.stdout_append = false;
      }

      last_rc = runWithRedirects(cmd, redirects);

      if (!redirects.here_doc_temp.empty()) {
        std::error_code ec;
        std::filesystem::remove(redirects.here_doc_temp, ec);
      }

      if (!is_last) {
        input_file = output_file;
        output_file = (output_file == &temp_a) ? &temp_b : &temp_a;
      }
    }

    std::error_code ec;
    std::filesystem::remove(temp_a, ec);
    std::filesystem::remove(temp_b, ec);

    g_last_exit_code = last_rc;
    return last_rc;
  } catch (...) {
    return 1;
  }
}

static std::vector<std::pair<std::string, char>> splitShellParts(
    const std::string& line) {
  std::vector<std::pair<std::string, char>> parts;
  std::string current;
  bool in_sq = false, in_dq = false;
  int control_depth = 0;
  int paren_depth = 0;

  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (c == '\'' && !in_dq) { in_sq = !in_sq; current += c; continue; }
    if (c == '"' && !in_sq) { in_dq = !in_dq; current += c; continue; }
    if (in_sq || in_dq) { current += c; continue; }

    if (c == '(') { paren_depth++; current += c; continue; }
    if (c == ')') { if (paren_depth > 0) paren_depth--; current += c; continue; }

    if (isWordAt(line, i, "if") || isWordAt(line, i, "for") || isWordAt(line, i, "while")) {
      control_depth++;
    } else if (isWordAt(line, i, "fi") || isWordAt(line, i, "done")) {
      if (control_depth > 0) control_depth--;
    }

    if (control_depth > 0 || paren_depth > 0) {
      current += c;
      continue;
    }

    if (c == ';') {
      parts.emplace_back(trimAscii(current), ';');
      current.clear();
      continue;
    }
    if (i + 1 < line.size() && c == '&' && line[i + 1] == '&') {
      parts.emplace_back(trimAscii(current), 'a');
      current.clear();
      ++i;
      continue;
    }
    if (i + 1 < line.size() && c == '|' && line[i + 1] == '|') {
      parts.emplace_back(trimAscii(current), 'o');
      current.clear();
      ++i;
      continue;
    }
    if (c == '&' && (i + 1 >= line.size() || std::isspace(static_cast<unsigned char>(line[i + 1])))) {
      parts.emplace_back(trimAscii(current), ';');
      current.clear();
      continue;
    }
    current += c;
  }
  if (!current.empty() || !parts.empty()) {
    parts.emplace_back(trimAscii(current), '\0');
  }
  return parts;
}


struct ExitTrapScope {
  std::string saved;
  ExitTrapScope() { saved = g_exit_trap; }
  ~ExitTrapScope() {
    if (!g_exit_trap.empty()) {
      std::string trap_cmd = g_exit_trap;
      g_exit_trap.clear();
      executeShellLine(trap_cmd);
    }
    g_exit_trap = saved;
  }
};

static int executeShellLine(const std::string& line) {
  ExitTrapScope trap_scope;
  std::string trimmed = trimAscii(line);
  if (!trimmed.empty()) {
    size_t eq = trimmed.find('=');
    if (eq != std::string::npos && eq > 0) {
      bool looks_like_assign = true;
      for (size_t i = 0; i < eq; ++i) {
        if (!std::isalnum(static_cast<unsigned char>(trimmed[i])) && trimmed[i] != '_') {
          looks_like_assign = false;
          break;
        }
      }
      if (looks_like_assign) {
        std::string name = trimmed.substr(0, eq);
        std::string rest = trimAscii(trimmed.substr(eq + 1));
        if (!rest.empty() && rest.front() == '(' && rest.back() == ')') {
          std::string inner = rest.substr(1, rest.size() - 2);
          auto arr_tokens = tokenizeCommand(inner);
          for (auto& t : arr_tokens) {
            if (t.size() >= 2 &&
                ((t.front() == '"' && t.back() == '"') ||
                 (t.front() == '\'' && t.back() == '\''))) {
              t = t.substr(1, t.size() - 2);
            }
          }
          g_shell_arrays[name] = arr_tokens;
          g_last_exit_code = 0;
          return 0;
        } else {
          auto test_tokens = tokenizeCommand(trimmed);
          size_t assign_count = 0;
          for (size_t i = 0; i < test_tokens.size(); ++i) {
            size_t t_eq = test_tokens[i].find('=');
            if (t_eq != std::string::npos && t_eq > 0) {
              bool valid = true;
              for (size_t k = 0; k < t_eq; ++k) {
                if (!std::isalnum(static_cast<unsigned char>(test_tokens[i][k])) &&
                    test_tokens[i][k] != '_') {
                  valid = false;
                  break;
                }
              }
              if (valid) {
                assign_count++;
                continue;
              }
            }
            break;
          }
          if (assign_count == test_tokens.size()) {
            std::string value = expandEnvVars(rest);
            if (value.size() >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                 (value.front() == '\'' && value.back() == '\''))) {
              value = value.substr(1, value.size() - 2);
            }
            g_shell_vars[name] = value;
            g_last_exit_code = 0;
            return 0;
          }
        }
      }
    }
  }

  auto parts = splitShellParts(line);
  if (parts.empty()) return 0;

  int rc = 0;
  char pending_op = '\0';

  for (size_t j = 0; j < parts.size(); ++j) {
    bool should_execute = true;
    if (pending_op == 'a' && rc != 0) should_execute = false;
    else if (pending_op == 'o' && rc == 0) should_execute = false;

    if (should_execute) {
      const std::string& cmd_str = parts[j].first;
      if (cmd_str.empty()) {
        rc = 0;
      } else {
        std::string actual_cmd = cmd_str;
        bool negate = false;
        if (!actual_cmd.empty() && actual_cmd.front() == '!') {
          negate = true;
          actual_cmd = trimAscii(actual_cmd.substr(1));
        }

        auto first_word_end = actual_cmd.find_first_of(" \t;");
        std::string first_word = toLowerAscii(actual_cmd.substr(0, first_word_end));
        if (!actual_cmd.empty() && actual_cmd.front() == '(' && actual_cmd.back() == ')') {
          auto saved_cwd = std::filesystem::current_path();
          rc = executeShellLine(trimAscii(actual_cmd.substr(1, actual_cmd.size() - 2)));
          std::error_code ec;
          std::filesystem::current_path(saved_cwd, ec);
        } else if (first_word == "if") {
          rc = executeIfStatement(actual_cmd);
        } else if (first_word == "for") {
          rc = executeForLoop(actual_cmd);
        } else if (first_word == "while") {
          rc = executeWhileLoop(actual_cmd);
        } else if (first_word == "bash" || first_word == "sh") {
          // Don't expand env vars in bash -c script arguments;
          // let executeSimpleCommand extract and evaluate the script.
          rc = executeSimpleCommand(actual_cmd);
          if (negate) {
            rc = (rc == 0) ? 1 : 0;
          }
        } else {
          // Expand braces, arithmetic, command substitution, etc.
          actual_cmd = expandAll(actual_cmd);
          // Expand env vars for simple commands
          actual_cmd = expandEnvVars(actual_cmd);

          // Check for exit
          auto exit_tokens = tokenizeCommand(actual_cmd);
          if (!exit_tokens.empty() && toLowerAscii(exit_tokens[0]) == "exit") {
            int code = 0;
            if (exit_tokens.size() > 1) {
              try { code = std::stoi(exit_tokens[1]); } catch (...) {}
            }
            g_last_exit_code = code;
            return code;
          }

          auto pipe_segments = splitPipes(actual_cmd);
          if (pipe_segments.size() > 1) {
            rc = executePipeline(pipe_segments);
          } else {
            std::string cmd = actual_cmd;
            RedirectInfo redirects = parseRedirects(cmd);
            rc = runWithRedirects(cmd, redirects);
          }
          if (negate) {
            rc = (rc == 0) ? 1 : 0;
          }
          g_last_exit_code = rc;
        }
      }
    }

    pending_op = parts[j].second;
  }

  g_last_exit_code = rc;
  return rc;
}


static int runBashFallback(const std::string& line) noexcept {
  if (line.empty()) return 0;

  if (hasUnhandledBashSyntax(line)) {
    safeErrorPrintLn("winuxcmd: unhandled bash syntax in: " + line);
    return 127;
  }

  std::string trimmed = trimAscii(line);
  auto first_word_end = trimmed.find_first_of(" \t");
  std::string first_word = toLowerAscii(trimmed.substr(0, first_word_end));
  if (first_word == "bash" || first_word == "sh") {
    return executeShellLine(line);
  }

  std::string expanded = expandAll(line);
  return executeShellLine(expanded);
}

}  // namespace

/**
 * @brief Print help information
 * @return Exit code (1 - error)
 */
static int printHelp() noexcept {
  safePrintLn(L"WinuxCmd - Windows Compatible Linux Command Set");
  safePrintLn(L"Usage: winuxcmd <command> [options]...");
  safePrintLn(L"       winuxcmd -c <shell command>");
  safePrintLn(L"");
  safePrintLn(L"Available commands:");

  // Get all registered commands and display them with brief descriptions
  auto commands = CommandRegistry::getAllCommands();
  for (const auto &[cmd_name, cmd_desc] : commands) {
    // Display command name with its brief description
    std::wstring cmd_str = utf8_to_wstring(std::string(cmd_name));
    std::wstring desc_str = utf8_to_wstring(std::string(cmd_desc));
    // Pad command name for alignment
    if (cmd_str.length() < 10) {
      cmd_str.append(10 - cmd_str.length(), L' ');
    }
    safePrintLn(L"  " + cmd_str + L"   " + desc_str);
  }

  safePrintLn(L"");
  safePrintLn(L"Use 'winuxcmd <command> --help' for command-specific help.");
  return 1;
}

static std::wstring buildReplPrompt() noexcept {
  try {
    auto cwd = std::filesystem::current_path().wstring();
    return L"winux " + cwd + L"> ";
  } catch (...) {
    return L"winux > ";
  }
}

static std::string toPowerShellEncodedCommand(const std::wstring &script) {
  // PowerShell -EncodedCommand expects UTF-16LE bytes.
  std::vector<unsigned char> bytes;
  bytes.reserve(script.size() * 2);
  for (wchar_t ch : script) {
    auto u = static_cast<unsigned int>(ch);
    bytes.push_back(static_cast<unsigned char>(u & 0xFF));
    bytes.push_back(static_cast<unsigned char>((u >> 8) & 0xFF));
  }
  return encoding::base64_encode(bytes);
}

static bool isPowerShellProcessName(std::wstring_view name) {
  std::wstring lower(name);
  std::ranges::transform(lower, lower.begin(), [](wchar_t c) {
    return static_cast<wchar_t>(std::towlower(c));
  });
  return lower == L"powershell.exe" || lower == L"pwsh.exe";
}

static std::optional<std::wstring> getProcessExeNameByPid(DWORD pid) {
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE) return std::nullopt;
  PROCESSENTRY32W pe{};
  pe.dwSize = sizeof(pe);
  if (!Process32FirstW(snap, &pe)) {
    CloseHandle(snap);
    return std::nullopt;
  }
  do {
    if (pe.th32ProcessID == pid) {
      std::wstring name = pe.szExeFile;
      CloseHandle(snap);
      return name;
    }
  } while (Process32NextW(snap, &pe));
  CloseHandle(snap);
  return std::nullopt;
}

static std::optional<DWORD> getParentPid(DWORD pid) {
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE) return std::nullopt;
  PROCESSENTRY32W pe{};
  pe.dwSize = sizeof(pe);
  if (!Process32FirstW(snap, &pe)) {
    CloseHandle(snap);
    return std::nullopt;
  }
  do {
    if (pe.th32ProcessID == pid) {
      DWORD ppid = pe.th32ParentProcessID;
      CloseHandle(snap);
      return ppid;
    }
  } while (Process32NextW(snap, &pe));
  CloseHandle(snap);
  return std::nullopt;
}

static void detectReplFallbackShell() {
  g_repl_fallback_shell = FallbackShell::Cmd;
  g_repl_powershell_exe = L"powershell.exe";

  auto maybe_parent_pid = getParentPid(GetCurrentProcessId());
  if (!maybe_parent_pid.has_value()) return;
  auto maybe_parent_name = getProcessExeNameByPid(*maybe_parent_pid);
  if (!maybe_parent_name.has_value()) return;
  if (!isPowerShellProcessName(*maybe_parent_name)) return;

  g_repl_fallback_shell = FallbackShell::PowerShell;
  std::wstring lowered = *maybe_parent_name;
  std::ranges::transform(lowered, lowered.begin(), [](wchar_t c) {
    return static_cast<wchar_t>(std::towlower(c));
  });
  if (lowered == L"pwsh.exe") {
    g_repl_powershell_exe = L"pwsh.exe";
  } else {
    g_repl_powershell_exe = L"powershell.exe";
  }
}

static int runNativeFallback(const std::string &line) noexcept {
  safeErrorPrintLn("winuxcmd: native fallback is disabled (external processes not allowed): " + line);
  return 127;
}

static bool hasShellMeta(std::string_view line) {
  return line.find('|') != std::string_view::npos ||
         line.find('>') != std::string_view::npos ||
         line.find('<') != std::string_view::npos ||
         line.find('&') != std::string_view::npos;
}

// ---------------------------------------------------------------------------
// Bash fallback helpers
// ---------------------------------------------------------------------------


static bool hasBashSyntax(std::string_view line) {
  bool in_sq = false, in_dq = false;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (c == '\'' && !in_dq) {
      in_sq = !in_sq;
      continue;
    }
    if (c == '"' && !in_sq) {
      in_dq = !in_dq;
      continue;
    }
    if (in_sq || in_dq) continue;

    // Outside quotes: check for bash syntax markers
    if (c == ';') return true;
    if (c == '$') return true;
    if (c == '`') return true;
    if (c == '!' && (i == 0 || std::isspace(static_cast<unsigned char>(line[i - 1]))))
      return true;
    if (c == '(' && i == 0) return true;
    if (c == '<' && i + 1 < line.size() &&
        (line[i + 1] == '<' || line[i + 1] == '('))
      return true;
  }

  // Check for bash keywords at the start of the line
  size_t start = 0;
  while (start < line.size() &&
         std::isspace(static_cast<unsigned char>(line[start])))
    ++start;
  auto end = line.find_first_of(" \t;|&<>(){=", start);
  std::string_view keyword = line.substr(start, end - start);
  static const std::unordered_set<std::string_view> bash_keywords = {
      "if",   "for",   "while", "until",
      "case", "select"};
  if (bash_keywords.count(keyword)) return true;

  // Check for brace expansion {a,b,c}
  size_t brace_open = line.find('{', start);
  while (brace_open != std::string_view::npos) {
    size_t brace_close = line.find('}', brace_open);
    if (brace_close != std::string_view::npos) {
      auto inner = line.substr(brace_open + 1, brace_close - brace_open - 1);
      if (inner.find(',') != std::string_view::npos) return true;
    }
    brace_open = line.find('{', brace_open + 1);
  }

  return false;
}

static int runSmartFallback(const std::string& line) noexcept {
  if (line.empty()) return 0;
  if (hasShellMeta(line) || hasBashSyntax(line)) {
    return runBashFallback(line);
  }
  return runNativeFallback(line);
}

static std::optional<std::string> rewriteSudoBuiltinLine(
    const std::string &line) {
  std::istringstream iss(line);
  std::string first;
  std::string second;
  if (!(iss >> first)) return std::nullopt;
  if (toLowerAscii(first) != "sudo") return std::nullopt;
  if (!(iss >> second)) return std::nullopt;
  if (g_repl_executable_path.empty()) return std::nullopt;

  if (!CommandRegistry::hasCommand(second)) {
    auto lowered = toLowerAscii(second);
    if (!CommandRegistry::hasCommand(lowered)) return std::nullopt;
    second = std::move(lowered);
  }

  size_t rest_start = line.find_first_not_of(" \t", first.size());
  if (rest_start == std::string::npos) return std::nullopt;
  std::string rest = line.substr(rest_start);

  auto exe = g_repl_executable_path;
  if (exe.find(' ') != std::string::npos ||
      exe.find('\t') != std::string::npos) {
    exe = "\"" + exe + "\"";
  }

  // Use explicit winuxcmd.exe path so native Windows sudo can elevate and
  // execute built-in commands that do not exist as standalone binaries.
  std::string rewritten = "sudo " + exe + " " + rest;
  return rewritten;
}

static std::string rewritePipeBuiltinsLine(const std::string &line) {
  if (line.find('|') == std::string::npos || g_repl_executable_path.empty()) {
    return line;
  }

  std::vector<std::string> parts;
  size_t start = 0;
  while (start <= line.size()) {
    size_t pos = line.find('|', start);
    if (pos == std::string::npos) {
      parts.push_back(line.substr(start));
      break;
    }
    parts.push_back(line.substr(start, pos - start));
    start = pos + 1;
  }

  for (auto &part : parts) {
    auto trimmed = trimAscii(part);
    if (trimmed.empty()) continue;

    std::istringstream iss(trimmed);
    std::string first;
    if (!(iss >> first)) continue;

    std::string lowered = toLowerAscii(first);
    if (!CommandRegistry::hasCommand(first) &&
        !CommandRegistry::hasCommand(lowered)) {
      continue;
    }

    auto exe = g_repl_executable_path;
    if (exe.find(' ') != std::string::npos ||
        exe.find('\t') != std::string::npos) {
      exe = "\"" + exe + "\"";
    }
    part = exe + " " + trimmed;
  }

  std::string out;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) out += " | ";
    out += trimAscii(parts[i]);
  }
  return out;
}

// -----------------------------------------------------------------------------
// Interactive REPL
// -----------------------------------------------------------------------------

/// Build a completer function that suggests command names (prefix match) and
/// command options (after a space).
static Completer makeCompleter() {
  return [](std::string_view input) -> std::vector<CompletionItem> {
    // Complete only the segment after the last pipe.
    std::string full_input(input);
    size_t seg_start = 0;
    if (auto pipe_pos = full_input.rfind('|'); pipe_pos != std::string::npos) {
      seg_start = pipe_pos + 1;
      while (seg_start < full_input.size() && full_input[seg_start] == ' ')
        ++seg_start;
    }

    std::string prefix_before = full_input.substr(0, seg_start);
    std::string segment = full_input.substr(seg_start);
    auto space = segment.find(' ');

    if (space == std::string::npos) {
      auto items = getCommandCompletions(segment);
      for (auto &item : items) item.text = prefix_before + item.text;
      return items;
    }

    std::string cmd_name = segment.substr(0, space);
    std::string rest = segment.substr(space + 1);
    while (!rest.empty() && rest.front() == ' ') rest.erase(rest.begin());

    size_t token_start_in_rest = 0;
    if (auto pos = rest.find_last_of(" \t"); pos != std::string::npos)
      token_start_in_rest = pos + 1;
    std::string current_token = rest.substr(token_start_in_rest);

    auto buildFullText = [&](std::string_view replacement) {
      std::string replaced = segment.substr(0, space + 1);
      replaced += rest.substr(0, token_start_in_rest);
      replaced += replacement;
      return prefix_before + replaced;
    };

    std::vector<CompletionItem> items;
    std::unordered_set<std::string> seen;
    auto addItem = [&](std::string text, std::string hint) {
      std::string key = toLowerAscii(text);
      if (seen.insert(std::move(key)).second)
        items.push_back({std::move(text), std::move(hint)});
    };

    auto opts = CommandRegistry::getCommandOptions(cmd_name);
    for (auto &opt : opts) {
      for (const std::string *form : {&opt.short_name, &opt.long_name}) {
        if (form->empty()) continue;
        if (current_token.empty() || form->starts_with(current_token)) {
          addItem(buildFullText(*form), opt.description);
        }
      }
    }

    auto nativeOpts = getWindowsOptionCompletions(cmd_name, current_token);
    for (auto &opt : nativeOpts) {
      addItem(buildFullText(opt.text), opt.hint);
    }

    // File/dir completion should not be overshadowed by option completion.
    auto pathItems = getPathCompletions(current_token);
    for (auto &p : pathItems) {
      addItem(buildFullText(p.text), p.hint);
    }

    std::ranges::sort(items, {}, &CompletionItem::text);
    return items;
  };
}
/// Run WinuxCmd in interactive REPL mode.
static void runReplMode() noexcept {
  safePrintLn(
      L"WinuxCmd " + utf8_to_wstring(WinuxCmd::VERSION_STRING) +
      L"  (interactive)  Type 'exit' to quit, '--help' for command list.");
  safePrintLn(
      L"Use Tab for completions; \u2191\u2193 for history; \u2190\u2192 to "
      L"move cursor.\n");

  auto completer = makeCompleter();

  while (true) {
    std::string line = readInteractiveLine(buildReplPrompt(), completer);

    // Ctrl+D sentinel
    if (!line.empty() && line[0] == '\x04') break;

    // Strip leading/trailing whitespace
    auto first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) continue;
    line = line.substr(first);
    auto last = line.find_last_not_of(" \t\r\n");
    if (last != std::string::npos) line = line.substr(0, last + 1);
    if (line.empty()) continue;

    if (line == "exit" || line == "quit") break;

    std::string resolved_line = line;
    if (auto rewritten = rewriteSudoBuiltinLine(resolved_line);
        rewritten.has_value()) {
      resolved_line = *rewritten;
    }

    // REPL does not implement a full shell parser.
    // Route shell-style compound commands to native-shell fallback.
    if (hasBashSyntax(resolved_line)) {
      runBashFallback(resolved_line);
      continue;
    }
    if (hasShellMeta(resolved_line)) {
      runBashFallback(resolved_line);
      continue;
    }

    // Tokenise with basic quoting support
    // Quoted arguments get a \x01 prefix to protect wildcards from expansion
    std::vector<std::string> tokens;
    {
      std::string tok;
      bool in_single_quote = false;
      bool in_double_quote = false;
      bool was_quoted = false;
      for (size_t i = 0; i < resolved_line.size(); ++i) {
        char c = resolved_line[i];
        if (c == '\'' && !in_double_quote) {
          in_single_quote = !in_single_quote;
          was_quoted = true;
        } else if (c == '"' && !in_single_quote) {
          in_double_quote = !in_double_quote;
          was_quoted = true;
        } else if (std::isspace(static_cast<unsigned char>(c)) &&
                   !in_single_quote && !in_double_quote) {
          if (!tok.empty()) {
            if (was_quoted) {
              tokens.push_back("\x01" + tok);
            } else {
              tokens.push_back(std::move(tok));
            }
            tok.clear();
            was_quoted = false;
          }
        } else {
          tok += c;
        }
      }
      if (!tok.empty()) {
        if (was_quoted) {
          tokens.push_back("\x01" + tok);
        } else {
          tokens.push_back(std::move(tok));
        }
      }
    }
    if (tokens.empty()) continue;

    // REPL built-ins: keep cwd changes in current winuxcmd process.
    std::string lower_cmd = toLowerAscii(tokens[0]);
    if (lower_cmd == "cd" || lower_cmd == "chdir") {
      std::string arg = line.substr(tokens[0].size());
      arg = trimAscii(arg);
      if (arg.empty()) {
        safePrintLn(std::filesystem::current_path().wstring());
        continue;
      }
      if (arg.size() >= 2 && ((arg.front() == '"' && arg.back() == '"') ||
                              (arg.front() == '\'' && arg.back() == '\''))) {
        arg = arg.substr(1, arg.size() - 2);
      }
      std::wstring warg = utf8_to_wstring(arg);
      if (!SetCurrentDirectoryW(warg.c_str())) {
        safePrintLn(L"cd: cannot change directory: " + utf8_to_wstring(arg));
      }
      continue;
    }

    if (tokens[0].size() == 2 &&
        std::isalpha(static_cast<unsigned char>(tokens[0][0])) &&
        tokens[0][1] == ':') {
      std::wstring wdrive = utf8_to_wstring(tokens[0]);
      if (!SetCurrentDirectoryW(wdrive.c_str())) {
        safePrintLn(L"cd: cannot change drive: " + wdrive);
      }
      continue;
    }

    std::vector<std::string_view> args;
    args.reserve(tokens.size());
    for (auto &t : tokens) args.emplace_back(t);

    std::string_view cmd_name = args[0];
    std::span<std::string_view> cmd_args(args.data() + 1, args.size() - 1);
    if (CommandRegistry::hasCommand(cmd_name)) {
      CommandRegistry::dispatch(cmd_name, cmd_args);
    } else {
      runBashFallback(resolved_line);
    }
  }

  safePrintLn(L"\nGoodbye!");
}

/**
 * @brief Main function for WinuxCmd
 * @param argc Number of command-line arguments
 * @param argv Array of command-line arguments
 * @return Exit code from the executed command (0 = success, non-zero = error)
 */
int get_command_refs_sum();

int main(int argc, char *argv[]) noexcept {
  // Force linker to include all command object files
  (void)get_command_refs_sum();

  if (argc < 1) {
    return printHelp();
  }
  // Automatically set console or pipe output.
  setupConsoleForUnicode();
  // Always use UTF-8 encoding for this process
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
  std::setlocale(LC_ALL, ".UTF-8");
  detectReplFallbackShell();
  // Get the executable name (stem only)
  std::string self_name = path::get_executable_name(argv[0]);

  // Convert command-line arguments to string_views for efficiency
  std::vector<std::string_view> args;
  args.reserve(argc - 1);
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  // -c <command>: execute a shell command directly and print result
  if (!args.empty() && args[0] == "-c") {
    if (args.size() < 2) {
      safeErrorPrintLn("winuxcmd: -c requires a command to execute");
      return 1;
    }

    // Reconstruct the full command line from all tokens after -c.
    std::string cmd_line;
    for (size_t i = 1; i < args.size(); ++i) {
      if (i > 1) cmd_line.push_back(' ');
      cmd_line.append(args[i]);
    }

    // Shell meta-characters (pipes, redirects, chaining) and bash-specific
    // syntax must be handled by a real shell, not by built-in dispatch.
    if (hasShellMeta(cmd_line) || hasBashSyntax(cmd_line)) {
      return runBashFallback(cmd_line);
    }

    // Try built-in command dispatch first; fall back to native shell.
    //
    // When the entire command is passed as a single quoted string
    // (e.g. `winuxcmd -c "ls -la"`), args[1] contains the full command
    // line.  Tokenise it so that built-in dispatch can see the real
    // command name and arguments.
    std::string_view cmd_name = args[1];
    std::span<std::string_view> cmd_args;
    if (args.size() > 2) {
      cmd_args = std::span<std::string_view>(args.data() + 2, args.size() - 2);
    }

    // Storage that outlives the string_view span built from it.
    std::vector<std::string> tok_storage;
    std::vector<std::string_view> tok_views;

    if (cmd_name.find_first_of(" \t") != std::string_view::npos) {
      // Tokenise the full command line with basic quote support.
      std::string tok;
      bool in_sq = false, in_dq = false;
      for (char c : cmd_name) {
        if (c == '\'' && !in_dq) {
          in_sq = !in_sq;
        } else if (c == '"' && !in_sq) {
          in_dq = !in_dq;
        } else if (!in_sq && !in_dq && (c == ' ' || c == '\t')) {
          if (!tok.empty()) {
            tok_storage.push_back(std::move(tok));
            tok.clear();
          }
        } else {
          tok.push_back(c);
        }
      }
      if (!tok.empty()) tok_storage.push_back(std::move(tok));

      // Build string_views pointing into tok_storage.
      tok_views.reserve(tok_storage.size());
      for (const auto &s : tok_storage) tok_views.push_back(s);

      if (!tok_views.empty()) {
        cmd_name = tok_views[0];
        // Append the original trailing args (if any) after the tokenised ones.
        for (auto a : cmd_args) {
          tok_storage.push_back(std::string(a));
          tok_views.push_back(tok_storage.back());
        }
        cmd_args = std::span<std::string_view>(tok_views).subspan(1);
      }
    }

    if (CommandRegistry::hasCommand(cmd_name)) {
      return CommandRegistry::dispatch(cmd_name, cmd_args);
    }

    // Fall back to bash if available, otherwise cmd/powershell.
    return runBashFallback(cmd_line);
  }

  if (self_name == "winuxcmd") {
    // Mode 1: winuxcmd <command> [args...] (e.g., winuxcmd ls -la)
    if (args.empty()) {
      // Enter interactive REPL when running on a real console, otherwise help.
      HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
      DWORD mode = 0;
      if (isOutputConsole() && GetConsoleMode(hIn, &mode)) {
        g_repl_executable_path = path::get_executable_path(argv[0]);
        runReplMode();
        return 0;
      }
      return printHelp();
    }

    // Machine-readable completion query: winuxcmd --complete-cmd [prefix]
    if (args[0] == "--complete-cmd") {
      std::string_view prefix = args.size() >= 2 ? args[1] : "";
      auto items = getCommandCompletions(prefix);
      for (const auto &item : items) {
        safePrint(item.text);
        safePrint("\t");
        safePrintLn(item.hint);
      }
      return 0;
    }

    // Machine-readable option query: winuxcmd --complete-opt <cmd> [prefix]
    if (args[0] == "--complete-opt" && args.size() >= 2) {
      std::string_view cmd = args[1];
      std::string_view prefix = args.size() >= 3 ? args[2] : "";
      auto opts = CommandRegistry::getCommandOptions(cmd);
      for (auto &opt : opts) {
        for (const std::string *form : {&opt.short_name, &opt.long_name}) {
          if (form->empty()) continue;
          if (prefix.empty() || std::string_view(*form).starts_with(prefix)) {
            safePrint(*form);
            safePrint("\t");
            safePrintLn(opt.description);
          }
        }
      }
      if (opts.empty()) {
        auto items = getWindowsOptionCompletions(cmd, prefix);
        for (const auto &item : items) {
          safePrint(item.text);
          safePrint("\t");
          safePrintLn(item.hint);
        }
      }
      return 0;
    }

    // Check for top-level help flags/alias
    if (args.size() == 1 && (args[0] == "--help" || args[0] == "-h")) {
      return printHelp();
    }

    // Check for version flags
    if (args.size() == 1 && (args[0] == "--version" || args[0] == "-v")) {
      safePrintLn(L"WinuxCmd " + utf8_to_wstring(WinuxCmd::VERSION_STRING));
      return 0;
    }

    if (!args.empty() && args[0] == "help") {
      if (args.size() == 1) return printHelp();
      if (args.size() == 2) {
        std::string topic(args[1]);
        if (CommandRegistry::hasCommand(topic)) {
          CommandRegistry::printHelp(topic);
          return 0;
        }
        std::string lowered = toLowerAscii(topic);
        if (CommandRegistry::hasCommand(lowered)) {
          CommandRegistry::printHelp(lowered);
          return 0;
        }
        safeErrorPrintLn("winuxcmd: no help topic for '" + topic + "'");
        return 1;
      }
      safeErrorPrintLn("winuxcmd: help accepts at most one command name");
      return 1;
    }

    // Extract command name and remaining arguments

    const std::string_view cmd_name = args[0];

    const std::span<std::string_view> cmd_args(args.data() + 1,

                                               args.size() - 1);

    // Check for --version in command arguments
    bool has_version = false;
    for (const auto &arg : cmd_args) {
      if (arg == "--version") {
        has_version = true;
        break;
      }
    }

    if (has_version && CommandRegistry::hasCommand(cmd_name)) {
      safePrintLn(L"WinuxCmd " + utf8_to_wstring(WinuxCmd::VERSION_STRING));
      return 0;
    }

    if (!CommandRegistry::hasCommand(cmd_name)) {
      std::string raw_line;

      raw_line.reserve(64);

      for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) raw_line.push_back(' ');

        raw_line.append(args[i]);
      }

      return runBashFallback(raw_line);
    }

    // Direct execution
    // Dispatch the command to corresponding implementation
    return CommandRegistry::dispatch(cmd_name, cmd_args);
  } else {
    // Mode 2: <command>.exe [args...] (e.g., ls.exe -la)
    // Treat executable name as command name for direct calls
    const std::span<std::string_view> cmd_args(args.data(), args.size());

    // Dispatch the command to corresponding implementation
    return CommandRegistry::dispatch(self_name, cmd_args);
  }
}
