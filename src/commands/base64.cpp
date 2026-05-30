/*
 *  Copyright © 2026 WinuxCmd
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
 *  - File: base64.cpp
 *  - CopyrightYear: 2026
 */
#include "core/command_macros.h"
#include "../core/core.h"
#include "../utils/utils.h"
#include "../container/container.h"

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

auto constexpr BASE64_OPTIONS = std::array{
    OPTION("-d", "--decode", "decode data", BOOL_TYPE),
    OPTION("-i", "--ignore-garbage",
           "when decoding, ignore non-alphabet characters", BOOL_TYPE),
    OPTION("-w", "--wrap",
           "wrap encoded lines after COLS character (default 76). Use 0 to "
           "disable line wrapping",
           INT_TYPE)};

namespace base64_pipeline {
namespace cp = core::pipeline;

auto read_input(std::string_view filename) -> cp::Result<std::string> {
  std::string content;
  if (filename == "-" || filename.empty()) {
    content.assign(std::istreambuf_iterator<char>(std::cin),
                   std::istreambuf_iterator<char>());
    if (std::cin.fail() && !std::cin.eof()) {
      return cp::unexpected("error reading from standard input");
    }
  } else {
    std::ifstream file(std::string(filename), std::ios::binary);
    if (!file) {
      return cp::unexpected(std::string("cannot open '") + std::string(filename) +
                            "' for reading");
    }
    content.assign(std::istreambuf_iterator<char>(file),
                   std::istreambuf_iterator<char>());
    if (file.fail() && !file.eof()) {
      return cp::unexpected("error reading from file");
    }
  }
  return content;
}

struct Config {
  bool decode = false;
  bool ignore_garbage = false;
  int wrap = 76;
  std::string file;
};

auto build_config(const CommandContext<BASE64_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;
  cfg.decode = ctx.get<bool>("--decode", false) || ctx.get<bool>("-d", false);
  cfg.ignore_garbage =
      ctx.get<bool>("--ignore-garbage", false) || ctx.get<bool>("-i", false);
  cfg.wrap = ctx.get<int>("--wrap", 76);

  if (!ctx.positionals.empty()) {
    if (ctx.positionals.size() > 1) {
      return cp::unexpected("extra operand");
    }
    cfg.file = std::string(ctx.positionals[0]);
  }

  return cfg;
}

auto run(const Config& cfg) -> int {
  auto content_result = read_input(cfg.file);
  if (!content_result) {
    cp::report_error(content_result, L"base64");
    return 1;
  }

  std::string content = *content_result;

  if (cfg.decode) {
    auto decoded = encoding::base64_decode(content, cfg.ignore_garbage);
    if (decoded.empty() && !content.empty()) {
      safeErrorPrintLn("base64: invalid input");
      return 1;
    }
    std::string output(reinterpret_cast<const char*>(decoded.data()),
                       decoded.size());
    safePrint(output);
  } else {
    auto data = std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(content.data()), content.size());
    std::string output = encoding::base64_encode(data, cfg.wrap);
    if (!output.empty()) {
      output += '\n';
    }
    safePrint(output);
  }

  return 0;
}

}  // namespace base64_pipeline

REGISTER_COMMAND(
    base64, "base64", "base64 [OPTION]... [FILE]",
    "Encode or decode FILE, or standard input, to standard output.\n"
    "With no FILE, or when FILE is -, read standard input.\n"
    "\n"
    "The data are encoded as described for the base64 alphabet in RFC\n"
    "4648. When decoding, the input may contain newlines in addition\n"
    "to the bytes of the formal base64 alphabet. Use --ignore-garbage\n"
    "to attempt to recover from any other non-alphabet bytes in the\n"
    "encoded stream.",
    "  base64 <<< 'Hello, World'\n"
    "  echo 'SGVsbG8sIFdvcmxk' | base64 -d\n"
    "  base64 -w 0 file.txt  # No line wrapping",
    "base32(1), basenc(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd",
    BASE64_OPTIONS) {
  using namespace base64_pipeline;

  auto cfg_result = build_config(ctx);
  if (!cfg_result) {
    cp::report_error(cfg_result, L"base64");
    return 1;
  }

  return run(*cfg_result);
}
