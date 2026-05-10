/**
 * @file small_vector_example.cpp
 * @brief Example usage of SmallVector container
 *
 * This example demonstrates all features of SmallVector:
 * - Basic operations (push_back, emplace_back, pop_back)
 * - Small buffer optimization
 * - Iterators and range-based for loops
 * - Construction methods
 * - Copy and move semantics
 * - Element access
 * - Appending and inserting
 * - Memory characteristics
 *
 * @author WinuxCmd Project
 * @copyright Copyright © 2026 WinuxCmd
 */

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <initializer_list>
#include "container/small_vector.h"

// A non-trivially copyable type to demonstrate the difference
class Person {
 private:
  std::string name_;
  int age_;

 public:
  Person(const char* name, int age) : name_(name), age_(age) {
    std::cout << "  [ctor] Person(" << name_ << ", " << age_ << ")\n";
  }

  ~Person() { std::cout << "  [dtor] Person(" << name_ << ", " << age_ << ")\n"; }

  Person(const Person& other) : name_(other.name_), age_(other.age_) {
    std::cout << "  [copy] Person(" << name_ << ", " << age_ << ")\n";
  }

  Person(Person&& other) noexcept
      : name_(std::move(other.name_)), age_(other.age_) {
    std::cout << "  [move] Person(" << name_ << ", " << age_ << ")\n";
  }

  Person& operator=(const Person& other) {
    if (this != &other) {
      name_ = other.name_;
      age_ = other.age_;
      std::cout << "  [copy assign] Person(" << name_ << ", " << age_ << ")\n";
    }
    return *this;
  }

  Person& operator=(Person&& other) noexcept {
    if (this != &other) {
      name_ = std::move(other.name_);
      age_ = other.age_;
      std::cout << "  [move assign] Person(" << name_ << ", " << age_ << ")\n";
    }
    return *this;
  }

  std::string_view name() const { return name_; }
  int age() const { return age_; }

  bool operator==(const Person& other) const {
    return name_ == other.name_ && age_ == other.age_;
  }
};

// A simple struct to demonstrate trivially copyable types
struct Point {
  int x;
  int y;

  bool operator==(const Point& other) const = default;
};

// Helper function to print vector state
template <typename T, unsigned N>
void print_vector_state(const SmallVector<T, N>& vec, const char* name) {
  std::cout << "  " << name << ": size=" << vec.size() << ", capacity=" << vec.capacity() << ", ";
  if (vec.capacity() <= N) {
    std::cout << "inline storage (" << N * sizeof(T) << " bytes)";
  } else {
    std::cout << "heap storage (" << vec.capacity() * sizeof(T) << " bytes)";
  }
  std::cout << "\n";
}

// Helper to print vector contents
template <typename T, unsigned N>
void print_vector(const SmallVector<T, N>& vec, const char* name) {
  std::cout << "  " << name << " = [";
  for (size_t i = 0; i < vec.size(); ++i) {
    if (i > 0) std::cout << ", ";
    std::cout << vec[i];
  }
  std::cout << "]\n";
}

// Specialization for Person
template <unsigned N>
void print_vector(const SmallVector<Person, N>& vec, const char* name) {
  std::cout << "  " << name << " contains:\n";
  for (const auto& p : vec) {
    std::cout << "    - " << p.name() << " (" << p.age() << ")\n";
  }
}

int main() {
  std::cout << "========================================\n";
  std::cout << "SmallVector Example - Windows/MSVC Version\n";
  std::cout << "========================================\n";

  // ========== 1. Basic Usage ==========
  std::cout << "\n--- 1. Basic Usage with int (trivially copyable) ---\n";

  SmallVector<int, 4> numbers;
  print_vector_state(numbers, "numbers");

  std::cout << "  Adding 3 elements (stays inline):\n";
  numbers.push_back(10);
  numbers.push_back(20);
  numbers.emplace_back(30);
  print_vector(numbers, "numbers");
  print_vector_state(numbers, "numbers");

  std::cout << "  Adding 2 more elements (exceeds inline capacity):\n";
  numbers.push_back(40);
  numbers.push_back(50);
  print_vector(numbers, "numbers");
  print_vector_state(numbers, "numbers");

  std::cout << "  Access and modification:\n";
  std::cout << "    front = " << numbers.front() << "\n";
  std::cout << "    back = " << numbers.back() << "\n";
  std::cout << "    [2] = " << numbers[2] << "\n";
  numbers[2] = 99;
  print_vector(numbers, "after modification");

  // ========== 2. Range-based for loop ==========
  std::cout << "\n--- 2. Range-based for loop ---\n";
  std::cout << "  All numbers: ";
  for (int n : numbers) {
    std::cout << n << " ";
  }
  std::cout << "\n";

  // ========== 3. Construction Methods ==========
  std::cout << "\n--- 3. Various Construction Methods ---\n";

  std::cout << "  From initializer list (size 3, capacity 3):\n";
  SmallVector<int, 3> vec1 = {1, 2, 3};
  print_vector(vec1, "vec1");
  print_vector_state(vec1, "vec1");

  std::cout << "  With size and default value:\n";
  SmallVector<std::string, 2> vec2(3, "hello");
  print_vector(vec2, "vec2");
  print_vector_state(vec2, "vec2");

  std::cout << "  From iterator range:\n";
  std::vector<double> src = {1.1, 2.2, 3.3, 4.4};
  SmallVector<double, 2> vec3(src.begin(), src.end());
  print_vector(vec3, "vec3");
  print_vector_state(vec3, "vec3");

  // ========== 4. Non-trivially Copyable Types ==========
  std::cout << "\n--- 4. Non-trivially Copyable Types (Person) ---\n";

  std::cout << "  Creating persons with inline storage (capacity=2):\n";
  SmallVector<Person, 2> persons;
  print_vector_state(persons, "persons");

  std::cout << "\n  Adding first person (inline):\n";
  persons.emplace_back("Alice", 25);
  print_vector_state(persons, "persons");

  std::cout << "\n  Adding second person (inline):\n";
  persons.emplace_back("Bob", 30);
  print_vector_state(persons, "persons");

  std::cout << "\n  Adding third person (triggers heap allocation + moves):\n";
  persons.emplace_back("Charlie", 35);
  print_vector_state(persons, "persons");

  std::cout << "\n  Final persons:\n";
  print_vector(persons, "persons");

  // ========== 5. Copy and Move Semantics ==========
  std::cout << "\n--- 5. Copy and Move Semantics ---\n";

  SmallVector<int, 3> original = {1, 2, 3, 4, 5};
  std::cout << "  Original:\n";
  print_vector(original, "original");
  print_vector_state(original, "original");

  std::cout << "\n  Copy construction:\n";
  auto copy = original;
  print_vector(copy, "copy");
  print_vector_state(copy, "copy");

  std::cout << "\n  Move construction:\n";
  auto moved = std::move(original);
  print_vector(moved, "moved");
  print_vector_state(moved, "moved");
  std::cout << "  Original after move: size=" << original.size() << "\n";

  // ========== 6. Appending Operations ==========
  std::cout << "\n--- 6. Appending Operations ---\n";

  SmallVector<int, 3> vec4 = {1, 2, 3};
  std::cout << "  Initial:\n";
  print_vector(vec4, "vec4");
  print_vector_state(vec4, "vec4");

  std::cout << "\n  append(4,5):\n";
  vec4.append({4, 5});
  print_vector(vec4, "vec4");
  print_vector_state(vec4, "vec4");

  std::cout << "\n  append 3 copies of 9:\n";
  vec4.append(3, 9);
  print_vector(vec4, "vec4");
  print_vector_state(vec4, "vec4");

  // ========== 7. Element Operations ==========
  std::cout << "\n--- 7. Element Operations ---\n";

  SmallVector<int, 3> vec5 = {10, 20, 30, 40, 50};
  std::cout << "  Initial:\n";
  print_vector(vec5, "vec5");

  std::cout << "  pop_back (removes last):\n";
  vec5.pop_back();
  print_vector(vec5, "vec5");

  std::cout << "  clear():\n";
  vec5.clear();
  print_vector(vec5, "vec5");
  print_vector_state(vec5, "vec5");

  std::cout << "  reuse after clear:\n";
  vec5.push_back(100);
  vec5.push_back(200);
  print_vector(vec5, "vec5");

  // ========== 8. Reserve and Resize ==========
  std::cout << "\n--- 8. Reserve and Resize ---\n";

  SmallVector<int, 3> vec6;
  std::cout << "  Initial:\n";
  print_vector_state(vec6, "vec6");

  std::cout << "  reserve(10):\n";
  vec6.reserve(10);
  print_vector_state(vec6, "vec6");

  std::cout << "  resize(5):\n";
  vec6.resize(5);
  print_vector(vec6, "vec6");
  print_vector_state(vec6, "vec6");

  std::cout << "  resize(2) (shrink):\n";
  vec6.resize(2);
  print_vector(vec6, "vec6");

  // ========== 9. Comparison Operators ==========
  std::cout << "\n--- 9. Comparison Operators ---\n";

  SmallVector<int, 3> a = {1, 2, 3};
  SmallVector<int, 3> b = {1, 2, 3};
  SmallVector<int, 3> c = {1, 2, 4};

  std::cout << "  a == b: " << (a == b) << "\n";
  std::cout << "  a == c: " << (a == c) << "\n";
  std::cout << "  a != c: " << (a != c) << "\n";

  // ========== 10. Memory Footprint ==========
  std::cout << "\n--- 10. Memory Footprint ---\n";

  std::cout << "  sizeof(SmallVector<int, 0>)  = " << sizeof(SmallVector<int, 0>) << " bytes\n";
  std::cout << "  sizeof(SmallVector<int, 4>)  = " << sizeof(SmallVector<int, 4>) << " bytes\n";
  std::cout << "  sizeof(SmallVector<int, 8>)  = " << sizeof(SmallVector<int, 8>) << " bytes\n";
  std::cout << "  sizeof(SmallVector<int, 16>) = " << sizeof(SmallVector<int, 16>) << " bytes\n";
  std::cout << "  sizeof(SmallVector<Point, 4>) = " << sizeof(SmallVector<Point, 4>) << " bytes\n";
  std::cout << "  sizeof(SmallVector<Person, 4>) = " << sizeof(SmallVector<Person, 4>) << " bytes\n";

  // ========== 11. Edge Cases ==========
  std::cout << "\n--- 11. Edge Cases ---\n";

  std::cout << "  Empty vector:\n";
  SmallVector<int, 3> empty;
  std::cout << "    empty.size() = " << empty.size() << "\n";
  std::cout << "    empty.empty() = " << empty.empty() << "\n";

  std::cout << "  Single element:\n";
  empty.push_back(42);
  std::cout << "    front = " << empty.front() << ", back = " << empty.back() << "\n";

  std::cout << "\n  Large N (128 inline elements):\n";
  SmallVector<int, 128> large;
  std::cout << "    capacity = " << large.capacity() << ", inline storage = " << 128 * sizeof(int) << " bytes\n";

  // ========== 12. Real-world Usage Pattern ==========
  std::cout << "\n--- 12. Real-world Usage: Command Line Arguments ---\n";

  // Simulate parsing command line arguments
  const char* argv[] = {"program.exe", "-v", "--file", "test.txt", "-n", "42"};
  int argc = 6;

  SmallVector<std::string_view, 8> args;
  for (int i = 0; i < argc; ++i) {
    args.push_back(argv[i]);
  }

  std::cout << "  Parsed " << args.size() << " arguments:\n";
  for (size_t i = 0; i < args.size(); ++i) {
    std::cout << "    args[" << i << "] = '" << args[i] << "'\n";
  }

  // ========== 13. Nested Containers ==========
  std::cout << "\n--- 13. Nested Containers ---\n";

  SmallVector<SmallVector<int, 3>, 4> matrix;

  matrix.emplace_back(std::initializer_list<int>{1, 2, 3});
  matrix.emplace_back(std::initializer_list<int>{4, 5, 6});
  matrix.emplace_back(std::initializer_list<int>{7, 8, 9});

  std::cout << "  Matrix (3x3):\n";
  for (const auto& row : matrix) {
    std::cout << "    [";
    for (int val : row) {
      std::cout << " " << val;
    }
    std::cout << " ]\n";
  }

  std::cout << "\n========================================\n";
  std::cout << "Example completed successfully!\n";
  std::cout << "========================================\n";

  return 0;
}
