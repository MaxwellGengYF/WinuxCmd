#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

namespace core::pipeline {

template <typename E>
struct Unexpected {
  E error;
  explicit Unexpected(const E& e) : error(e) {}
  explicit Unexpected(E&& e) : error(std::move(e)) {}
};

template <std::size_t N>
auto unexpected(const char (&s)[N]) -> Unexpected<std::string_view> {
  return Unexpected<std::string_view>(std::string_view(s, N - 1));
}

inline auto unexpected(const char* s) -> Unexpected<std::string_view> {
  return Unexpected<std::string_view>(std::string_view(s));
}

template <typename E>
auto unexpected(const E& e) -> Unexpected<std::remove_cv_t<std::remove_reference_t<E>>> {
  return Unexpected<std::remove_cv_t<std::remove_reference_t<E>>>(e);
}

template <typename E>
auto unexpected(E&& e) -> Unexpected<std::remove_cv_t<std::remove_reference_t<E>>> {
  return Unexpected<std::remove_cv_t<std::remove_reference_t<E>>>(std::forward<E>(e));
}

template <typename T, typename E>
class Expected {
  std::variant<T, E> data_;

 public:
  Expected() = default;
  Expected(const T& v) : data_(v) {}
  Expected(T&& v) : data_(std::move(v)) {}

  template <typename G>
  Expected(const Unexpected<G>& u) : data_(E(u.error)) {}
  template <typename G>
  Expected(Unexpected<G>&& u) : data_(E(std::forward<G>(u.error))) {}

  bool has_value() const { return std::holds_alternative<T>(data_); }
  explicit operator bool() const { return has_value(); }

  T& value() & { return std::get<T>(data_); }
  const T& value() const& { return std::get<T>(data_); }
  T&& value() && { return std::get<T>(std::move(data_)); }

  E& error() & { return std::get<E>(data_); }
  const E& error() const& { return std::get<E>(data_); }
  E&& error() && { return std::get<E>(std::move(data_)); }

  T& operator*() & { return value(); }
  const T& operator*() const& { return value(); }
  T&& operator*() && { return std::move(value()); }
  T* operator->() { return &value(); }
  const T* operator->() const { return &value(); }

  template <typename F>
  auto and_then(F&& f) & {
    using U = std::invoke_result_t<F, T&>;
    if (has_value()) {
      return std::forward<F>(f)(value());
    } else {
      return U(unexpected(error()));
    }
  }

  template <typename F>
  auto and_then(F&& f) && {
    using U = std::invoke_result_t<F, T&&>;
    if (has_value()) {
      return std::forward<F>(f)(std::move(value()));
    } else {
      return U(unexpected(std::move(error())));
    }
  }

  template <typename F>
  auto transform(F&& f) & {
    using U = std::invoke_result_t<F, T&>;
    using ResultType = Expected<U, E>;
    if (has_value()) {
      if constexpr (std::is_void_v<U>) {
        std::forward<F>(f)(value());
        return ResultType();
      } else {
        return ResultType(std::forward<F>(f)(value()));
      }
    } else {
      return ResultType(unexpected(error()));
    }
  }

  template <typename F>
  auto transform(F&& f) && {
    using U = std::invoke_result_t<F, T&&>;
    using ResultType = Expected<U, E>;
    if (has_value()) {
      if constexpr (std::is_void_v<U>) {
        std::forward<F>(f)(std::move(value()));
        return ResultType();
      } else {
        return ResultType(std::forward<F>(f)(std::move(value())));
      }
    } else {
      return ResultType(unexpected(std::move(error())));
    }
  }
};

template <typename E>
class Expected<void, E> {
  std::optional<E> error_;

 public:
  Expected() : error_(std::nullopt) {}

  template <typename G>
  Expected(const Unexpected<G>& u) : error_(E(u.error)) {}
  template <typename G>
  Expected(Unexpected<G>&& u) : error_(E(std::forward<G>(u.error))) {}

  bool has_value() const { return !error_.has_value(); }
  explicit operator bool() const { return has_value(); }

  void value() const& {
    if (error_) throw std::runtime_error("bad expected access");
  }

  E& error() & { return *error_; }
  const E& error() const& { return *error_; }
  E&& error() && { return std::move(*error_); }

  template <typename F>
  auto and_then(F&& f) {
    using U = std::invoke_result_t<F>;
    if (has_value()) {
      return std::forward<F>(f)();
    } else {
      return U(unexpected(error()));
    }
  }

  template <typename F>
  auto transform(F&& f) {
    using U = std::invoke_result_t<F>;
    using ResultType = Expected<U, E>;
    if (has_value()) {
      if constexpr (std::is_void_v<U>) {
        std::forward<F>(f)();
        return ResultType();
      } else {
        return ResultType(std::forward<F>(f)());
      }
    } else {
      return ResultType(unexpected(error()));
    }
  }
};

}  // namespace core::pipeline
