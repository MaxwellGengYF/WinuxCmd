#include <iostream>
#include <string_view>
#include <array>
#include <utility>
#include "container/constexpr_map.h"
using namespace std::string_view_literals;

int main() {
  std::cout << "=== ConstexprMap Examples ===\n";

  constexpr auto signals = make_constexpr_map<std::string_view, int>(
      std::array<std::pair<std::string_view, int>, 4>{
          {{"KILL"sv, 9}, {"TERM"sv, 15}, {"INT"sv, 2}, {"HUP"sv, 1}}});

  std::cout << "Signal KILL = " << signals["KILL"sv] << "\n";
  std::cout << "Signal TERM = " << signals.get_or("TERM"sv, -1) << "\n";
  std::cout << "Signal UNKNOWN = " << signals.get_or("UNKNOWN"sv, -1) << "\n";

  std::cout << "\nAll signals:\n";
  for (const auto& [name, num] : signals) {
    std::cout << "  " << name << " -> " << num << "\n";
  }

  constexpr auto errors = make_constexpr_map<int, std::string_view>(
      std::array<std::pair<int, std::string_view>, 3>{
          {{0, "Success"sv}, {2, "Not found"sv}, {13, "Permission denied"sv}}});

  std::cout << "\nError 0: " << errors.get_or(0, "Unknown"sv) << "\n";
  std::cout << "Error 2: " << errors.get_or(2, "Unknown"sv) << "\n";
  std::cout << "Error 99: " << errors.get_or(99, "Unknown"sv) << "\n";

  return 0;
}
