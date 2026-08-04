/**
 * @file base.hpp
 * @author ilpeN
 * @since 1.0.0
 * @brief Provides basic macros, functions, types
*/
#ifndef ICL_BASE_HPP
#define ICL_BASE_HPP

#include <type_traits>
#include <string>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cassert>
#include <cstddef>

/**
 * @brief Platform detection macros
 * @details
 * Depending on the target OS, one or more of PLATFORM_POSIX,
 * PLATFORM_UNIX, PLATFORM_LINUX, PLATFORM_APPLE and PLATFORM_WINDOWS
 * get defined as 1. PLATFORM_POSIX is defined by default and then
 * undefined again if the target turns out to be Windows.
*/
#define PLATFORM_POSIX 1
#if defined(__unix__) || defined(__unix)
  #define PLATFORM_UNIX 1
#endif
#if defined(__linux__)
  #define PLATFORM_LINUX 1
#endif
#if defined(__APPLE__) || defined(__MACH__)
  #define PLATFORM_APPLE 1
#endif
#if defined(_WIN32) || defined(__CYGWIN__)
  #define PLATFORM_WINDOWS 1
  #undef PLATFORM_POSIX
#endif

// You can define NATIVE_COMPILER in command line while bootstrapping
#ifndef NATIVE_COMPILER
  #define NATIVE_COMPILER NULL // Detect compiler via macros
#endif

/**
 * @brief Rust-style fixed-width integer type aliases
 * @details
 * Short names for the fixed-width integer types from <cstdint>.
 * i8/i16/i32/i64 are signed, u8/u16/u32/u64 are unsigned. s8/s16/s32/s64
 * are just other names for the signed types, for people who like the
 * "s" prefix instead of "i". If ICL_NO_GLOBAL_NAMESPACE is defined,
 * these live inside icl::; otherwise they are put in the global
 * namespace so you can use them without the icl:: prefix.
*/
#ifdef ICL_NO_GLOBAL_NAMESPACE
namespace icl {
#endif
using i8 = std::int8_t;
using s8 = std::int8_t;
using u8 = std::uint8_t;

using i16 = std::int16_t;
using s16 = std::int16_t;
using u16 = std::uint16_t;

using i32 = std::int32_t;
using s32 = std::int32_t;
using u32 = std::uint32_t;

using i64 = std::int64_t;
using s64 = std::int64_t;
using u64 = std::uint64_t;
#ifdef ICL_NO_GLOBAL_NAMESPACE
}
#endif

namespace icl {

/**
 * @brief Internal helpers used by todo()/unreachable(), not part of the public API
*/
namespace detail {
[[noreturn]]
inline void graceful_abort() {
#if !defined(ICL_NDEBUG)
  #ifdef PLATFORM_WINDOWS
  std::exit(3);
  #else // PLATFORM_POSIX
  std::exit(134);
  #endif
#endif
}

[[noreturn]]
inline void unreachable() {
#if !defined(ICL_NDEBUG)
  #ifdef _MSC_VER
  __assume(false);
  #else
  __builtin_unreachable();
  #endif
#endif
}
} // namespace detail

/**
 * @brief Marks a code path as "not implemented yet" and
 * exits with code 134 (POSIX) or 3 (windows)
 * @details
 * Use this to stub out functions you haven't written yet. Works like
 * printf: give it a format string and arguments, and it prints the
 * message before exiting the program.
 * @param fmt printf-style format string
*/
#define TODO(fmt, ...) \
  do { \
    fprintf(stderr, "%s:%d: TODO: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    icl::detail::graceful_abort(); \
  } while (0)

/**
 * @brief Marks a code path as unreachable
 * @details
 * Use this on branches that should never run, for example an
 * "impossible" default case in a switch. Helps the compiler optimize
 * and helps you catch logic bugs early, since reaching it at runtime
 * means something went wrong. Works like printf: give it a format
 * string and arguments.
 * @param fmt printf-style format string
*/
#define UNREACHABLE(fmt, ...) \
  do { \
    fprintf(stderr, "%s:%d: UNREACHABLE: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    icl::detail::unreachable(); \
  } while (0)

/**
 * @brief Determines whether a type provides a valid equality operator.
 *
 * Evaluates to `true` if the expression `a == b` is well-formed for
 * two constant objects of type `T`; otherwise `false`.
 *
 * @tparam T Type to cast
*/
template <typename T, typename = void>
struct is_equality_comparable : std::false_type {};
template <typename T>
struct is_equality_comparable<T,
       std::void_t<decltype(std::declval<const T&>() == std::declval<const T&>()), bool>>
       : std::true_type {};
template <typename T>
inline constexpr bool is_equality_comparable_v = is_equality_comparable<T>::value;

/**
 * @brief Determines whether L type has overload append operator for R type.
 *
 * Evaluates to `true` if the expression `a += b` is well-formed for
 * two constant objects of type `L` and `R`; otherwise `false`.
 *
 * @tparam L left hand side type (appended to)
 * @tparam R right hand side type (to be appended)
*/
template <typename L, typename R, typename = void>
struct is_appendable : std::false_type {};
template <typename L, typename R>
struct is_appendable<L, R,
       std::void_t<decltype(std::declval<L&>() += std::declval<const R&>()), bool>>
       : std::true_type {};
template <typename L, typename R>
inline constexpr bool is_appendable_v = is_appendable<L, R>::value;

/**
 * @def defer { ... };
 * @brief Calls code inside the block whenever it reaches end of the scope
 * @details
 * It works like defer in Go language, but we have RAII in C++ so
 * we're using it. Captures everything (lazy) from outer stack
 * by reference. The macro expands itself with [&]() at the end.
 *
 * @code{.cpp}
 * #include <dirent.h>
 * bool func() {
 *   DIR *dir = opendir("./my_dir");
 *   defer { closedir(dir); };
 *   while (struct dirent *entry = readdir(dir)) {
 *     if (strcmp(entry->d_name, "top_secret_file.txt") == 0) return false;
 *     icl::println("{}", entry->d_name);
 *   }
 *   return true;
 * }
 * @encode
*/
struct DeferHelper {
  template <typename F>
  struct Guard {
    F f;
    ~Guard() { f(); }
  };
  template <typename F>
  Guard<F> operator +(F f) { return Guard<F>{std::move(f)}; }
};
#define _ICL_DEFER_CONCAT_(a, b) a##b
#define _ICL_DEFER_CONCAT(a, b) _ICL_DEFER_CONCAT_(a, b)
#define defer auto _ICL_DEFER_CONCAT(_defer_, __LINE__) = icl::DeferHelper{} + [&]()

} // namespace icl

#endif // ICL_BASE_HPP
