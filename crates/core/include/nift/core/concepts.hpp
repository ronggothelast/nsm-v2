#pragma once
/// @file concepts.hpp
/// @brief C++20 concepts for Nift template constraints.

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

namespace nift::core {

/// A type that can be converted to a string view (string, string_view, const char*).
template <typename T>
concept StringLike = std::convertible_to<T, std::string_view>;

/// A type that is callable with no arguments and returns void.
template <typename F>
concept NullaryCallable = std::invocable<F> && std::same_as < std::invoke_result_t<F>,
void > ;

/// A type that represents an error (has to_string).
template <typename E>
concept ErrorType = requires(E e) {
  { to_string(e) } -> std::convertible_to<std::string_view>;
};

/// A type that is hashable (has std::hash specialization).
template <typename T>
concept Hashable = requires(T a) {
  { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};

}  // namespace nift::core
