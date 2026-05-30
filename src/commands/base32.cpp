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
 *  - File: base32.cpp
 *  - CopyrightYear: 2026
 */
#include "core/command_macros.h"
#include "../core/core.h"
#include "../utils/utils.h"
#include "../container/container.h"

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

auto constexpr BASE32_OPTIONS = std::array{
    OPTION("-d", "--decode", "decode data"),
    OPTION("-i", "--ignore-garbage",
           "when decoding, ignore non-alphabet characters"),
    OPTION("-w", "--wrap",
           "wrap encoded lines at COLS (default 76). Use 0 to disable line wrapping",
           INT_TYPE)};

namespace base32_pipeline {
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

auto build_config(const CommandContext<BASE32_OPTIONS.size()>& ctx)
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
    cp::report_error(content_result, L"base32");
    return 1;
  }

  std::string content = *content_result;

  if (cfg.decode) {
    auto decoded = encoding::base32_decode(content, cfg.ignore_garbage);
    if (decoded.empty() && !content.empty()) {
      safeErrorPrintLn("base32: invalid input");
      return 1;
    }
    std::string output(decoded.begin(), decoded.end());
    safePrint(output);
  } else {
    auto data = std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(content.data()), content.size());
    std::string encoded = encoding::base32_encode(data, cfg.wrap);
    if (!encoded.empty()) {
      safePrintLn(encoded);
    }
  }

  return 0;
}

}  // namespace base32_pipeline

REGISTER_COMMAND(
    base32, "base32", "base32 [OPTION]... [FILE]",
    "Base32 encode or decode FILE, or standard input, to standard output.\n"
    "Encode or decode data using the Base32 encoding scheme (RFC 4648).\n"
    "With no FILE, or when FILE is -, read standard input.",
    "  base32 file.txt\n"
    "  echo 'Hello' | base32\n"
    "  base32 -d encoded.txt\n"
    "  base32 -w 64 file.bin",
    "base64(1), basenc(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd",
    BASE32_OPTIONS) {
  using namespace base32_pipeline;

  auto cfg_result = build_config(ctx);
  if (!cfg_result) {
    cp::report_error(cfg_result, L"base32");
    return 1;
  }

  return run(*cfg_result);
}
