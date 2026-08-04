/**
 * @file print.hpp
 * @author ilpeN
 * @brief Provides print/println/eprint/eprintln functions for C++20, 23, 17
 * @since 1.0.0
 * @details
 * It's fully-functional on C++20 and 23 (uses STL's format).
 * On C++23, it directly uses std::print family
 * On C++20, it uses std::format to format messages then fwrite
 * to stdout/stderr or custom FILE*. Note that it only supports UTF-8
 * (not UTF-16, wide char bullshit just 1 byte chars)
 * But if you have libfmt on C++17, it'll try to use that.
 * If you don't have it, it fallbacks to basic formatting
 * without format specification just type-generic "{}" style formatting
 * @code{cpp}
 * // C++23
 * icl::println("hello, {}", "world");
 * icl::println("hello, {:5f}", 10.0);
 * icl::println(L"can compile");
 *
 * // C++20
 * icl::println("hello, {}", "world");
 * icl::println("hello, {:5f}", 10.0);
 * icl::println(L"doesn't compile");
 *
 * // C++17 with fmtlib
 * icl::println("hello, {}", "world");
 * icl::println("hello, {:5f}", 10.0);
 * icl::println(L"doesn't compile");
 *
 * // C++17 without fmtlib
 * icl::println("hello, {}", "world");
 * icl::println("doesn't compile: {:5f}", 10.0);
 * icl::println(L"doesn't compile");
 * @endcode
*/

#ifndef ICL_PRINT_HPP
#define ICL_PRINT_HPP

#include "./base.hpp"
#include <cstdio>

/**
 * @brief Picks the formatting engine to use, from best to worst:
 * 1. C++23's <print> (native std::print/std::println)
 * 2. C++20's <format> (we build print/println ourselves on top of it)
 * 3. libfmt, if it's available, when compiling with C++17
 * 4. A minimal internal fallback (see further below) if none of the above exist
*/
#if __cplusplus >= 202002L
#include <format>
namespace icl { using ::std::format; using ::std::format_string; }
#if __cplusplus >= 202302L
#include <print>
namespace icl { using ::std::print; using ::std::println; }
#endif // __cplusplus >= 202302L

#elif __has_include(<fmt/core.h>) && __has_include(<fmt/format.h>)
#define ICL_LIBFMT_DEFINED
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
namespace icl {
  using ::fmt::format;
  using ::fmt::format_string;
  using ::fmt::print;
  using ::fmt::println;
}

#else
  // TODO: Add simple formatting system for C++17
  #error "Print doesn't supported below C++20 without libfmt yet"
#endif

namespace icl {

/**
 * @brief Our own print/println, built on top of std::format
 * @details
 * Used only when the compiler has <format> but not <print> yet
 * (this is the normal case on C++20, before C++23 is available).
 * Formats the message with std::format, then writes it out with fwrite.
*/
#if !defined(__cpp_lib_print) && __cplusplus >= 202002L
template <typename... Args>
inline void print(FILE *stream, format_string<Args...> fmt, Args&&... args) {
  std::string formatted = format(fmt, std::forward<Args>(args)...);
  fwrite(formatted.c_str(), 1, formatted.size(), stream);
}

template <typename... Args>
inline void print(format_string<Args...> fmt, Args&&... args) {
  print(stdout, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void println(FILE *stream, format_string<Args...> fmt, Args&&... args) {
  print(stream, fmt, std::forward<Args>(args)...);
  // ASSUMPTION: stream is in text-mode (LF -> CRLF conversions are always available on windows)
  fputc('\n', stream);
}

template <typename... Args>
inline void println(format_string<Args...> fmt, Args&&... args) {
  println(stdout, fmt, std::forward<Args>(args)...);
}

#elif !defined(ICL_LIBFMT_DEFINED) && !defined(__cpp_lib_print)
// Fallback to simple C++17 format
// TODO: Add format string parsing, checking

/**
 * @struct fstring
 * @brief Minimal stand-in for std::format_string, used only when
 * neither <format> nor libfmt are available
 * @details
 * Right now this just stores the raw format string and does not parse
 * or validate it at compile time. Real format string parsing is not
 * implemented yet — see the format() function below.
 * @tparam Args types of the arguments the format string expects
 * (not used yet, kept here for API compatibility with std::format_string)
*/
template <typename... Args>
struct fstring {
  std::string_view sv;

  template <size_t N>
  constexpr fstring(const char (&s)[N]) : sv(s, N - 1) {}

  constexpr fstring(std::string_view s) : sv(s) {}
};

/**
 * @brief Alias so user code can write icl::format_string<Args...> the
 * same way no matter which backend is active
*/
template <typename... Args>
using format_string = fstring<Args...>;

/**
 * @brief Not implemented yet
 * @details
 * This fallback path (C++17 without libfmt) does not have a working
 * formatter yet. Calling this always aborts. Add libfmt to your
 * project, or build with C++20/23, to get a working format() function.
*/
template <typename... Args>
[[noreturn]] inline std::string format(format_string<Args...> fmt, Args&&... args) {
  TODO("format");
}

/**
 * @brief Not implemented yet, see format() above
*/
template <typename... Args>
[[noreturn]] inline void print(FILE *stream, format_string<Args...> fmt, Args&&... args) {
  TODO("print");
}

template <typename... Args>
inline void print(format_string<Args...> fmt, Args&&... args) {
  print(stdout, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void println(FILE *stream, format_string<Args...> fmt, Args&&... args) {
  print(stream, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void println(format_string<Args...> fmt, Args&&... args) {
  println(stdout, fmt, std::forward<Args>(args)...);
}

#endif // __cplusplus && __cpp_lib_print

/**
 * @brief print()/println() shortcuts that write to stderr instead of stdout
 * @details
 * eprint() is the same as print(stderr, ...), and eprintln() is the
 * same as println(stderr, ...). Handy for error and debug messages.
*/
template <typename... Args>
inline void eprint(format_string<Args...> fmt, Args&&... args) {
  print(stderr, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void eprintln(format_string<Args...> fmt, Args&&... args) {
  println(stderr, fmt, std::forward<Args>(args)...);
}

} // namespace icl

#endif // ICL_PRINT_HPP
