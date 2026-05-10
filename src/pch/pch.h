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
 *  - File: pch.h
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */

/**
 *@breif PreCompile headers to speed up compilation.
 *@note Pch doesn't affect the final executable size.
 */

#ifndef PCH_H
#define PCH_H
#pragma warning(disable : 4530)
#pragma warning(disable : 4541)  // Disable typeid warning with /GR-
#pragma warning(disable : 4129)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define STRICT
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>   // For basic windows functions
#include <winsock2.h>  // Must be before windows.h to avoid conflicts
// Include these headers after windows.h
// Fuck clang-format.
#include <fcntl.h>       // For _setmode
#include <fileapi.h>     // For FindFirstFileW, FindNextFileW
#include <handleapi.h>   // For GetStdHandle, INVALID_HANDLE_VALUE
#include <io.h>          // For _get_osfhandle
#include <lmcons.h>      // For UNLEN
#include <psapi.h>       // For GetProcessMemoryInfo
#include <sddl.h>        // For ConvertSidToStringSidW
#include <shlwapi.h>     // For PathFileExistsW
#include <sysinfoapi.h>  // For GetUserNameW
#include <tlhelp32.h>    // For CreateToolhelp32Snapshot, Process32First
#include <winternl.h>    // For PROCESS_BASIC_INFORMATION

// C standard headers
#include <cctype>   // For isspace
#include <cstdint>  // For uint64_t
#include <cstdio>   // For printf, fflush
#include <cstdlib>  // For basic functions
#include <cstring>  // For strlen
#include <cwchar>   // For wprintf, fwprintf
#include <cassert>
#include <cerrno>
#include <cfloat>
#include <charconv>
#include <climits>
#include <cmath>
#include <codecvt>
#include <compare>
#include <cstdarg>
#include <cstddef>
#include <ctime>
#include <cwctype>

// C++ standard library headers
#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <bitset>
#include <chrono>
#include <concepts>
#include <condition_variable>
#include <deque>
#include <exception>
#include <execution>
#include <filesystem>
#include <format>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <initializer_list>
#include <iomanip>
#include <ios>
#include <iostream>
#include <istream>
#include <iterator>
#include <latch>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <numeric>
#include <optional>
#include <ostream>
#include <queue>
#include <random>
#include <ranges>
#include <regex>
#include <semaphore>
#include <set>
#include <shared_mutex>
#include <source_location>
#include <span>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <string_view>
#include <syncstream>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <version>

#endif  // PCH_H
