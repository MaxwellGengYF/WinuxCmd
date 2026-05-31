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
 *  - File: sha224sum.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implementation for sha224sum.
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright © 2026 WinuxCmd
// *** SIMPLIFIED IMPLEMENTATION - Some features may not be fully supported ***
// include other header after pch.h
#include <wincrypt.h>

#include "core/command_macros.h"

#pragma comment(lib, "advapi32.lib")

#include "../core/core.h"
#include "../utils/utils.h"
#include "../container/container.h"

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

auto constexpr SHA224SUM_OPTIONS = std::array{
    OPTION("-b", "--binary", "read in binary mode (default)", BOOL_TYPE),
    OPTION("-c", "--check", "read SHA224 sums from the FILEs and check them",
           STRING_TYPE),
    OPTION("-t", "--text", "read in text mode", BOOL_TYPE),
    OPTION("-q", "--quiet",
           "don't print OK for each successfully verified file", BOOL_TYPE),
    OPTION("-s", "--status", "don't output anything, status code shows success",
           BOOL_TYPE),
    OPTION("-w", "--warn", "warn about improperly formatted checksum lines",
           BOOL_TYPE),
    OPTION("", "--tag",
           "create a BSD-style checksum", BOOL_TYPE),
    OPTION("", "--zero",
           "end each output line with NUL, not newline", BOOL_TYPE),
    OPTION("", "--strict",
           "with --check, exit non-zero for any invalid input", BOOL_TYPE)};

namespace sha224sum_pipeline {
namespace cp = core::pipeline;

struct Config {
  bool binary_mode = true;
  bool check_mode = false;
  bool text_mode = false;
  bool quiet = false;
  bool status = false;
  bool warn = false;
  bool tag = false;
  bool zero = false;
  bool strict = false;
  std::string check_file;
  SmallVector<std::string, 64> files;
};

auto build_config(const CommandContext<SHA224SUM_OPTIONS.size()>& ctx)
    -> cp::Result<Config> {
  Config cfg;
  cfg.binary_mode =
      ctx.get<bool>("--binary", false) || ctx.get<bool>("-b", false);
  auto check_opt = ctx.get<std::string>("--check", "");
  cfg.check_mode =
      !check_opt.empty() || !ctx.get<std::string>("-c", "").empty();
  cfg.text_mode = ctx.get<bool>("--text", false) || ctx.get<bool>("-t", false);
  cfg.quiet = ctx.get<bool>("--quiet", false) || ctx.get<bool>("-q", false);
  cfg.status = ctx.get<bool>("--status", false) || ctx.get<bool>("-s", false);
  cfg.warn = ctx.get<bool>("--warn", false) || ctx.get<bool>("-w", false);
  cfg.tag = ctx.get<bool>("--tag", false);
  cfg.zero = ctx.get<bool>("--zero", false);
  cfg.strict = ctx.get<bool>("--strict", false);

  if (cfg.check_mode) {
    cfg.check_file = ctx.get<std::string>("--check", "");
    if (cfg.check_file.empty()) {
      cfg.check_file = ctx.get<std::string>("-c", "");
    }
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

  if (cfg.files.empty() && !cfg.check_mode) {
    cfg.files.push_back("-");
  }

  return cfg;
}

// Calculate SHA224 hash using Windows CryptoAPI
// Note: Windows CryptoAPI doesn't directly support SHA224, need to use SHA256
// and truncate
auto calculate_sha224(const std::string& filename, bool text_mode = false)
    -> cp::Result<std::string> {
  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;

  // Open cryptographic provider
  // Note: SHA256 requires PROV_RSA_AES or a SHA256-capable provider
  if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES,
                           CRYPT_VERIFYCONTEXT)) {
    return core::pipeline::unexpected("failed to acquire cryptographic context");
  }

  // Create hash object using SHA256 (we'll truncate to SHA224)
  if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
    CryptReleaseContext(hProv, 0);
    return core::pipeline::unexpected("failed to create hash object");
  }

  bool success = false;
  if (filename == "-" || filename.empty()) {
    // Read from stdin
    std::array<char, 8192> buffer;
    size_t bytes_read;

    while ((bytes_read = fread(buffer.data(), 1, buffer.size(), stdin)) > 0) {
      if (!CryptHashData(hHash, reinterpret_cast<BYTE*>(buffer.data()),
                         static_cast<DWORD>(bytes_read), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return core::pipeline::unexpected("failed to hash data");
      }
    }
    success = true;
  } else {
    // Read from file (binary mode by default, text mode if --text)
    std::ifstream file(filename, text_mode ? std::ios::in : std::ios::binary);
    if (!file) {
      CryptDestroyHash(hHash);
      CryptReleaseContext(hProv, 0);
      return core::pipeline::unexpected(std::string("cannot open '") + filename +
                             "' for reading");
    }

    std::array<char, 8192> buffer;
    while (file) {
      file.read(buffer.data(), buffer.size());
      std::streamsize bytes_read = file.gcount();
      if (bytes_read > 0) {
        if (!CryptHashData(hHash, reinterpret_cast<BYTE*>(buffer.data()),
                           static_cast<DWORD>(bytes_read), 0)) {
          CryptDestroyHash(hHash);
          CryptReleaseContext(hProv, 0);
          return core::pipeline::unexpected("failed to hash data");
        }
      }
    }
    success = !file.fail();
  }

  // Get hash value
  DWORD hash_len = 32;  // SHA256 produces 32 bytes
  std::array<BYTE, 32> hash_value{};

  if (!CryptGetHashParam(hHash, HP_HASHVAL, hash_value.data(), &hash_len, 0)) {
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return core::pipeline::unexpected("failed to get hash value");
  }

  CryptDestroyHash(hHash);
  CryptReleaseContext(hProv, 0);

  // SHA224 is SHA256 truncated to 28 bytes (224 bits)
  // Convert first 28 bytes to hex string
  std::string result;
  result.reserve(56);
  for (DWORD i = 0; i < 28; ++i) {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02x", hash_value[i]);
    result += buf;
  }

  return result;
}

auto run(const Config& cfg) -> int {
  if (cfg.check_mode) {
    std::ifstream check_file(cfg.check_file);
    if (!check_file) {
      cp::report_custom_error(L"sha224sum", L"cannot open '" + std::wstring(cfg.check_file.begin(), cfg.check_file.end()) + L"' for reading");
      return 1;
    }
    bool all_ok = true; int files_checked = 0; int files_failed = 0;
    std::string line;
    while (std::getline(check_file, line)) {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      if (line.empty()) continue;
      size_t hash_end = line.find(' ');
      if (hash_end == std::string::npos || hash_end == 0) { if (cfg.warn) safeErrorPrint("sha224sum: improperly formatted checksum line: "+line+"\n"); if (cfg.strict){all_ok=false;++files_failed;} continue; }
      std::string expected_hash = line.substr(0, hash_end);
      size_t file_start = hash_end + 1;
      while (file_start < line.size() && line[file_start] == ' ') ++file_start;
      if (file_start < line.size() && line[file_start] == '*') { ++file_start; while (file_start < line.size() && line[file_start] == ' ') ++file_start; }
      std::string filename = line.substr(file_start);
      if (filename.empty()) { if (cfg.warn) safeErrorPrint("sha224sum: improperly formatted checksum line: "+line+"\n"); if (cfg.strict){all_ok=false;++files_failed;} continue; }
      ++files_checked;
      auto hash_result = calculate_sha224(filename, cfg.text_mode);
      if (!hash_result) { if (!cfg.status) safeErrorPrint("sha224sum: "+filename+": FAILED open or read\n"); all_ok=false; ++files_failed; continue; }
      if (*hash_result == expected_hash) { if (!cfg.status && !cfg.quiet) safePrint(filename+": OK\n"); }
      else { if (!cfg.status) safePrint(filename+": FAILED\n"); all_ok=false; ++files_failed; }
    }
    if (files_checked==0) { safeErrorPrint("sha224sum: no properly formatted checksum lines found\n"); return 1; }
    if (!cfg.status && files_failed > 0) { safeErrorPrint("sha224sum: WARNING: "+std::to_string(files_failed)+" computed checksum"+(files_failed>1?"s":"")+" did NOT match\n"); }
    return all_ok ? 0 : 1;
  }

  bool all_ok = true;

  for (const auto& file : cfg.files) {
    auto hash_result = calculate_sha224(file, cfg.text_mode);
    if (!hash_result) {
      cp::report_error(hash_result, L"sha224sum");
      all_ok = false;
      continue;
    }

    // Output format: HASH  FILENAME (or BSD-style if --tag)
    const char* term = cfg.zero ? "\0" : "\n";
    if (cfg.tag) {
      safePrint("SHA224 (" + file + ") = " + *hash_result + term);
    } else if (cfg.quiet && !cfg.check_mode) {
      safePrint(*hash_result + term);
    } else {
      safePrint(*hash_result + "  " + file + term);
    }
  }

  return all_ok ? 0 : 1;
}

}  // namespace sha224sum_pipeline

REGISTER_COMMAND(sha224sum, "sha224sum", "sha224sum [OPTION]... [FILE]...",
                 "Compute and check SHA224 message digest.\n"
                 "\n"
                 "With no FILE, or when FILE is -, read standard input.\n"
                 "\n"
                 "SHA224 produces a 224-bit (28-byte) hash value, typically "
                 "rendered as a 56-digit hexadecimal number.\n"
                 "This implementation uses SHA256 and truncates to 224 bits as "
                 "Windows CryptoAPI doesn't directly support SHA224.",
                 "  sha224sum file.txt\n"
                 "  echo \"test\" | sha224sum\n"
                 "  sha224sum *.txt > checksums.sha224",
                 "md5sum(1), sha1sum(1), sha256sum(1), sha512sum(1)",
                 "WinuxCmd", "Copyright © 2026 WinuxCmd", SHA224SUM_OPTIONS) {
  using namespace sha224sum_pipeline;

  auto cfg_result = build_config(ctx);
  if (!cfg_result) {
    cp::report_error(cfg_result, L"sha224sum");
    return 1;
  }

  return run(*cfg_result);
}
