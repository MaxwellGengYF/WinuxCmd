/*
 *  Copyright © 2026 WinuxCmd
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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 *  - File: find.cpp
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - @contributor1 WinuxCmd
/// @Description: Implementation for find command.
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright © 2026 WinuxCmd
// include other header after pch.h
#include "core/command_macros.h"

#include "../core/core.h"
#include "../utils/utils.h"
#include "../container/container.h"

#include <regex>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

auto constexpr FIND_OPTIONS = std::array{
    OPTION("-name", "", "base of file name matches shell pattern PATTERN",
           STRING_TYPE),
    OPTION("-iname", "", "like -name, but the match is case insensitive",
           STRING_TYPE),
    OPTION("-path", "", "full path matches shell pattern PATTERN",
           STRING_TYPE),
    OPTION("-ipath", "", "like -path, but the match is case insensitive",
           STRING_TYPE),
    OPTION("-type", "", "file is of type c: b,d,p,f,l,s,D", STRING_TYPE),
    OPTION("-size", "", "file uses n units of space", STRING_TYPE),
    OPTION("-empty", "", "file is empty", BOOL_TYPE),
    OPTION("-mmin", "", "file's data was last modified n minutes ago",
           STRING_TYPE),
    OPTION("-mtime", "", "file's data was last modified n*24 hours ago",
           STRING_TYPE),
    OPTION("-true", "", "always true", BOOL_TYPE),
    OPTION("-false", "", "always false", BOOL_TYPE),
    OPTION("-regex", "", "full path matches regular expression", STRING_TYPE),
    OPTION("-iregex", "",
           "like -regex, but the match is case insensitive", STRING_TYPE),
    OPTION("-newer", "", "file was modified more recently than file",
           STRING_TYPE),
    OPTION("-mindepth", "",
           "descend at least LEVELS levels of directories before tests",
           INT_TYPE),
    OPTION("-maxdepth", "",
           "descend at most LEVELS levels of directories below starting-points",
           INT_TYPE),
    OPTION("-depth", "", "process each directory's contents before directory",
           BOOL_TYPE),
    OPTION("-print", "", "print the full file name on the standard output",
           BOOL_TYPE),
    OPTION("-print0", "",
           "print the full file name on the standard output, followed by a "
           "null character",
           BOOL_TYPE),
    OPTION("-printf", "", "print format", STRING_TYPE),
    OPTION("-exec", "", "execute command", TERMINATED_STRING_TYPE),
    OPTION("-ok", "", "execute command after confirmation",
           TERMINATED_STRING_TYPE),
    OPTION("-delete", "", "delete files", BOOL_TYPE),
    OPTION("-prune", "", "prune tree", BOOL_TYPE),
    OPTION("-quit", "", "exit immediately", BOOL_TYPE),
    OPTION("-L", "", "follow symbolic links"),
    OPTION("-H", "", "do not follow symbolic links, except while processing "
                     "command line arguments"),
    OPTION("-P", "", "never follow symbolic links (default)"),
    OPTION("-o", "", "or", BOOL_TYPE),
    OPTION("-or", "", "or", BOOL_TYPE),
    OPTION("-a", "", "and", BOOL_TYPE),
    OPTION("-and", "", "and", BOOL_TYPE),
    OPTION("-not", "", "not", BOOL_TYPE)};

namespace find_pipeline {

// Stubs for missing validators (needed to compile)


namespace cp = core::pipeline;

// ============================================================================
// AST
// ============================================================================

enum class NodeType {
  Name,
  Iname,
  Path,
  Ipath,
  Type,
  Size,
  Empty,
  Mmin,
  Mtime,
  True,
  False,
  Regex,
  Iregex,
  Newer,
  Print,
  Print0,
  Printf,
  Exec,
  Ok,
  Delete,
  Prune,
  Quit,
  Not,
  And,
  Or
};

struct Node {
  NodeType type;
  std::string value;                // for primaries that take a string
  std::vector<std::string> exec_args;  // for -exec / -ok
  bool exec_plus = false;              // true for +, false for ;
  std::unique_ptr<Node> left;
  std::unique_ptr<Node> right;

  explicit Node(NodeType t) : type(t) {}
  Node(NodeType t, std::string v) : type(t), value(std::move(v)) {}
  Node(NodeType t, std::vector<std::string> args, bool plus)
      : type(t), exec_args(std::move(args)), exec_plus(plus) {}
};

// ============================================================================
// Tokenization
// ============================================================================

enum class TokenType { Primary, Operator };

struct Token {
  TokenType type;
  std::string name;                 // option name or operator symbol
  std::string value;                // string value for STRING/INT options
  std::vector<std::string> exec_args;  // for -exec / -ok manual parsing
  bool exec_plus = false;
};

auto is_known_option(std::string_view arg,
                     const std::array<OptionMeta, FIND_OPTIONS.size()>& metas)
    -> bool {
  for (const auto& m : metas) {
    if (m.short_name == arg || m.long_name == arg) return true;
  }
  return false;
}

auto find_meta(std::string_view arg,
               const std::array<OptionMeta, FIND_OPTIONS.size()>& metas)
    -> const OptionMeta* {
  for (const auto& m : metas) {
    if (m.short_name == arg || m.long_name == arg) return &m;
  }
  return nullptr;
}

auto tokenize_expression(
    const std::vector<std::string_view>& raw_args,
    const std::array<OptionMeta, FIND_OPTIONS.size()>& metas)
    -> cp::Result<std::pair<std::vector<std::string>, std::vector<Token>>> {
  std::vector<std::string> paths;
  std::vector<Token> tokens;

  // Determine where expression starts:
  // Global options and paths can be interleaved at the beginning.
  // Expression starts at the first operator or non-global primary.
  size_t i = 0;
  while (i < raw_args.size()) {
    auto arg = raw_args[i];
    // Global options and their values are skipped
    if (auto meta = find_meta(arg, metas); meta) {
      if (arg == "-maxdepth" || arg == "-mindepth" || arg == "-depth" ||
          arg == "-L" || arg == "-H" || arg == "-P") {
        if (meta->type != OptionType::Bool) {
          if (i + 1 >= raw_args.size()) {
            return cp::unexpected("missing argument for " + std::string(arg));
          }
          i += 2;
        } else {
          ++i;
        }
        continue;
      }
    }
    // Operators or non-global primaries start the expression
    if (arg == "!" || arg == "(" || arg == ")" || is_known_option(arg, metas)) {
      break;
    }
    // Otherwise it's a path
    paths.emplace_back(arg);
    ++i;
  }

  // Parse expression tokens
  while (i < raw_args.size()) {
    auto arg = raw_args[i];
    if (arg == "!" || arg == "(" || arg == ")" ||
        arg == "-o" || arg == "-or" || arg == "-a" || arg == "-and" ||
        arg == "-not") {
      tokens.push_back({TokenType::Operator, std::string(arg)});
      ++i;
    } else if (arg == "-exec" || arg == "-ok") {
      std::vector<std::string> exec_args;
      bool plus = false;
      bool terminated = false;
      ++i;
      while (i < raw_args.size()) {
        auto a = std::string(raw_args[i]);
        if (a == ";") {
          terminated = true;
          ++i;
          break;
        }
        if (a == "+") {
          plus = true;
          terminated = true;
          ++i;
          break;
        }
        exec_args.push_back(a);
        ++i;
      }
      if (!terminated) {
        return cp::unexpected("missing argument to `" + std::string(arg) + "'");
      }
      tokens.push_back(
          {TokenType::Primary, std::string(arg), "", exec_args, plus});
    } else if (auto meta = find_meta(arg, metas); meta) {
      // Skip any remaining global options inside expression
      if (arg == "-maxdepth" || arg == "-mindepth" || arg == "-depth" ||
          arg == "-L" || arg == "-H" || arg == "-P") {
        if (meta->type != OptionType::Bool) {
          if (i + 1 >= raw_args.size()) {
            return cp::unexpected("missing argument for " + std::string(arg));
          }
          i += 2;
        } else {
          ++i;
        }
        continue;
      }
      if (meta->type == OptionType::Bool) {
        tokens.push_back({TokenType::Primary, std::string(arg)});
        ++i;
      } else {
        if (i + 1 >= raw_args.size()) {
          return cp::unexpected("missing argument for " + std::string(arg));
        }
        tokens.push_back(
            {TokenType::Primary, std::string(arg), std::string(raw_args[i + 1])});
        i += 2;
      }
    } else {
      return cp::unexpected("unknown token: " + std::string(arg));
    }
  }

  return std::make_pair(std::move(paths), std::move(tokens));
}

// Forward declarations for validators used in parse_primary
auto parse_size(std::string_view spec)
    -> cp::Result<std::tuple<int64_t, char, char>>;
auto parse_signed_number(std::string_view spec)
    -> cp::Result<std::pair<int64_t, char>>;

// ============================================================================
// Parser (recursive descent)
// ============================================================================

struct Parser {
  const std::vector<Token>& tokens;
  size_t pos = 0;

  auto peek() -> const Token* {
    if (pos < tokens.size()) return &tokens[pos];
    return nullptr;
  }

  auto consume() -> const Token* {
    if (pos < tokens.size()) return &tokens[pos++];
    return nullptr;
  }

  auto parse_expression() -> cp::Result<std::unique_ptr<Node>> {
    return parse_or();
  }

  auto parse_or() -> cp::Result<std::unique_ptr<Node>> {
    auto left = parse_and();
    if (!left) return left;

    while (auto t = peek()) {
      if (t->name != "-o" && t->name != "-or") break;
      consume();
      auto right = parse_and();
      if (!right) return right;
      auto node = std::make_unique<Node>(NodeType::Or);
      node->left = std::move(*left);
      node->right = std::move(*right);
      left = std::move(node);
    }
    return left;
  }

  auto parse_and() -> cp::Result<std::unique_ptr<Node>> {
    auto left = parse_unary();
    if (!left) return left;

    while (auto t = peek()) {
      if (t->name == "-a" || t->name == "-and") {
        consume();
      } else if (t->type == TokenType::Primary ||
                 (t->type == TokenType::Operator &&
                  (t->name == "!" || t->name == "-not" || t->name == "("))) {
        // implicit AND
      } else if (t->name == "-o" || t->name == "-or") {
        break;
      } else if (t->name == ")") {
        break;
      } else {
        break;
      }

      if (t->name == "-a" || t->name == "-and" || t->type == TokenType::Primary ||
          t->name == "!" || t->name == "-not" || t->name == "(") {
        auto right = parse_unary();
        if (!right) return right;
        auto node = std::make_unique<Node>(NodeType::And);
        node->left = std::move(*left);
        node->right = std::move(*right);
        left = std::move(node);
      } else {
        break;
      }
    }
    return left;
  }

  auto parse_unary() -> cp::Result<std::unique_ptr<Node>> {
    auto t = peek();
    if (t && (t->name == "!" || t->name == "-not")) {
      consume();
      auto child = parse_unary();
      if (!child) return child;
      auto node = std::make_unique<Node>(NodeType::Not);
      node->left = std::move(*child);
      return node;
    }
    return parse_primary();
  }

  auto parse_primary() -> cp::Result<std::unique_ptr<Node>> {
    auto t = peek();
    if (!t) return cp::unexpected("expected primary, got end of expression");

    if (t->name == "(") {
      consume();
      auto inner = parse_expression();
      if (!inner) return inner;
      auto close = peek();
      if (!close || close->name != ")") {
        return cp::unexpected("missing closing parenthesis");
      }
      consume();
      return inner;
    }

    if (t->type != TokenType::Primary) {
      return cp::unexpected("expected primary, got " + t->name);
    }

    consume();
    auto name = t->name;
    if (name == "-name")
      return std::make_unique<Node>(NodeType::Name, t->value);
    if (name == "-iname")
      return std::make_unique<Node>(NodeType::Iname, t->value);
    if (name == "-path")
      return std::make_unique<Node>(NodeType::Path, t->value);
    if (name == "-ipath")
      return std::make_unique<Node>(NodeType::Ipath, t->value);
    if (name == "-type")
      return std::make_unique<Node>(NodeType::Type, t->value);
    if (name == "-size") {
      if (!parse_size(t->value))
        return cp::unexpected("invalid size: " + t->value);
      return std::make_unique<Node>(NodeType::Size, t->value);
    }
    if (name == "-mmin") {
      if (!parse_signed_number(t->value))
        return cp::unexpected("invalid time: " + t->value);
      return std::make_unique<Node>(NodeType::Mmin, t->value);
    }
    if (name == "-mtime") {
  auto parsed = parse_signed_number(t->value);
      if (!parsed) return cp::unexpected("invalid time: " + t->value);
      return std::make_unique<Node>(NodeType::Mtime, t->value);
    }
    if (name == "-empty")
      return std::make_unique<Node>(NodeType::Empty);
    if (name == "-true")
      return std::make_unique<Node>(NodeType::True);
    if (name == "-false")
      return std::make_unique<Node>(NodeType::False);
    if (name == "-regex")
      return std::make_unique<Node>(NodeType::Regex, t->value);
    if (name == "-iregex")
      return std::make_unique<Node>(NodeType::Iregex, t->value);
    if (name == "-newer")
      return std::make_unique<Node>(NodeType::Newer, t->value);
    if (name == "-print")
      return std::make_unique<Node>(NodeType::Print);
    if (name == "-print0")
      return std::make_unique<Node>(NodeType::Print0);
    if (name == "-printf")
      return std::make_unique<Node>(NodeType::Printf, t->value);
    if (name == "-exec")
      return std::make_unique<Node>(NodeType::Exec, t->exec_args, t->exec_plus);
    if (name == "-ok")
      return std::make_unique<Node>(NodeType::Ok, t->exec_args, t->exec_plus);
    if (name == "-delete")
      return std::make_unique<Node>(NodeType::Delete);
    if (name == "-prune")
      return std::make_unique<Node>(NodeType::Prune);
    if (name == "-quit")
      return std::make_unique<Node>(NodeType::Quit);

    return cp::unexpected("unknown primary: " + name);
  }
};

// ============================================================================
// Global configuration
// ============================================================================

struct Config {
  std::vector<std::string> roots;
  int mindepth = 0;
  int maxdepth = std::numeric_limits<int>::max();
  bool depth = false;
  bool follow_symlinks = false;
  bool had_error = false;
  bool global_quit = false;
  bool prune_next = false;
  bool output_action_present = false;
  bool delete_present = false;
  bool default_print_suppressed = false;
  std::unique_ptr<Node> ast;
  std::filesystem::file_time_type now;

  // -exec + batching
  struct ExecBatch {
    std::string command;
    std::vector<std::string> template_args;
    std::vector<std::string> paths;
  };
  std::vector<ExecBatch> exec_batches;
};

// ============================================================================
// Helpers
// ============================================================================

auto path_display(const std::filesystem::path& p) -> std::string {
  auto s = p.generic_string();
  if (s.empty()) return ".";
  return s;
}

auto depth_from_root(const std::filesystem::path& root,
                     const std::filesystem::path& p) -> int {
  std::error_code ec;
  auto rel = std::filesystem::relative(p, root, ec);
  if (ec || rel.empty()) return 0;
  int d = 0;
  for (const auto& part : rel) {
    (void)part;
    ++d;
  }
  return d;
}

auto to_unix_seconds(std::filesystem::file_time_type ft) -> double {
  auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
      ft - std::filesystem::file_time_type::clock::now() +
      std::chrono::system_clock::now());
  auto sec =
      std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch())
          .count();
  auto frac =
      std::chrono::duration_cast<std::chrono::microseconds>(
          sctp.time_since_epoch() - std::chrono::seconds(sec))
          .count();
  return static_cast<double>(sec) + static_cast<double>(frac) / 1e6;
}

auto get_octal_mode(const std::filesystem::path& p) -> int {
  std::error_code ec;
  auto perms = std::filesystem::status(p, ec).permissions();
  if (ec) return 0;
  int mode = 0;
  using pm = std::filesystem::perms;
  if ((perms & pm::owner_read) != pm::none) mode |= 0400;
  if ((perms & pm::owner_write) != pm::none) mode |= 0200;
  if ((perms & pm::owner_exec) != pm::none) mode |= 0100;
  if ((perms & pm::group_read) != pm::none) mode |= 0040;
  if ((perms & pm::group_write) != pm::none) mode |= 0020;
  if ((perms & pm::group_exec) != pm::none) mode |= 0010;
  if ((perms & pm::others_read) != pm::none) mode |= 0004;
  if ((perms & pm::others_write) != pm::none) mode |= 0002;
  if ((perms & pm::others_exec) != pm::none) mode |= 0001;
  return mode;
}

auto get_ls_mode(const std::filesystem::path& p) -> std::string {
  std::error_code ec;
  auto st = std::filesystem::status(p, ec);
  if (ec) return "?????????";
  auto perms = st.permissions();
  std::string s(10, '-');
  if (std::filesystem::is_directory(st))
    s[0] = 'd';
  else if (std::filesystem::is_symlink(st))
    s[0] = 'l';
  else
    s[0] = '-';
  using pm = std::filesystem::perms;
  s[1] = (perms & pm::owner_read) != pm::none ? 'r' : '-';
  s[2] = (perms & pm::owner_write) != pm::none ? 'w' : '-';
  s[3] = (perms & pm::owner_exec) != pm::none ? 'x' : '-';
  s[4] = (perms & pm::group_read) != pm::none ? 'r' : '-';
  s[5] = (perms & pm::group_write) != pm::none ? 'w' : '-';
  s[6] = (perms & pm::group_exec) != pm::none ? 'x' : '-';
  s[7] = (perms & pm::others_read) != pm::none ? 'r' : '-';
  s[8] = (perms & pm::others_write) != pm::none ? 'w' : '-';
  s[9] = (perms & pm::others_exec) != pm::none ? 'x' : '-';
  return s;
}

auto get_user_name() -> std::string {
  char name[256];
  DWORD size = sizeof(name);
  if (GetUserNameA(name, &size)) return name;
  return "";
}

auto get_group_name() -> std::string {
  return get_user_name();
}

auto parse_size(std::string_view spec)
    -> cp::Result<std::tuple<int64_t, char, char>> {
  if (spec.empty()) return cp::unexpected("invalid size");
  size_t i = 0;
  char sign = 0;
  if (spec[0] == '+' || spec[0] == '-') {
    sign = spec[0];
    ++i;
  }
  int64_t value = 0;
  size_t start = i;
  while (i < spec.size() && std::isdigit(static_cast<unsigned char>(spec[i]))) {
    value = value * 10 + (spec[i] - '0');
    ++i;
  }
  if (i == start) return cp::unexpected("invalid size");
  char unit = 'b';
  if (i < spec.size()) {
    unit = spec[i];
    if (unit != 'b' && unit != 'c' && unit != 'k' && unit != 'w')
      return cp::unexpected("invalid size unit");
    ++i;
  }
  if (i != spec.size()) return cp::unexpected("invalid size");
  return std::make_tuple(value, unit, sign);
}

auto size_matches(const std::filesystem::directory_entry& e,
                  std::string_view spec) -> bool {
  auto parsed = parse_size(spec);
  if (!parsed) return false;
  auto [value, unit, sign] = *parsed;

  std::error_code ec;
  int64_t file_size = static_cast<int64_t>(e.file_size(ec));
  if (ec) return false;

  int64_t target = value;
  switch (unit) {
    case 'b':
      target = value * 512;
      break;
    case 'c':
      target = value;
      break;
    case 'k':
      target = value * 1024;
      break;
    case 'w':
      target = value * 2;
      break;
  }

  if (sign == '+') return file_size > target;
  if (sign == '-') return file_size < target;
  return file_size == target;
}

auto parse_signed_number(std::string_view spec)
    -> cp::Result<std::pair<int64_t, char>> {
  if (spec.empty()) return cp::unexpected("invalid number");
  size_t i = 0;
  char sign = 0;
  if (spec[0] == '+' || spec[0] == '-') {
    sign = spec[0];
    ++i;
  }
  int64_t value = 0;
  size_t start = i;
  while (i < spec.size() && std::isdigit(static_cast<unsigned char>(spec[i]))) {
    value = value * 10 + (spec[i] - '0');
    ++i;
  }
  if (i == start || i != spec.size())
    return cp::unexpected("invalid number");
  return std::make_pair(value, sign);
}

auto is_empty_dir(const std::filesystem::path& p) -> bool {
  std::error_code ec;
  auto it = std::filesystem::directory_iterator(p, ec);
  if (ec) return false;
  return it == std::filesystem::directory_iterator();
}

auto type_matches(const std::filesystem::directory_entry& e,
                  std::string_view type, bool follow_symlinks) -> bool {
  if (type.empty()) return true;

  bool is_sym = e.is_symlink();

  if (type == "f") {
    if (follow_symlinks && is_sym) {
      std::error_code ec;
      return std::filesystem::is_regular_file(e.path(), ec);
    }
    return e.is_regular_file() && !is_sym;
  }
  if (type == "d") {
    if (follow_symlinks && is_sym) {
      std::error_code ec;
      return std::filesystem::is_directory(e.path(), ec);
    }
    return e.is_directory() && !is_sym;
  }
  if (type == "l") {
    if (follow_symlinks) return false;
    return is_sym;
  }
  return false;
}

auto quote_arg_w(const std::wstring& arg) -> std::wstring {
  if (arg.empty()) return L"\"\"";
  bool need_quote = arg.find_first_of(L" \t\"") != std::wstring::npos;
  if (!need_quote) return arg;
  std::wstring out = L"\"";
  size_t backslashes = 0;
  for (wchar_t c : arg) {
    if (c == L'\\') {
      ++backslashes;
    } else if (c == L'"') {
      out.append(backslashes * 2 + 1, L'\\');
      out.push_back(L'"');
      backslashes = 0;
    } else {
      out.append(backslashes, L'\\');
      backslashes = 0;
      out.push_back(c);
    }
  }
  out.append(backslashes * 2, L'\\');
  out.push_back(L'"');
  return out;
}

auto run_command(const std::string& command,
                 const std::vector<std::string>& args) -> bool {
  std::wstring cmd_line = quote_arg_w(utf8_to_wstring(command));
  for (const auto& a : args) {
    cmd_line += L" ";
    cmd_line += quote_arg_w(utf8_to_wstring(a));
  }

  STARTUPINFOW si = {sizeof(si)};
  PROCESS_INFORMATION pi{};
  BOOL success =
      CreateProcessW(nullptr, cmd_line.data(), nullptr, nullptr, TRUE, 0,
                     nullptr, nullptr, &si, &pi);
  if (!success) return false;

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 0;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return exit_code == 0;
}

auto confirm_and_run(const std::string& command,
                     const std::vector<std::string>& args) -> bool {
  std::string display = command;
  for (const auto& a : args) {
    display += " " + a;
  }
  safeErrorPrint(display);
  safeErrorPrint(" ?...");

  std::string response;
  std::getline(std::cin, response);
  for (unsigned char ch : response) {
    if (std::isspace(ch)) continue;
    if (ch == 'y' || ch == 'Y') {
      return run_command(command, args);
    }
    return false;
  }
  return false;
}

auto build_exec_args(const std::vector<std::string>& template_args,
                     const std::string& path) -> std::vector<std::string> {
  std::vector<std::string> result;
  for (const auto& arg : template_args) {
    std::string replaced = arg;
    size_t pos = 0;
    while ((pos = replaced.find("{}", pos)) != std::string::npos) {
      replaced.replace(pos, 2, path);
      pos += path.size();
    }
    result.push_back(replaced);
  }
  return result;
}

auto flush_exec_batches(Config& cfg) -> void {
  for (auto& batch : cfg.exec_batches) {
    if (batch.paths.empty()) continue;
    std::vector<std::string> args;
    for (const auto& arg : batch.template_args) {
      if (arg == "{}") {
        for (const auto& p : batch.paths) {
          args.push_back(p);
        }
      } else if (arg.find("{}") != std::string::npos) {
        // For + mode with embedded {}, replace with all paths space-separated
        std::string replacement;
        for (size_t i = 0; i < batch.paths.size(); ++i) {
          if (i > 0) replacement += " ";
          replacement += batch.paths[i];
        }
        std::string replaced = arg;
        size_t pos = 0;
        while ((pos = replaced.find("{}", pos)) != std::string::npos) {
          replaced.replace(pos, 2, replacement);
          pos += replacement.size();
        }
        args.push_back(replaced);
      } else {
        args.push_back(arg);
      }
    }
    run_command(batch.command, args);
  }
  cfg.exec_batches.clear();
}

// ============================================================================
// -printf
// ============================================================================

auto do_printf(const std::string& format, const std::filesystem::path& p,
               const std::filesystem::directory_entry& e, int depth,
               const std::filesystem::path& root) -> void {
  std::string output;
  for (size_t i = 0; i < format.size(); ++i) {
    if (format[i] == '%' && i + 1 < format.size()) {
      char spec = format[++i];
      switch (spec) {
        case 'p':
          output += path_display(p);
          break;
        case 'f':
          output += p.filename().string();
          break;
        case 'h': {
          auto parent = p.parent_path();
          if (parent.empty()) parent = ".";
          output += path_display(parent);
          break;
        }
        case 'y': {
          if (e.is_symlink())
            output += "l";
          else if (e.is_directory())
            output += "d";
          else if (e.is_regular_file())
            output += "f";
          else
            output += "?";
          break;
        }
        case 's': {
          std::error_code ec;
          auto sz = e.file_size(ec);
          output += std::to_string(sz);
          break;
        }
        case 'm': {
          output += std::to_string(get_octal_mode(p));
          break;
        }
        case 'M': {
          output += get_ls_mode(p);
          break;
        }
        case 'T': {
          if (i + 1 < format.size() && format[i + 1] == '@') {
            ++i;
            std::error_code ec;
            auto ft = e.last_write_time(ec);
            if (!ec) {
              auto t = to_unix_seconds(ft);
              std::ostringstream oss;
              oss << std::fixed << std::setprecision(9) << t;
              output += oss.str();
            }
          } else {
            output += "%T";
          }
          break;
        }
        case 'u':
          output += get_user_name();
          break;
        case 'g':
          output += get_group_name();
          break;
        case 'l': {
          std::error_code ec;
          auto target = std::filesystem::read_symlink(p, ec);
          if (!ec) output += target.string();
          break;
        }
        case 'd':
          output += std::to_string(depth);
          break;
        case '%':
          output += "%";
          break;
        default:
          output += '%';
          output += spec;
          break;
      }
    } else if (format[i] == '\\' && i + 1 < format.size()) {
      char esc = format[++i];
      switch (esc) {
        case 't':
          output += '\t';
          break;
        case 'n':
          output += '\n';
          break;
        case 'r':
          output += '\r';
          break;
        case '0':
          output += '\0';
          break;
        case '\\':
          output += '\\';
          break;
        default:
          output += esc;
          break;
      }
    } else {
      output += format[i];
    }
  }
  safePrint(output);
}

// ============================================================================
// AST evaluation
// ============================================================================

struct EvalResult {
  bool value = false;
  bool output_done = false;
};

auto evaluate_node(Config& cfg, const Node& node,
                   const std::filesystem::path& p,
                   const std::filesystem::directory_entry& e, int depth,
                   const std::filesystem::path& root) -> EvalResult;

auto eval_primary(Config& cfg, const Node& node,
                  const std::filesystem::path& p,
                  const std::filesystem::directory_entry& e, int depth,
                  const std::filesystem::path& root) -> EvalResult {
  switch (node.type) {
    case NodeType::Name: {
      auto name = p.filename().string();
      if (name.empty()) name = p.generic_string();
      bool m = wildcard_match(node.value, name, true);
      return {m, false};
    }
    case NodeType::Iname: {
      auto name = p.filename().string();
      if (name.empty()) name = p.generic_string();
      bool m = wildcard_match(node.value, name, false);
      return {m, false};
    }
    case NodeType::Path: {
      bool m = wildcard_match(node.value, path_display(p), true);
      return {m, false};
    }
    case NodeType::Ipath: {
      bool m = wildcard_match(node.value, path_display(p), false);
      return {m, false};
    }
    case NodeType::Type: {
      return {type_matches(e, node.value, cfg.follow_symlinks), false};
    }
    case NodeType::Size: {
      return {size_matches(e, node.value), false};
    }
    case NodeType::Empty: {
      std::error_code ec;
      bool empty = false;
      bool is_reg = e.is_regular_file(ec);
      if (is_reg && !ec) {
        auto sz = e.file_size(ec);
        empty = (sz == 0);
      } else {
        std::error_code ec2;
        bool is_dir = e.is_directory(ec2);
        if (is_dir && !ec2) {
          empty = is_empty_dir(p);
        }
      }
      return {empty, false};
    }
    case NodeType::Mmin: {
      auto parsed = parse_signed_number(node.value);
      if (!parsed) return {false, false};
      auto [n, sign] = *parsed;
      std::error_code ec;
      auto mtime = e.last_write_time(ec);
      if (ec) return {false, false};
      auto diff = cfg.now - mtime;
      auto minutes =
          std::chrono::duration_cast<std::chrono::minutes>(diff).count();
      if (sign == '+') return {minutes > n, false};
      if (sign == '-') return {minutes < n, false};
      return {minutes == n, false};
    }
    case NodeType::Mtime: {
      auto parsed = parse_signed_number(node.value);
      if (!parsed) return {false, false};
      auto [n, sign] = *parsed;
      std::error_code ec;
      auto mtime = e.last_write_time(ec);
      if (ec) return {false, false};
      auto diff = cfg.now - mtime;
      auto hours =
          std::chrono::duration_cast<std::chrono::hours>(diff).count();
      double days = static_cast<double>(hours) / 24.0;
      if (sign == '+') return {days > n, false};
      if (sign == '-') return {days < n, false};
      return {std::fabs(days - n) < 0.5, false};
    }
    case NodeType::True:
      return {true, false};
    case NodeType::False:
      return {false, false};
    case NodeType::Regex: {
      try {
        std::regex re(node.value);
        auto disp = path_display(p);
        bool m = std::regex_match(disp, re);
        return {m, false};
      } catch (...) {
        return {false, false};
      }
    }
    case NodeType::Iregex: {
      try {
        std::regex re(node.value, std::regex::icase);
        auto disp = path_display(p);
        bool m = std::regex_match(disp, re);
        return {m, false};
      } catch (...) {
        return {false, false};
      }
    }
    case NodeType::Newer: {
      std::error_code ec;
      auto ref_time = std::filesystem::last_write_time(node.value, ec);
      if (ec) return {false, false};
      auto mtime = e.last_write_time(ec);
      if (ec) return {false, false};
      return {mtime > ref_time, false};
    }
    case NodeType::Print: {
      safePrint(path_display(p));
      safePrint("\n");
      return {true, true};
    }
    case NodeType::Print0: {
      safePrint(path_display(p));
      safePrint(std::string("\0", 1));
      return {true, true};
    }
    case NodeType::Printf: {
      do_printf(node.value, p, e, depth, root);
      return {true, true};
    }
    case NodeType::Exec: {
      if (node.exec_plus) {
        // Batch mode
        if (cfg.exec_batches.empty() ||
            cfg.exec_batches.back().command != node.exec_args[0] ||
            cfg.exec_batches.back().template_args.size() !=
                node.exec_args.size() - 1) {
          cfg.exec_batches.push_back(
              {node.exec_args[0],
               std::vector<std::string>(node.exec_args.begin() + 1,
                                        node.exec_args.end()),
               {}});
        }
        cfg.exec_batches.back().paths.push_back(path_display(p));
        return {true, true};
      } else {
        auto args = build_exec_args(node.exec_args, path_display(p));
        std::string cmd = args.empty() ? "" : args[0];
        std::vector<std::string> rest;
        if (args.size() > 1) rest.assign(args.begin() + 1, args.end());
        bool ok = run_command(cmd, rest);
        return {ok, true};
      }
    }
    case NodeType::Ok: {
      auto args = build_exec_args(node.exec_args, path_display(p));
      std::string cmd = args.empty() ? "" : args[0];
      std::vector<std::string> rest;
      if (args.size() > 1) rest.assign(args.begin() + 1, args.end());
      bool ok = confirm_and_run(cmd, rest);
      return {ok, true};
    }
    case NodeType::Delete: {
      std::error_code ec;
      bool ok = std::filesystem::remove(p, ec);
      if (!ok || ec) {
        cfg.had_error = true;
        return {false, true};
      }
      return {true, true};
    }
    case NodeType::Prune: {
      cfg.prune_next = true;
      return {true, false};
    }
    case NodeType::Quit: {
      cfg.global_quit = true;
      return {true, false};
    }
    default:
      return {false, false};
  }
}

auto evaluate_node(Config& cfg, const Node& node,
                   const std::filesystem::path& p,
                   const std::filesystem::directory_entry& e, int depth,
                   const std::filesystem::path& root) -> EvalResult {
  switch (node.type) {
    case NodeType::Not: {
      auto r = evaluate_node(cfg, *node.left, p, e, depth, root);
      return {!r.value, r.output_done};
    }
    case NodeType::And: {
      auto l = evaluate_node(cfg, *node.left, p, e, depth, root);
      if (!l.value) return {false, l.output_done};
      auto r = evaluate_node(cfg, *node.right, p, e, depth, root);
      return {r.value, l.output_done || r.output_done};
    }
    case NodeType::Or: {
      auto l = evaluate_node(cfg, *node.left, p, e, depth, root);
      if (l.value) return {true, l.output_done};
      auto r = evaluate_node(cfg, *node.right, p, e, depth, root);
      return {r.value, r.output_done};
    }
    default:
      return eval_primary(cfg, node, p, e, depth, root);
  }
}

// ============================================================================
// AST inspection helpers
// ============================================================================

auto contains_output_action(const Node& node) -> bool {
  switch (node.type) {
    case NodeType::Print:
    case NodeType::Print0:
    case NodeType::Printf:
    case NodeType::Exec:
    case NodeType::Ok:
    case NodeType::Delete:
      return true;
    case NodeType::Not:
      return contains_output_action(*node.left);
    case NodeType::And:
    case NodeType::Or:
      return contains_output_action(*node.left) ||
             contains_output_action(*node.right);
    default:
      return false;
  }
}

auto contains_delete(const Node& node) -> bool {
  switch (node.type) {
    case NodeType::Delete:
      return true;
    case NodeType::Not:
      return contains_delete(*node.left);
    case NodeType::And:
    case NodeType::Or:
      return contains_delete(*node.left) || contains_delete(*node.right);
    default:
      return false;
  }
}

// ============================================================================
// Traversal
// ============================================================================

auto visit(Config& cfg, const std::filesystem::path& p, int depth,
           const std::filesystem::path& root) -> void {
  if (depth > cfg.maxdepth) return;
  if (cfg.global_quit) return;

  std::error_code ec;
  std::filesystem::directory_entry entry(p, ec);
  if (ec) {
    cfg.had_error = true;
    return;
  }

  bool is_dir = entry.is_directory(ec);
  if (ec) is_dir = false;

  bool postorder = cfg.depth || cfg.delete_present;

  if (postorder && is_dir) {
    auto options = std::filesystem::directory_options::skip_permission_denied;
    if (cfg.follow_symlinks)
      options |= std::filesystem::directory_options::follow_directory_symlink;

    auto it = std::filesystem::directory_iterator(p, options, ec);
    if (!ec) {
      for (const auto& child : it) {
        if (cfg.global_quit) return;
        visit(cfg, child.path(), depth + 1, root);
      }
    }
  }

  if (depth >= cfg.mindepth && depth <= cfg.maxdepth) {
    bool result = true;
    bool output_done = false;
    if (cfg.ast) {
      auto r = evaluate_node(cfg, *cfg.ast, p, entry, depth, root);
      result = r.value;
      output_done = r.output_done;
    }
    if (result && !output_done && !cfg.default_print_suppressed) {
      safePrint(path_display(p));
      safePrint("\n");
    }
  }

  if (cfg.global_quit) return;

  if (!postorder) {
    if (cfg.prune_next) {
      cfg.prune_next = false;
      return;
    }

    if (is_dir) {
      auto options = std::filesystem::directory_options::skip_permission_denied;
      if (cfg.follow_symlinks)
        options |=
            std::filesystem::directory_options::follow_directory_symlink;

      auto it = std::filesystem::directory_iterator(p, options, ec);
      if (!ec) {
        for (const auto& child : it) {
          if (cfg.global_quit) return;
          visit(cfg, child.path(), depth + 1, root);
        }
      }
    }
  }
}

auto scan_root(Config& cfg, const std::filesystem::path& root) -> void {
  std::error_code ec;
  bool exists = std::filesystem::exists(root, ec);
  if (ec || !exists) {
    safeErrorPrint("find: '");
    safeErrorPrint(root.string());
    safeErrorPrint("': No such file or directory\n");
    cfg.had_error = true;
    return;
  }
  visit(cfg, root, 0, root);
}

// ============================================================================
// Build config
// ============================================================================

auto build_config(const CommandContext<FIND_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;

  // Extract global options from occurrences
  for (const auto& occ : ctx.options.occurrences()) {
    if (!ctx.metas || occ.index >= FIND_OPTIONS.size()) continue;
    const auto& meta = (*ctx.metas)[occ.index];
    if (meta.short_name == "-maxdepth" || meta.long_name == "-maxdepth") {
      if (auto v = std::get_if<int>(&occ.value)) cfg.maxdepth = *v;
    } else if (meta.short_name == "-mindepth" ||
               meta.long_name == "-mindepth") {
      if (auto v = std::get_if<int>(&occ.value)) cfg.mindepth = *v;
    } else if (meta.short_name == "-depth" || meta.long_name == "-depth") {
      cfg.depth = true;
    } else if (meta.short_name == "-L" || meta.long_name == "-L") {
      cfg.follow_symlinks = true;
    } else if (meta.short_name == "-H" || meta.long_name == "-H") {
      return cp::unexpected("-H is not supported");
    }
  }

  cfg.now = std::filesystem::file_time_type::clock::now();

  // Tokenize
  auto token_result = tokenize_expression(ctx.raw_args, *ctx.metas);
  if (!token_result) return cp::unexpected(token_result.error());
  auto [paths, tokens] = *token_result;

  // Parse paths
  for (const auto& p : paths) cfg.roots.push_back(p);
  if (cfg.roots.empty()) cfg.roots.push_back(".");

  // Parse expression
  if (!tokens.empty()) {
    Parser parser{tokens};
    auto ast = parser.parse_expression();
    if (!ast) return cp::unexpected(ast.error());
    if (parser.pos != tokens.size()) {
      return cp::unexpected("unexpected token after expression");
    }
    cfg.ast = std::move(*ast);
    cfg.output_action_present = contains_output_action(*cfg.ast);
    cfg.delete_present = contains_delete(*cfg.ast);
    cfg.default_print_suppressed = cfg.output_action_present;
  }

  return cfg;
}

auto process(Config& cfg) -> int {
  for (const auto& r : cfg.roots) {
    if (cfg.global_quit) break;
    scan_root(cfg, r);
  }
  flush_exec_batches(cfg);
  if (cfg.had_error) return 1;
  return 0;
}

}  // namespace find_pipeline

REGISTER_COMMAND(find, "find", "find [path...] [expression]",
                 "Search for files in a directory hierarchy.\n"
                 "If no path is given, '.' is used.",
                 "  find . -name '*.cpp'\n"
                 "  find src -type f -maxdepth 2\n"
                 "  find . -iname 'readme*'",
                 "grep(1), ls(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd",
                 FIND_OPTIONS) {
  using namespace find_pipeline;

  auto cfg = build_config(ctx);
  if (!cfg) {
    cp::report_error(cfg, L"find");
    return 1;
  }

  return process(*cfg);
}
