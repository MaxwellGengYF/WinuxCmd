#pragma once
#include "../utils/expected_polyfill.h"
#include "../utils/utils.h"
/*
 *  Copyright © 2026 [caomengxuan666]
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 *  - File: pipeline.cppm
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */


namespace core::pipeline {
struct Error {
  std::string message;
  Error() = default;
  Error(const char* s) : message(s) {}
  Error(std::string_view sv) : message(sv) {}
  Error(const std::string& s) : message(s) {}
  Error(std::string&& s) : message(std::move(s)) {}
  operator std::string_view() const { return message; }
};

// Helper to safely convert error to wstring
inline std::wstring error_to_wstring(const Error& e) {
  return utf8_to_wstring(e.message);
}

template <typename T>
using Result = Expected<T, Error>;

template <typename T>
void report_error(const Result<T>& result, std::wstring_view command_name) {
  if (!result) {
    const auto& error_msg = result.error();
    std::wstring wmsg = error_to_wstring(error_msg);
    safeErrorPrint(std::wstring(command_name) + L": " + wmsg + L"\n");
  }
}

template <typename T>
int report_error_with_code(const Result<T>& result,
                           std::wstring_view command_name, int error_code = 1) {
  if (!result) {
    report_error(result, command_name);
    return error_code;
  }
  return 0;
}

inline void report_custom_error(std::wstring_view command_name,
                                std::wstring_view error_message) {
  safeErrorPrint(std::wstring(command_name) + L": " +
                 std::wstring(error_message) + L"\n");
}

}  // namespace core::pipeline
