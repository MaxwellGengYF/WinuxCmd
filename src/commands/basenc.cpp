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
 *  - File: basenc.cpp
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implementation for basenc command (multiple encodings).
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright © 2026 WinuxCmd

#include "core/command_macros.h"
#include "../core/core.h"
#include "../utils/utils.h"
#include "../container/container.h"

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

// ======================================================
// Options (constexpr)
// ======================================================

auto constexpr BASENC_OPTIONS =
    std::array{OPTION("-d", "--decode", "decode data"),
               OPTION("-b", "", "baseN", STRING_TYPE),
               OPTION("-w", "--wrap", "wrap at COLS", STRING_TYPE),
               OPTION("", "--base64", "use base64 encoding"),
               OPTION("", "--base64url", "use base64url encoding"),
               OPTION("", "--base32", "use base32 encoding"),
               OPTION("", "--base32hex", "use base32hex encoding"),
               OPTION("", "--base16", "use base16 (hex) encoding"),
               OPTION("", "--base2msbf", "use base2 MSB-first encoding"),
               OPTION("", "--base2lsbf", "use base2 LSB-first encoding"),
               OPTION("", "--base2", "use base2 (binary) encoding"),
               OPTION("", "--hex", "use base16 (hex) encoding"),
               OPTION("", "--bin", "use base2 (binary) encoding"),
               OPTION("", "--base58", "base58 encoding (not implemented)"),
               OPTION("", "--z85", "z85 encoding (not implemented)"),
               OPTION("", "--ignore-garbage",
                      "when decoding, ignore non-alphabet characters")};

// ======================================================
// Helper functions
// ======================================================

namespace {
enum class Encoding {
  BASE64,
  BASE64URL,
  BASE32,
  BASE32HEX,
  BASE16,
  BASE2MSBF,
  BASE2LSBF,
  NONE
};

// Base64 encode (standard or URL-safe)
std::string base64_encode(const std::string& data, int wrap, bool url_safe) {
  const char* alphabet = url_safe
      ? "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
      : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string result;

  for (size_t i = 0; i < data.size(); i += 3) {
    unsigned char b0 = data[i];
    unsigned char b1 = (i + 1 < data.size()) ? data[i + 1] : 0;
    unsigned char b2 = (i + 2 < data.size()) ? data[i + 2] : 0;

    result += alphabet[b0 >> 2];
    result += alphabet[((b0 & 0x03) << 4) | (b1 >> 4)];
    result += alphabet[((b1 & 0x0F) << 2) | (b2 >> 6)];
    result += alphabet[b2 & 0x3F];
  }

  size_t padding = (3 - data.size() % 3) % 3;
  result.resize(result.size() - padding);
  result.append(padding, '=');

  if (wrap > 0) {
    std::string wrapped;
    for (size_t i = 0; i < result.size(); i += wrap) {
      if (!wrapped.empty()) wrapped += '\n';
      wrapped += result.substr(i, wrap);
    }
    return wrapped;
  }

  return result;
}

// Base64 decode (standard or URL-safe)
std::string base64_decode(const std::string& encoded, bool url_safe,
                          bool ignore_garbage) {
  // Build decode table
  signed char decode_table[256];
  memset(decode_table, -1, sizeof(decode_table));
  const char* alphabet = url_safe
      ? "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
      : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (int i = 0; i < 64; ++i) {
    decode_table[static_cast<unsigned char>(alphabet[i])] = i;
  }

  std::string result;
  unsigned char buffer[4] = {0};
  size_t buffer_pos = 0;

  for (char c : encoded) {
    if (c == '=' || c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
    signed char value = decode_table[static_cast<unsigned char>(c)];
    if (value < 0) {
      if (ignore_garbage) continue;
      return {};  // Signal invalid input
    }

    buffer[buffer_pos++] = static_cast<unsigned char>(value);

    if (buffer_pos == 4) {
      result.push_back((buffer[0] << 2) | (buffer[1] >> 4));
      result.push_back(((buffer[1] & 0x0F) << 4) | (buffer[2] >> 2));
      result.push_back(((buffer[2] & 0x03) << 6) | buffer[3]);
      buffer_pos = 0;
    }
  }

  if (buffer_pos >= 2) {
    result.push_back((buffer[0] << 2) | (buffer[1] >> 4));
  }
  if (buffer_pos >= 3) {
    result.push_back(((buffer[1] & 0x0F) << 4) | (buffer[2] >> 2));
  }

  return result;
}

// Base32 encode (RFC 4648 or base32hex)
std::string base32_encode(const std::string& data, int wrap, bool hex_alphabet) {
  const char* alphabet = hex_alphabet
      ? "0123456789ABCDEFGHIJKLMNOPQRSTUV"
      : "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
  std::string result;

  for (size_t i = 0; i < data.size(); i += 5) {
    unsigned char b[5] = {0};
    size_t block_size = 0;
    for (size_t j = 0; j < 5 && i + j < data.size(); ++j) {
      b[j] = data[i + j];
      block_size++;
    }

    if (block_size >= 1) {
      result += alphabet[b[0] >> 3];
      result +=
          alphabet[((b[0] & 0x07) << 2) | (block_size >= 2 ? b[1] >> 6 : 0)];
    }
    if (block_size >= 2) {
      result += alphabet[(b[1] >> 1) & 0x1F];
      result +=
          alphabet[((b[1] & 0x01) << 4) | (block_size >= 3 ? b[2] >> 4 : 0)];
    }
    if (block_size >= 3) {
      result +=
          alphabet[((b[2] & 0x0F) << 1) | (block_size >= 4 ? b[3] >> 7 : 0)];
      result += alphabet[(block_size >= 4) ? ((b[3] >> 2) & 0x1F) : '='];
    }
    if (block_size >= 4) {
      result +=
          alphabet[((b[3] & 0x03) << 3) | (block_size >= 5 ? b[4] >> 5 : 0)];
      result += alphabet[(block_size >= 5) ? (b[4] & 0x1F) : '='];
    }

    if (block_size == 1)
      result.append(6, '=');
    else if (block_size == 2)
      result.append(4, '=');
    else if (block_size == 3)
      result.append(3, '=');
    else if (block_size == 4)
      result.append(1, '=');
  }

  if (wrap > 0) {
    std::string wrapped;
    for (size_t i = 0; i < result.size(); i += wrap) {
      if (!wrapped.empty()) wrapped += '\n';
      wrapped += result.substr(i, wrap);
    }
    return wrapped;
  }

  return result;
}

// Base32 decode
std::string base32_decode(const std::string& encoded, bool hex_alphabet,
                          bool ignore_garbage) {
  signed char decode_table[256];
  memset(decode_table, -1, sizeof(decode_table));
  const char* alphabet = hex_alphabet
      ? "0123456789ABCDEFGHIJKLMNOPQRSTUV"
      : "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
  for (int i = 0; i < 32; ++i) {
    decode_table[static_cast<unsigned char>(alphabet[i])] = i;
    // Also accept lowercase
    if (alphabet[i] >= 'A' && alphabet[i] <= 'Z') {
      decode_table[static_cast<unsigned char>(alphabet[i] + 32)] = i;
    }
  }

  std::string result;
  unsigned char buffer[8] = {0};
  size_t buffer_pos = 0;

  for (char c : encoded) {
    if (c == '=' || c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
    signed char value = decode_table[static_cast<unsigned char>(c)];
    if (value < 0) {
      if (ignore_garbage) continue;
      return {};
    }
    buffer[buffer_pos++] = static_cast<unsigned char>(value);

    if (buffer_pos == 8) {
      result.push_back((buffer[0] << 3) | (buffer[1] >> 2));
      result.push_back(((buffer[1] & 0x03) << 6) | (buffer[2] << 1) |
                       (buffer[3] >> 4));
      result.push_back(((buffer[3] & 0x0F) << 4) | (buffer[4] >> 1));
      result.push_back(((buffer[4] & 0x01) << 7) | (buffer[5] << 2) |
                       (buffer[6] >> 3));
      result.push_back(((buffer[6] & 0x07) << 5) | buffer[7]);
      buffer_pos = 0;
    }
  }

  if (buffer_pos >= 2)
    result.push_back((buffer[0] << 3) | (buffer[1] >> 2));
  if (buffer_pos >= 4)
    result.push_back(((buffer[1] & 0x03) << 6) | (buffer[2] << 1) |
                     (buffer[3] >> 4));
  if (buffer_pos >= 5)
    result.push_back(((buffer[3] & 0x0F) << 4) | (buffer[4] >> 1));
  if (buffer_pos >= 7)
    result.push_back(((buffer[4] & 0x01) << 7) | (buffer[5] << 2) |
                     (buffer[6] >> 3));

  return result;
}

// Base16 (hex) encoding
std::string base16_encode(const std::string& data, int wrap) {
  const char* alphabet = "0123456789ABCDEF";
  std::string result;
  for (unsigned char c : data) {
    result += alphabet[c >> 4];
    result += alphabet[c & 0x0F];
  }
  if (wrap > 0) {
    std::string wrapped;
    for (size_t i = 0; i < result.size(); i += wrap) {
      if (!wrapped.empty()) wrapped += '\n';
      wrapped += result.substr(i, wrap);
    }
    return wrapped;
  }
  return result;
}

// Base16 (hex) decoding
std::string base16_decode(const std::string& encoded, bool ignore_garbage) {
  std::string result;
  for (size_t i = 0; i + 1 < encoded.size(); i += 2) {
    char c1 = encoded[i];
    char c2 = encoded[i + 1];

    if (c1 == '\n' || c1 == '\r' || c1 == ' ' || c1 == '\t') {
      if (ignore_garbage) { i--; continue; }
      return {};
    }
    if (c2 == '\n' || c2 == '\r' || c2 == ' ' || c2 == '\t') {
      if (ignore_garbage) { i++; continue; }
      return {};
    }

    auto hex_val = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      return -1;
    };
    int v1 = hex_val(c1);
    int v2 = hex_val(c2);
    if (v1 < 0 || v2 < 0) {
      if (ignore_garbage) continue;
      return {};
    }
    result.push_back(static_cast<unsigned char>((v1 << 4) | v2));
  }
  return result;
}

// Base2 encoding (MSB first or LSB first)
std::string base2_encode(const std::string& data, int wrap, bool lsb_first) {
  std::string result;
  for (unsigned char c : data) {
    if (lsb_first) {
      for (int i = 0; i < 8; ++i)
        result += ((c >> i) & 1) ? '1' : '0';
    } else {
      for (int i = 7; i >= 0; --i)
        result += ((c >> i) & 1) ? '1' : '0';
    }
  }
  if (wrap > 0) {
    std::string wrapped;
    for (size_t i = 0; i < result.size(); i += wrap) {
      if (!wrapped.empty()) wrapped += '\n';
      wrapped += result.substr(i, wrap);
    }
    return wrapped;
  }
  return result;
}

// Determine encoding from -b argument
Encoding parse_encoding(const std::string& str) {
  if (str == "64" || str == "base64") return Encoding::BASE64;
  if (str == "64url" || str == "base64url") return Encoding::BASE64URL;
  if (str == "32" || str == "base32") return Encoding::BASE32;
  if (str == "32hex" || str == "base32hex") return Encoding::BASE32HEX;
  if (str == "16" || str == "base16" || str == "hex") return Encoding::BASE16;
  if (str == "2" || str == "base2" || str == "bin" || str == "2msbf" ||
      str == "base2msbf")
    return Encoding::BASE2MSBF;
  if (str == "2lsbf" || str == "base2lsbf") return Encoding::BASE2LSBF;
  return Encoding::NONE;
}
}  // namespace

// ======================================================
// Main command implementation
// ======================================================

REGISTER_COMMAND(
    basenc, "basenc", "basenc [OPTION]... [FILE]",
    "Encode or decode FILE, or standard input, using various encodings.\n"
    "Encode or decode data using multiple encoding schemes: base64, base32,\n"
    "base16 (hex), or base2 (binary). Default is base64.",
    "  basenc file.txt\n"
    "  echo 'Hello' | basenc --base64\n"
    "  basenc -d --base32 encoded.txt\n"
    "  basenc -b16 file.bin",
    "base64(1), base32(1)", "WinuxCmd", "Copyright © 2026 WinuxCmd",
    BASENC_OPTIONS) {
  bool decode = ctx.get<bool>("-d", false) || ctx.get<bool>("--decode", false);
  bool ignore_garbage =
      ctx.get<bool>("--ignore-garbage", false);

  // Parse wrap value
  int wrap = 76;
  std::string wrap_str = ctx.get<std::string>("--wrap", "");
  if (wrap_str.empty()) {
    wrap_str = ctx.get<std::string>("-w", "");
  }
  if (!wrap_str.empty()) {
    try {
      wrap = std::stoi(wrap_str);
    } catch (...) {
    }
  }

  // Count encoding selectors
  int selector_count = 0;
  Encoding encoding = Encoding::NONE;

  auto check_selector = [&](const char* name, Encoding enc) {
    if (ctx.get<bool>(name, false)) {
      selector_count++;
      encoding = enc;
    }
  };

  // -b N selector
  std::string b_val = ctx.get<std::string>("-b", "");
  if (!b_val.empty()) {
    selector_count++;
    encoding = parse_encoding(b_val);
    if (encoding == Encoding::NONE) {
      safeErrorPrintLn("basenc: unknown encoding type");
      return 1;
    }
  }

  check_selector("--base64", Encoding::BASE64);
  check_selector("--base64url", Encoding::BASE64URL);
  check_selector("--base32", Encoding::BASE32);
  check_selector("--base32hex", Encoding::BASE32HEX);
  check_selector("--base16", Encoding::BASE16);
  check_selector("--hex", Encoding::BASE16);
  check_selector("--base2msbf", Encoding::BASE2MSBF);
  check_selector("--base2lsbf", Encoding::BASE2LSBF);
  check_selector("--base2", Encoding::BASE2MSBF);
  check_selector("--bin", Encoding::BASE2MSBF);

  // Unimplemented selectors
  if (ctx.get<bool>("--base58", false)) {
    safeErrorPrintLn("basenc: base58 encoding is not implemented");
    return 1;
  }
  if (ctx.get<bool>("--z85", false)) {
    safeErrorPrintLn("basenc: z85 encoding is not implemented");
    return 1;
  }

  // No selector: error
  if (selector_count == 0) {
    safeErrorPrintLn("basenc: missing encoding type");
    return 1;
  }

  // Multiple selectors: error
  if (selector_count > 1) {
    safeErrorPrintLn("basenc: multiple encoding options");
    return 1;
  }

  // Read input
  std::string input;
  if (ctx.positionals.empty() || ctx.positionals[0] == "-") {
    input.assign(std::istreambuf_iterator<char>(std::cin),
                 std::istreambuf_iterator<char>());
  } else {
    std::wstring wfile = utf8_to_wstring(std::string(ctx.positionals[0]));
    HANDLE hFile =
        CreateFileW(wfile.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
      safeErrorPrintLn("basenc: cannot open '" +
                       std::string(ctx.positionals[0]) + "'");
      return 1;
    }

    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);
    input.resize(fileSize.QuadPart);
    DWORD bytesRead;
    ReadFile(hFile, input.data(), static_cast<DWORD>(fileSize.QuadPart),
             &bytesRead, nullptr);
    CloseHandle(hFile);
  }

  // Process based on encoding
  std::string output;
  bool invalid = false;
  bool is_encode = !decode;

  switch (encoding) {
    case Encoding::BASE64:
      if (decode) {
        output = base64_decode(input, false, ignore_garbage);
        if (output.empty() && !input.empty() && !ignore_garbage) invalid = true;
      } else {
        output = base64_encode(input, wrap, false);
      }
      break;
    case Encoding::BASE64URL:
      if (decode) {
        output = base64_decode(input, true, ignore_garbage);
        if (output.empty() && !input.empty() && !ignore_garbage) invalid = true;
      } else {
        output = base64_encode(input, wrap, true);
      }
      break;
    case Encoding::BASE32:
      if (decode) {
        output = base32_decode(input, false, ignore_garbage);
        if (output.empty() && !input.empty() && !ignore_garbage) invalid = true;
      } else {
        output = base32_encode(input, wrap, false);
      }
      break;
    case Encoding::BASE32HEX:
      if (decode) {
        output = base32_decode(input, true, ignore_garbage);
        if (output.empty() && !input.empty() && !ignore_garbage) invalid = true;
      } else {
        output = base32_encode(input, wrap, true);
      }
      break;
    case Encoding::BASE16:
      if (decode) {
        output = base16_decode(input, ignore_garbage);
        if (output.empty() && !input.empty() && !ignore_garbage) invalid = true;
      } else {
        output = base16_encode(input, wrap);
      }
      break;
    case Encoding::BASE2MSBF:
      if (decode) {
        invalid = true;
      } else {
        output = base2_encode(input, wrap, false);
      }
      break;
    case Encoding::BASE2LSBF:
      if (decode) {
        invalid = true;
      } else {
        output = base2_encode(input, wrap, true);
      }
      break;
    default:
      safeErrorPrintLn("basenc: missing encoding type");
      return 1;
  }

  if (invalid) {
    safeErrorPrintLn("basenc: invalid input");
    return 1;
  }

  safePrint(output);
  if (is_encode) {
    safePrint("\n");
  }

  return 0;
}
