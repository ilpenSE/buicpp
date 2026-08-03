/* SPDX-License-Identifier: MIT */
#ifndef ICL_HPP
#define ICL_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <utility>
#include <ostream>
#include <format>
#include <version>
#include <type_traits>

#if __cplusplus >= 202302L
#include <print>
#endif

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

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <time.h>
#include <dirent.h>
#endif // PLATFORM_WINDOWS

#define TODO(fmt, ...) \
  do { \
    fprintf(stderr, "%s:%d: TODO: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    abort(); \
  } while (0)

#define UNREACHABLE(fmt, ...) \
  do { \
    fprintf(stderr, "%s:%d: UNREACHABLE: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    __builtin_unreachable(); \
  } while (0)

/*
  You can define NATIVE_COMPILER in command line while bootstrapping
*/
#ifndef NATIVE_COMPILER
  #define NATIVE_COMPILER NULL /* Detect compiler via macros */
#endif

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
 * @brief Portable C++23 and C++20 println syntax
*/
#if __cpp_lib_print // C++23's println
using ::std::print;
using ::std::println;
template <typename... Args>
inline void eprint(std::format_string<Args...> fmt, Args&&... args) {
  print(stderr, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void eprintln(std::format_string<Args...> fmt, Args&&... args) {
  println(stderr, fmt, std::forward<Args>(args)...);
}

#else // Our implementation of std::println
  #if __cplusplus >= 202002L
  template <typename... Args>
  inline void print(FILE *stream, std::format_string<Args...> fmt, Args&&... args) {
    std::string formatted = std::format(fmt, std::forward<Args>(args)...);
    fwrite(formatted.c_str(), 1, formatted.size(), stream);
  }

  template <typename... Args>
  inline void println(FILE *stream, std::format_string<Args...> fmt, Args&&... args) {
    print(stream, fmt, std::forward<Args>(args)...);
    #ifdef PLATFORM_WINDOWS
    fputc('\r', stream);
    #else
    fputc('\n', stream);
    #endif
  }

  template <typename... Args>
  inline void println(std::format_string<Args...> fmt, Args&&... args) {
    println(stdout, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  inline void eprint(std::format_string<Args...> fmt, Args&&... args) {
    print(stderr, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  inline void eprintln(std::format_string<Args...> fmt, Args&&... args) {
    println(stderr, fmt, std::forward<Args>(args)...);
  }
  #else
    #error "print family requires >=C++20"
  #endif // __cplusplus
#endif // __cpp_lib_print

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

// Either start
struct LTag {};
struct RTag {};

template <typename L, typename R>
class Either {
public:
  // If left is default constructible, enable left otherwise enable right
  template <typename LL = L,
            typename = std::enable_if_t<std::is_default_constructible_v<LL>>>
  Either() : m_is_left(true), m_left() {}

  template <typename LL = L, typename RR = R,
            typename = std::enable_if_t<!std::is_default_constructible_v<LL> &&
                                        std::is_default_constructible_v<RR>>,
            typename = void>
  Either() : m_is_left(false), m_right() {}

  Either(const L& value, LTag) : m_is_left(true), m_left(value) { check_traits(); }
  Either(L&& value, LTag) : m_is_left(true), m_left(std::move(value)) { check_traits(); }
  Either(const R& value, RTag) : m_is_left(false), m_right(value) { check_traits(); }
  Either(R&& value, RTag) : m_is_left(false), m_right(std::move(value)) { check_traits(); }

  ~Either() {
    static_assert(std::is_destructible_v<L>, "Either<L, R>: L must be destructible");
    static_assert(std::is_destructible_v<R>, "Either<L, R>: R must be destructible");
    destruct_active();
  }

  // copy ctor
  Either(const Either& other) : m_is_left(other.m_is_left)
  {
    static_assert(std::is_copy_constructible_v<L>,
      "Either<L, R>: copy constructor requires L to be copy constructible");
    static_assert(std::is_copy_constructible_v<R>,
      "Either<L, R>: copy constructor requires R to be copy constructible");
    if (other.m_is_left) new (&m_left) L(other.m_left);
    else new (&m_right) R(other.m_right);
  }

  // copy assignment
  Either& operator =(const Either& other)
    noexcept(std::is_nothrow_copy_constructible_v<L> &&
             std::is_nothrow_copy_assignable_v<L> &&
             std::is_nothrow_copy_constructible_v<R> &&
             std::is_nothrow_copy_assignable_v<R>)
  {
    static_assert(std::is_copy_assignable_v<L> && std::is_copy_constructible_v<L>,
      "Either<L, R>: copy constructor requires L to be copy constructible and assignable");
    static_assert(std::is_copy_assignable_v<R> && std::is_copy_constructible_v<R>,
      "Either<L, R>: copy constructor requires R to be copy constructible and assignable");
    if (this == &other) return *this; // prevent self-assignment
    // same sides (both are left or right)
    if (m_is_left == other.m_is_left) {
      if (m_is_left) m_left = other.m_left;
      else m_right = other.m_right;
      return *this;
    }

    // different sides (one is left other is right or vice versa)
    if (other.m_is_left) {
      L tmp(other.m_left);
      destruct_active();
      new (&m_left) L(std::move_if_noexcept(tmp));
    } else {
      R tmp(other.m_right);
      destruct_active();
      new (&m_right) R(std::move_if_noexcept(tmp));
    }

    m_is_left = !m_is_left;
    return *this;
  }

  // move ctor
  Either(Either&& other)
    noexcept(std::is_nothrow_move_constructible_v<L> &&
             std::is_nothrow_move_constructible_v<R>)
    : m_is_left(other.m_is_left)
  {
    if (other.m_is_left) new (&m_left) L(std::move_if_noexcept(other.m_left));
    else new (&m_right) R(std::move_if_noexcept(other.m_right));
  }

  // move assignment
  Either& operator = (Either&& other) noexcept {
    if (this == &other) return *this;
    if (m_is_left == other.m_is_left) {
      // same sides (both are left or right)
      if (m_is_left) m_left = std::move(other.m_left);
      else m_right = std::move(other.m_right);
    } else {
      // different sides (one is left other is right or vice versa)
      destruct_active();
      if (other.m_is_left) new (&m_left) L(std::move(other.m_left));
      else new (&m_right) R(std::move(other.m_right));
      m_is_left = !m_is_left;
    }
    return *this;
  }

  // Equals or not equals operator overloads
  template <typename LL = L, typename RR = R>
  std::enable_if_t<is_equality_comparable_v<LL> && is_equality_comparable_v<RR>, bool>
  operator ==(const Either& other) const {
    if (m_is_left != other.m_is_left) return false;
    if (m_is_left) return m_left == other.m_left;
    else return m_right == other.m_right;
  }
  template <typename LL = L, typename RR = R>
  std::enable_if_t<is_equality_comparable_v<LL> && is_equality_comparable_v<RR>, bool>
  operator !=(const Either& other) const {
    return !(*this == other);
  }

  // Swap 2 different Either instances
  void swap(Either& other) {
    if (this == &other) return;
    Either tmp(std::move(*this));
    *this = std::move(other);
    other = std::move(tmp);
  }

  // Check if it's holding left or right
  bool is_left() const { return m_is_left; }
  bool is_right() const { return !m_is_left; }

  // Take (steal) or view for left
  L&& left() && {
    assert(m_is_left && "Tried to access left but it's holding right one");
    return std::move(m_left);
  }
  const L& left() const& {
    assert(m_is_left && "Tried to access left but it's holding right one");
    return m_left;
  }

  // Take (steal) or view for right
  R&& right() && {
    assert(!m_is_left && "Tried to access right but it's holding left one");
    return std::move(m_right);
  }
  const R& right() const& {
    assert(!m_is_left && "Tried to access right but it's holding left one");
    return m_right;
  }

  template <typename LF, typename RF>
  auto match(LF&& left_fn, RF&& right_fn) const& {
    if (m_is_left) return left_fn(m_left);
    else return right_fn(m_right);
  }

  template <typename LF, typename RF>
  auto match(LF&& left_fn, RF&& right_fn) && {
    if (m_is_left) return left_fn(std::move(m_left));
    else return right_fn(std::move(m_right));
  }

private:
  bool m_is_left = true;
  union {
    L m_left;
    R m_right;
  };
  void destruct_active() { if (m_is_left) m_left.~L(); else m_right.~R(); }

  static void check_traits() {
    static_assert(std::is_move_constructible_v<L> || std::is_copy_constructible_v<L>,
      "Either<L, R>: L must be movable or copyable");
    static_assert(std::is_move_constructible_v<R> || std::is_copy_constructible_v<R>,
      "Either<L, R>: R must be movable or copyable");
  }
}; // class Either

// Left factory for Either
template <typename L>
struct LeftOf {
  L value;
  LeftOf(const L& v) : value(v) {}
  LeftOf(L&& v) : value(std::move(v)) {}

  template <typename R>
  operator Either<L, R>() && { return Either<L, R>(std::move(value), LTag{}); }
  template <typename R>
  operator Either<L, R>() const& { return Either<L, R>(value, LTag{}); }
};
template <typename L>
LeftOf(L) -> LeftOf<L>;

// Right factory for Either
template <typename R>
struct RightOf {
  R value;
  RightOf(const R& v) : value(v) {}
  RightOf(R&& v) : value(std::move(v)) {}

  template <typename L>
  operator Either<L, R>() && { return Either<L, R>(std::move(value), RTag{}); }
  template <typename L>
  operator Either<L, R>() const& { return Either<L, R>(value, RTag{}); }
};
template <typename R>
RightOf(R) -> RightOf<R>;

struct Nothing{};
template <typename T>
class Option : public Either<T, Nothing> {
  using Base = Either<T, Nothing>;
public:
  using Base::Base;

  bool is_some() const { return this->is_left(); }
  bool is_none() const { return this->is_right(); }
  T&& value() && { return std::move(*this).left(); }
  const T& value() const& { return this->left(); }
};

template <typename T>
struct Some {
  T value;
  Some(const T& v) : value(v) {}
  Some(T&& v) : value(std::move(v)) {}

  operator Option<T>() && { return Option<T>(std::move(value), LTag{}); }
  operator Option<T>() const& { return Option<T>(value, LTag{}); }
};

struct NoneTag {
  template <typename T>
  operator Option<T>() const { return Option<T>(Nothing{}, RTag{}); }
};
inline constexpr NoneTag None = {};

struct Error {
  int code;
  const char* msg;
  const char* file;
  size_t line;
};

template <typename T, typename E = Error>
class Result : public Either<T, E> {
  using Base = Either<T, E>;
public:
  using Base::Base;

  bool is_ok() const { return this->is_left(); }
  bool is_err() const { return this->is_right(); }
  T&& value() && { return std::move(*this).left(); }
  const T& value() const& { return this->left(); }
  const E& error() const& { return this->right(); }
};

template <typename T>
struct Ok {
  T value;
  Ok(const T& v) : value(v) {}
  Ok(T&& v) : value(std::move(v)) {}

  template <typename E = Error>
  operator Result<T, E>() && { return Result<T, E>(std::move(value), LTag{}); }
  template <typename E = Error>
  operator Result<T, E>() const& { return Result<T, E>(value, LTag{}); }
};

template <typename E = Error>
struct Err {
  E value;
  Err(const E& e) : value(e) {}
  Err(E&& e) : value(std::move(e)) {}

  template <typename T>
  operator Result<T, E>() const& { return Result<T, E>(value, RTag{}); }
  template <typename T>
  operator Result<T, E>() && { return Result<T, E>(std::move(value), RTag{}); }
};
template <typename E = Error> Err(E) -> Err<E>;

// Either end

// Dynamic Arrays start
template <typename T>
class ArrayList {
public:
  // Constructor / Destructor
  ArrayList(size_t init_capacity = 32) :
    m_items(static_cast<T*>(::operator new(init_capacity * sizeof(T)))),
    m_count(0), m_capacity(init_capacity)
  {
    static_assert(std::is_destructible_v<T>, "ArrayList<T>: T must be destructible");
    static_assert(std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>,
      "ArrayList<T>: T must be movable or copyable");
  }

  ArrayList(std::initializer_list<T> list) :
     m_items(static_cast<T*>(::operator new(list.size() * sizeof(T)))),
     m_count(0), m_capacity(list.size())
  {
    static_assert(std::is_destructible_v<T>, "ArrayList<T>: T must be destructible");
    static_assert(std::is_copy_constructible_v<T>,
      "ArrayList<T>: initializer_list constructor requires T to be copy constructible");
    construct_range(list.begin(), list.end());
  }

  ~ArrayList() {
    static_assert(std::is_destructible_v<T>, "ArrayList<T>: T must be destructible");
    for (size_t i = 0; i < m_count; i++) m_items[i].~T();
    ::operator delete(m_items);
  }

  // Copy constructor (tight copy)
  ArrayList(const ArrayList& other) :
    m_items(static_cast<T*>(::operator new(other.m_count*sizeof(T)))),
    m_count(0), m_capacity(other.m_count)
  {
    static_assert(std::is_copy_constructible_v<T>,
      "ArrayList<T>: copy constructor requires T to be copy constructible");
    construct_range(other.m_items, other.m_items + other.m_count);
  }

  // Copy operator
  ArrayList& operator =(const ArrayList& other) {
    static_assert(std::is_copy_assignable_v<T> && std::is_copy_constructible_v<T>,
      "ArrayList<T>: copy assignment requires T to be copy constructible and assignable");
    if (this == &other) return *this;
    ArrayList<T> temp(other);
    std::swap(m_items, temp.m_items);
    std::swap(m_count, temp.m_count);
    std::swap(m_capacity, temp.m_capacity);
    return *this;
  }

  // Move constructor
  ArrayList(ArrayList&& other) noexcept :
    m_items(other.m_items),
    m_count(other.m_count),
    m_capacity(other.m_capacity)
  {
    other.m_items = nullptr;
    other.m_count = 0;
    other.m_capacity = 0;
  }

  // Move operator
  ArrayList& operator =(ArrayList&& other) noexcept {
    std::swap(m_items, other.m_items);
    std::swap(m_count, other.m_count);
    std::swap(m_capacity, other.m_capacity);
    return *this;
  }

  // Access operators, doesn't check boundaries (go at() for boundary check)
  T& operator[](size_t idx) { return m_items[idx]; }
  const T& operator[](size_t idx) const { return m_items[idx]; }

  // Equals operators
  template <typename U = T>
  std::enable_if_t<is_equality_comparable_v<U>, bool>
  operator ==(const ArrayList& other) {
    if (this == &other) return true;
    if (m_count != other.m_count) return false;
    for (size_t i = 0; i < m_count; ++i) {
        if (!(m_items[i] == other.m_items[i])) return false;
    }
    return true;
  }

  template <typename U = T>
  std::enable_if_t<is_equality_comparable_v<U>, bool>
  operator !=(const ArrayList& other) {
    return !(*this == other);
  }

  // Reserve extra "extra" amount of capacity
  bool reserve(size_t extra);

  // Add element into idx (shifts array if you're not pushing to back)
  bool add(const T& item, size_t idx) { return add_impl(item, idx); }
  bool add(T&& item, size_t idx) { return add_impl(std::move(item), idx); }
  bool push(const T& item) { return add_impl(item, m_count); }
  bool push(T&& item) { return add_impl(std::move(item), m_count); }
  template<typename... Args>
  bool push_many(Args&&... args);

  // Remove element from idx (shifts array if you're not removing from back)
  T pop() { return remove(m_count - 1); }
  T remove(size_t idx);
  T remove_unord(size_t idx);

  // shift "amount" elements in range [start, end) to left or right
  // Overwrites elements on its way and it can cause holes and
  // the holes are stay constructed but unspecified (moved-from state)
  bool shift_right(size_t start, size_t end, size_t amount = 1);
  bool shift_left(size_t start, size_t end, size_t amount = 1);

  // Concatenates all types into one single big string
  // T must be appendable to std::string otherwise
  // this function will be ignored (SFINAE)
  template <typename U = T>
  std::enable_if_t<is_appendable_v<std::string, U>, std::string>
  join(char delim) {
    std::string out;
    for (size_t i = 0; i < m_count; i++) {
      const auto& item = (*this)[i];
      out += item;
      if (i != count() - 1) out += delim;
    }
    return out;
  }

  // Access an element (with boundary check)
  T* at(size_t idx) {
    if (idx >= m_count) return nullptr;
    return &m_items[idx];
  }

  // [begin, end) iterators, c = const
  T* begin() const { return m_items; }
  T* end() const { return m_items + m_count; }
  const T* cbegin() const { return m_items; }
  const T* cend()   const { return m_items + m_count; }

  size_t count() const { return m_count; }
  size_t capacity() const { return m_capacity; }
  T* items() const { return m_items; }

  void set_count(size_t val) { m_count = val; }
  void set_capacity(size_t val) { m_capacity = val; }

private:
  T* m_items;
  size_t m_count = 0;
  size_t m_capacity;

  // Add an item to arbitrary idx (boundary checks available)
  template <typename U>
  bool add_impl(U&& item, size_t idx);

  // Construct items in range [first, last) into this instance
  // Uses placement new, exception safe
  // (calls dtors of constructed items before and deallocates items pointer on exception)
  template <typename It>
  void construct_range(It first, It last);
}; // class ArrayList<T>

template <typename T>
std::ostream& operator<<(std::ostream& os, const ArrayList<T>& arr);
// Dynamic Arrays end

inline const char* shift(int& argc, char**& argv) {
  if (argc == 0) return nullptr;
  argc--;
  return *argv++;
}

enum class Compiler {
  UNKNOWN = 0, GCC, CLANG, MSVC, INTEL_LLVM, INTEL_CLASSIC, Count
};

const char* to_string(Compiler e);
Compiler compiler_from_cstr(const char* str);
std::pair<const char*, Compiler> get_native_compiler();

struct CmdRunOptions {
  bool is_log = true;
  bool is_reset = true;
};

class CommandBuilder : public ArrayList<std::string> {
public:
  using ArrayList<std::string>::ArrayList;
  bool run(CmdRunOptions opts = {});
};

/**
 * @brief os namespace
 * @details
 * Assumes that you have x86_64/ARM64 CPU (64-bits)
*/
namespace os {
#ifdef PLATFORM_WINDOWS
  // sys/types.h
  using mode_t = u32;
  using uid_t = u32;
  using gid_t = u32;
  using ino_t = u64;
  using ssize_t = intptr_t;
  using off_t = i64;
  using nlink_t = u32;

  // time.h
  using time_t = int;
  struct timespec {
    time_t tv_sec;
    long   tv_nsec;
  };

  // sys/stat.h
  struct stat {
    mode_t st_mode;
    off_t st_size;
    uid_t st_uid;
    gid_t st_gid;
    ino_t st_ino;
    nlink_t st_nlink;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
  };
  int lstat(const char *path, struct stat *st);
  int stat(const char *path, struct stat *st);

  constexpr auto ifmt  = 0u;
  constexpr auto ifreg = FILE_ATTRIBUTE_NORMAL;
  constexpr auto ifdir = FILE_ATTRIBUTE_DIRECTORY;
  constexpr auto iflnk = FILE_ATTRIBUTE_REPARSE_POINT;
  constexpr auto ifblk = FILE_ATTRIBUTE_DEVICE;
  constexpr auto ifchr = 0u;

  // TODO: maybe change these while implementing windows api
  // note that iwusr logic is reversed
  constexpr auto iwusr = FILE_ATTRIBUTE_READONLY;
  constexpr auto irusr = 0u;
  constexpr auto ixusr = 0u;

  constexpr auto f_ok = 0x0;
  constexpr auto r_ok = 0x4;
  constexpr auto w_ok = 0x2;
  constexpr auto x_ok = 0x1;

  // unistd.h
  ssize_t readlink(const char *path, char *buf, size_t bufsz);
  int access(const char *path, mode_t mode);
  int mkdir(const char *path, mode_t mode);

  // dirent.h
  struct dirent {
    unsigned char d_type;
    char d_name[256];
  };
  struct DIR {
    WIN32_FIND_DATAW data;
    struct dirent entry;
    HANDLE hFind;
    bool first;
  };
  DIR *opendir(const char *dirname);
  struct dirent *readdir(DIR *dirp);
  int closedir(DIR *dirp);

#else
// sys/types.h
using ::mode_t;
using ::time_t;
using ::ino_t;
using ::ssize_t;
using ::uid_t;
using ::gid_t;

// dirent.h
using ::DIR;
using ::dirent;
using ::opendir;
using ::closedir;
using ::readdir;

// unistd.h
using ::readlink;
using ::access;

// sys/stat.h
constexpr auto ifmt  = S_IFMT;
constexpr auto ifreg = S_IFREG;
constexpr auto ifdir = S_IFDIR;
constexpr auto ifchr = S_IFCHR;
constexpr auto iflnk = S_IFLNK;
constexpr auto ifblk = S_IFBLK;

constexpr auto iwusr = S_IWUSR;
constexpr auto irusr = S_IRUSR;
constexpr auto ixusr = S_IXUSR;

constexpr auto f_ok = F_OK;
constexpr auto r_ok = R_OK;
constexpr auto w_ok = W_OK;
constexpr auto x_ok = X_OK;

using ::stat;
using ::lstat;
using ::mkdir;
#endif

enum class SignalType {
  KILL, TERMINATE, INTERRUPT, STOP, CONTINUE, Count
};

#ifdef PLATFORM_POSIX
inline int to_posix(SignalType st) {
  switch (st) {
  case SignalType::KILL: return SIGKILL;
  case SignalType::TERMINATE: return SIGTERM;
  case SignalType::INTERRUPT: return SIGINT;
  case SignalType::STOP: return SIGSTOP;
  case SignalType::CONTINUE: return SIGCONT;
  default: return -1;
  }
  UNREACHABLE("icl::os::to_posix(icl::os::SignalType)");
}
#endif

#ifdef PLATFORM_WINDOWS
namespace win32 {
int last_error_to_errno();
int stat(const char *path, struct stat *st, bool follow_symlink);

int wlen_from_cstr(const std::string& str);
int wlen_from_cstr(const char *buffer);
int wlen_from_cstr(const char *buffer, int size);

int len_from_wstr(const std::wstring& wstr);
int len_from_wstr(const wchar_t *wbuffer);
int len_from_wstr(const wchar_t *wbuffer, int wsize);

bool wide_to_utf8(const wchar_t* wbuffer, int wsize, char* buffer, int size);
bool utf8_to_wide(const char* buffer, int size, wchar_t* wbuffer, int wsize);
std::wstring to_wstring(const std::string& str);
std::string to_string(const std::wstring& str);

void cmdline_escape_if_needed(std::wstring& wstr);
} // namespace win32
#endif

enum class ProcessState { INVALID, CONSTRUCTED, RUNNING, REAPED, Count };

// processes (Abstracted API, fork + execvp + sys/wait.h)
// TODO: Add stdin/stdout/stderr redirecting
class Process {
public:
#ifdef PLATFORM_POSIX
  template <typename... Args,
    typename = std::enable_if_t<(std::is_convertible_v<Args, const char *> && ...)>>
  Process(const char *file, Args... args) : m_argv(sizeof...(Args) + 2), m_state(ProcessState::CONSTRUCTED)
  {
    m_argv.push_many(file, args...);
  }

  Process(const char *file, int argc, char **argv) : m_argv(argc + 2), m_state(ProcessState::CONSTRUCTED)
  {
    m_argv.push(file);
    for (size_t i = 0; i < argc; i++) {
      m_argv.push(argv[i]);
    }
  }

  Process(const CommandBuilder& cmd) : m_argv(cmd.count() + 1), m_state(ProcessState::CONSTRUCTED)
  {
    for (const auto& item : cmd) {
      m_argv.push(item.c_str());
    }
  }

#else // PLATFORM_WINDOWS
  template <typename... Args,
    typename = std::enable_if_t<(std::is_convertible_v<Args, const wchar_t *> && ...)>>
  Process(const char *file, Args... args) : m_state(ProcessState::CONSTRUCTED)
  {
    std::wstring wfile = win32::to_wstring(file);
    win32::cmdline_escape_if_needed(wfile);
    m_cmdline += wfile;
    m_cmdline += L' ';
    ((win32::cmdline_escape_if_needed(args), m_cmdline += args, m_cmdline += L' '), ...);
  }

  Process(const char *file, int argc, char **argv) : m_state(ProcessState::CONSTRUCTED)
  {
    std::wstring wfile = win32::to_wstring(file);
    win32::cmdline_escape_if_needed(wfile);
    m_cmdline += wfile;
    m_cmdline += L' ';

    for (size_t i = 0; i < argc; i++) {
      std::wstring warg = win32::to_wstring(argv[i]);
      win32::cmdline_escape_if_needed(warg);
      m_cmdline += warg;
      m_cmdline += L' ';
    }
  }

  Process(const CommandBuilder& cmd) : m_state(ProcessState::CONSTRUCTED)
  {
    for (const auto& elem : cmd) {
      std::wstring welem = win32::to_wstring(elem);
      win32::cmdline_escape_if_needed(welem);
      m_cmdline += welem;
      m_cmdline += L' ';
    }
  }
#endif

  ~Process() {
#ifdef PLATFORM_WINDOWS
    if (m_hProcess) CloseHandle(m_hProcess);
#endif
    if (m_state == ProcessState::RUNNING) wait();
    m_state = ProcessState::INVALID;
  }

  ProcessState state() { return m_state; }
  bool spawn();
  bool signal(SignalType signal_type);
  int wait();

private:
#ifdef PLATFORM_WINDOWS
  std::wstring m_cmdline;
  HANDLE m_hProcess = nullptr;
#else
  pid_t m_pid = 0;
  ArrayList<const char *> m_argv;
#endif
  int m_exit_code = 0;
  ProcessState m_state = ProcessState::INVALID;
}; // class Process
} // namespace os

// Those files can be folders (so it's cross-refering)
namespace io {
enum class FilePermission : u8 {
  NONE = 0,
  EXECUTE = 1 << 0,
  WRITE = 1 << 1,
  READ = 1 << 2,
  Count = 4,
};

constexpr FilePermission operator |(FilePermission lhs, FilePermission rhs) {
  return static_cast<FilePermission>(
    static_cast<std::underlying_type_t<FilePermission>>(lhs) |
    static_cast<std::underlying_type_t<FilePermission>>(rhs)
  );
}

constexpr FilePermission& operator |=(FilePermission& lhs, FilePermission rhs) {
  lhs = lhs | rhs;
  return lhs;
}

constexpr FilePermission operator &(FilePermission lhs, FilePermission rhs) {
  return static_cast<FilePermission>(
    static_cast<std::underlying_type_t<FilePermission>>(lhs) &
    static_cast<std::underlying_type_t<FilePermission>>(rhs)
  );
}

constexpr bool operator ==(FilePermission lhs, FilePermission rhs) {
  return static_cast<std::underlying_type_t<FilePermission>>(lhs) ==
         static_cast<std::underlying_type_t<FilePermission>>(rhs);
}

constexpr bool operator !=(FilePermission lhs, FilePermission rhs) {
  return !(lhs == rhs);
}

constexpr bool is_set(FilePermission perms, FilePermission value) {
  return (perms & value) != FilePermission::NONE;
}

// TODO: Add FIFO and socket types
enum class FileType {
  UNKNOWN = 0,
  REGULAR, // Normal text/binary files (-)
  DIRECTORY, // Directories / Folders (d)
  CHARDEV, // Character device (c)
  BLOCKDEV, // Block device (b)
  SYMLINK, // Symbolic links (l)
  Count,
};

struct File {
  FileType type;
  std::string name;
  std::string content;
  size_t size;
  FilePermission permissions;
  os::time_t mtime;
  os::ino_t inode;
  ArrayList<File> files;
};

bool mkdir_if_not_exists(const char* path);

// Conversions
const char* to_string(FileType ft);
FilePermission to_filepermission(os::mode_t mode);
FileType to_filetype(os::mode_t mode);

// File utilities
Result<std::string> read_file_content(const char* file_path, size_t file_size = 0);
Result<File> read_file_metadata(const char* file_path, bool follow_symlink = true);
Result<File> read_entire_file(const char* file_path, bool follow_symlink = true);

// Directory utilities
Result<File> read_entire_directory(const char* dir_path, bool read_content = false);
Result<File> read_directory(const char* dir_path, bool read_content = false);

} // namespace io

#define REBUILD_URSELF(argc, argv, ...) \
  icl::_rebuild_urself((argc), (argv), __FILE__, ##__VA_ARGS__)

template<typename... Args>
bool _rebuild_urself(int argc, char** argv, const char* file_name, Args... args);

} // namespace icl

#if __cplusplus >= 202002L
template <typename T>
struct std::formatter<icl::ArrayList<T>> : std::formatter<std::string> {
  auto format(const icl::ArrayList<T>& arr, std::format_context& ctx) const -> decltype(ctx.out());
};

template <>
struct std::formatter<icl::CommandBuilder> : std::formatter<std::string> {
  auto format(const icl::CommandBuilder& cmd, std::format_context& ctx) const -> decltype(ctx.out());
};
#endif

#ifdef ICL_IMPLEMENTATION

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

namespace icl {

namespace os {
#ifdef PLATFORM_WINDOWS
// dirent.h
DIR *opendir(const char *dirname) {
  (void)dirname;
  TODO("opendir");
}

struct dirent *readdir(DIR *dirp) {
  (void)dirp;
  TODO("readdir");
}

int closedir(DIR *dirp) {
  (void)dirp;
  TODO("closedir");
}

// sys/stat.h
int lstat(const char *path, struct stat *st) { return win32::stat(path, st, false); }
int stat(const char *path, struct stat *st) { return win32::stat(path, st, true); }

// unistd.h
ssize_t readlink(const char *path, char *buf, size_t bufsz) {
  (void)(path); (void)(buf); (void)(bufsz);
  TODO("readlink");
}

int access(const char *path, mode_t mode) {
  (void)(path); (void)(mode);
  TODO("access");
}

int mkdir(const char *path, mode_t mode) {
  (void)path; (void)mode;
  TODO("mkdir");
}

namespace win32 {
int stat(const char *path, struct stat *st, bool follow_symlink) {
  (void)path; (void)st; (void)follow_symlink;
  TODO("stat");
}

int wlen_from_cstr(const std::string& str) { return wlen_from_cstr(str.c_str(), str.size()); }
int wlen_from_cstr(const char *buffer) { return wlen_from_cstr(buffer, -1); }
int wlen_from_cstr(const char *buffer, int size) {
  return MultiByteToWideChar(CP_UTF8, 0, buffer, size, nullptr, 0);
}

int len_from_wstr(const std::wstring& wstr) { return len_from_wstr(wstr.c_str(), wstr.size()); }
int len_from_wstr(const wchar_t *wbuffer) { return len_from_wstr(wbuffer, -1); }
int len_from_wstr(const wchar_t *wbuffer, int wsize) {
  return WideCharToMultiByte(CP_UTF8, 0, wbuffer, wsize, nullptr, 0, nullptr, nullptr);
}

bool wide_to_utf8(const wchar_t* wbuffer, int wsize, char* buffer, int size) {
  int n = WideCharToMultiByte(CP_UTF8, 0, wbuffer, wsize, buffer, size, nullptr, nullptr);
  if (n <= 0 || n > size) { errno = last_error_to_errno(); return false; }
  return true;
}

bool utf8_to_wide(const char* buffer, int size, wchar_t* wbuffer, int wsize) {
  int len = MultiByteToWideChar(CP_UTF8, 0, buffer, size, wbuffer, wsize);
  if (len <= 0) { errno = last_error_to_errno(); return false; }
  return true;
}

std::wstring to_wstring(const std::string& str) {
  int n = wlen_from_cstr(str);
  if (n <= 0) return {};
  std::wstring ws(n, L'\0');
  if (!utf8_to_wide(str.c_str(), static_cast<int>(str.size()), ws.data(), static_cast<int>(ws.size()))) return {};
  return ws;
}

std::string to_string(const std::wstring& wstr) {
  int n = len_from_wstr(wstr);
  if (n <= 0) return {};
  std::string str(n, '\0');
  if (!wide_to_utf8(wstr.c_str(), static_cast<int>(wstr.size()), str.data(), static_cast<int>(str.size()))) return {};
  return str;
}

int last_error_to_errno() {
  switch (GetLastError()) {
  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
    return ENOENT;
  case ERROR_ACCESS_DENIED:
    return EACCES;
  case ERROR_ALREADY_EXISTS:
  case ERROR_FILE_EXISTS:
    return EEXIST;
  case ERROR_INVALID_NAME:
  case ERROR_BAD_PATHNAME:
    return EINVAL;
  case ERROR_TOO_MANY_OPEN_FILES:
    return EMFILE;
  case ERROR_DISK_FULL:
    return ENOSPC;
  case ERROR_NOT_READY:
    return ENODEV;
  case ERROR_DIRECTORY:
    return ENOTDIR;
    case ERROR_CANT_RESOLVE_FILENAME: // symlink loop
    return ELOOP;
  default:
    return EIO;
  }
  UNREACHABLE("icl::os::win32::last_error_to_errno");
}

void cmdline_escape_if_needed(std::wstring& ws) {
  if (ws.empty()) { ws = L"\"\""; return; }

  bool need_escaping = false;
  bool need_quotes = false;
  for (wchar_t ch : ws) {
    if (ch == L'\\', ch == L'"') need_escaping = true;
    if (ch == L'\t', ch == L' ') need_quotes = true;
  }

  if (need_quotes) ws.push_back(L'"');

  size_t backslashes = 0;
  for (wchar_t c : ws) {
    if (c == L'\\') {
      ++backslashes;
    } else if (c == L'"') {
      ws.append(backslashes * 2 + 1, L'\\');
      ws.push_back(L'"');
      backslashes = 0;
    } else {
      ws.append(backslashes, L'\\');
      backslashes = 0;
      ws.push_back(c);
    }
  }

  if (need_escaping) ws.append(backslashes * 2, L'\\');
  else ws.append(backslashes, L'\\');

  if (need_quotes) ws.push_back(L'"');
}
} // namespace win32
#endif // PLATFORM_WINDOWS

bool Process::spawn() {
  if (m_state != ProcessState::CONSTRUCTED &&
      m_state != ProcessState::REAPED) {
    eprintln("ERROR: Proces::spawn() called in invalid state");
    errno = EINVAL;

    // user-friendly error messages
    eprint("  Hint:");
    switch (m_state) {
    case ProcessState::RUNNING: {
      eprintln("process is already running, reape or signal it "
               "by calling wait() or signal() then spawn() another one");
    } break;
    case ProcessState::INVALID: {
      eprintln("process class is in weird state maybe constructor failed?");
    } break;
    default: UNREACHABLE("ProcessState switch-case in Process::spawn()");
    }

    return false;
  }

  #ifdef PLATFORM_POSIX
  pid_t pid = ::fork();
  if (pid == 0) { // child
    m_argv.push(nullptr);
    ::execvp(m_argv[0], const_cast<char *const*>(m_argv.items()));
    eprintln("ERROR: execvp failed: {}", strerror(errno));
    std::exit(1);
  } else if (pid > 0) { // parent
    m_pid = pid;
  } else {
    eprintln("ERROR: fork failed: {}", strerror(errno));
    return false;
  }

#else // PLATFORM_WINDOWS
  STARTUPINFOW siStartupInfo{};
  siStartupInfo.cb = sizeof(siStartupInfo);
  PROCESS_INFORMATION siProcessInformation{};

  BOOL ret = CreateProcessW(nullptr, m_cmdline.data(),
                            nullptr, nullptr, false,
                            0, nullptr, nullptr,
                            &siStartupInfo, &siProcessInformation);
  if (!ret) { errno = win32::last_error_to_errno(); return false; }

  CloseHandle(siProcessInformation.hThread);
  m_hProcess = siProcessInformation.hProcess;
#endif // PLATFORM_POSIX

  m_state = ProcessState::RUNNING;
  return true;
}

bool Process::signal(SignalType signal_type) {
  if (m_state != ProcessState::RUNNING) {
    eprintln("ERROR: Process::signal() called in invalid state");
    errno = EINVAL;

    // user-friendly error messages
    eprint("  Hint:  ");
    switch (m_state) {
    case ProcessState::CONSTRUCTED: {
      eprintln("process class is already constructed, call spawn() between ctor and signal()");
    } break;
    case ProcessState::REAPED: {
      eprintln("process is already reaped, spawn() another one then signal() it");
    } break;
    case ProcessState::INVALID: {
      eprintln("process is in weird state, maybe constructor failed");
    } break;
    default: UNREACHABLE("ProcessState switch-case in Process::signal()");
    }
    return -1;
  }

#ifdef PLATFORM_POSIX
  assert(m_pid > 0 && "Process::signal(): RUNNING state but pid <= 0, invariant broken");
  if (::kill(m_pid, to_posix(signal_type)) != 0) return false;
#else // PLATFORM_WINDOWS
  assert(m_hProcess && "Process::signal(): RUNNING state but m_hProcess = nullptr, invariant broken");
  BOOL ret;
  if (signal_type == SignalType::KILL) ret = TerminateProcess(m_hProcess, 1);
  else ret = GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0); // SIGINT
  if (!ret) return false;
#endif // PLATFORM_POSIX
  m_state = ProcessState::REAPED;
  return true;
}

int Process::wait() {
  if (m_state != ProcessState::RUNNING) {
    eprintln("ERROR: Process::wait() called in invalid state");
    errno = EINVAL;

    // user-friendly error messages
    eprint("  Hint:  ");
    switch (m_state) {
    case ProcessState::CONSTRUCTED: {
      eprintln("process class is already constructed, call spawn() between ctor and wait()");
    } break;
    case ProcessState::REAPED: {
      eprintln("process is already reaped, spawn() another one then wait() it");
    } break;
    case ProcessState::INVALID: {
      eprintln("process is in weird state, maybe constructor failed");
    } break;
    default: UNREACHABLE("ProcessState switch-case in Process::wait()");
    }
    return -1;
  }

#ifdef PLATFORM_POSIX
  assert(m_pid && "Process::wait(): RUNNING state but pid <= 0, invariant broken");
  int status;
  if (::waitpid(m_pid, &status, 0) != m_pid) return -1;
  if (WIFEXITED(status)) m_exit_code = WEXITSTATUS(status);
  if (WIFSIGNALED(status)) m_exit_code = 128 + WTERMSIG(status);
#else // PLATFORM_POSIX
  assert(m_hProcess && "Process::wait(): RUNNING state but m_hProcess = nullptr, invariant broken");
  DWORD status;
  if (WaitForSingleObject(m_hProcess, INFINITE) != WAIT_OBJECT_0) {
    errno = win32::last_error_to_errno();
    return -1;
  }
  if (!GetExitCodeProcess(m_hProcess, &status)) {
    errno = win32::last_error_to_errno();
    return -1;
  }
  m_exit_code = static_cast<int>(status);
#endif
  m_state = ProcessState::REAPED;
  return m_exit_code;
}
} // namespace os

// Dynamic Arrays implementation start
template <typename T>
bool ArrayList<T>::reserve(size_t extra) {
  size_t needed = m_count + extra;
  if (m_capacity >= needed) return true; // enough capacity, no need to realloc

  // calculate new capacity
  size_t new_cap = m_capacity;
  while (new_cap < needed)
    new_cap += (new_cap >> 1) + 1;

  // allocate a new block and call move ctors (if move ctor of T throws, fallbacks to copy)
  T* new_items = static_cast<T*>(::operator new(new_cap*sizeof(T)));
  size_t constructed = 0;
  // TODO: Move this logic to construct_move_range() function if this logic repeats in somewhere else
#if __cpp_exceptions
  try {
#endif
    for (size_t i = 0; i < m_count; i++) {
      new (&new_items[i]) T(std::move_if_noexcept(m_items[i]));
      constructed++;
    }
#if __cpp_exceptions
  } catch(...) {
    for (size_t i = 0; i < constructed; i++) new_items[i].~T();
    ::operator delete(new_items);
    throw;
  }
#endif

  // destruct all old elements and deallocate items ptr
  for (size_t i = 0; i < m_count; ++i) m_items[i].~T();
  ::operator delete(m_items);

  // update fields
  m_items = new_items;
  m_capacity = new_cap;
  return true;
}

template <typename T>
bool ArrayList<T>::shift_left(size_t start, size_t end, size_t amount) {
  if (start > end) return false;
  if (start < amount) return false;
  if (end > m_count) return false;
  T* first = m_items + start;
  T* last  = m_items + end;
  T* d_first = m_items + start - amount;
  while (first != last) {
    *(d_first++) = std::move(*(first++));
  }
  return true;
}

template <typename T>
bool ArrayList<T>::shift_right(size_t start, size_t end, size_t amount) {
  if (start > end) return false;
  if (end > m_count) return false;
  if (end + amount > m_capacity) return false;
  T* first = m_items + start;
  T* last  = m_items + end;
  T* d_last = m_items + end + amount;
  T* live_boundary = m_items + m_count;
  // TODO: Make this exception-safe and add rollback mechanism
  while (first != last) {
    --last; --d_last;
    if (d_last >= live_boundary) {
      new (d_last) T(std::move_if_noexcept(*last));
    } else {
      *d_last = std::move(*last);
    }
  }
  return true;
}

template <typename T>
template <typename U>
bool ArrayList<T>::add_impl(U&& item, size_t idx) {
  if (idx > m_count) return false;
  if (!reserve(1)) return false;
  bool ok = shift_right(idx, m_count);
  assert(ok && "Shifting right in ArrayList<T>::add failed, this should never fail");
  (void)ok;
  if (idx == m_count) new (m_items + idx) T(std::forward<U>(item));
  else m_items[idx] = std::forward<U>(item); // assignment bcs hole is constructed bcs of current shifting algorithm
  m_count++;
  return true;
}

template <typename T>
template<typename... Args>
bool ArrayList<T>::push_many(Args&&... args) {
  if (!reserve(sizeof...(args))) return false;
  bool ok = true;
  auto try_push = [&](auto&& item){
    if (ok) ok = push(std::forward<decltype(item)>(item));
  };
  (try_push(std::forward<Args>(args)), ...);
  return ok;
}

template <typename T>
T ArrayList<T>::remove(size_t idx) {
  assert(idx >= m_count && "Boundary check failed in remove");
  // TODO: Make this exception-safe
  T out = std::move_if_noexcept(m_items[idx]);
  bool ok = shift_left(idx + 1, m_count);
  assert(ok && "Shifting left in ArrayList<T>::remove failed, this should never fail");
  (void)ok;
  m_items[m_count - 1].~T();
  m_count--;
  return out;
}

template <typename T>
T ArrayList<T>::remove_unord(size_t idx) {
  assert(idx >= m_count && "Boundary check failed in remove_unord");
  T out = std::move_if_noexcept(m_items[idx]);
  if (idx != m_count - 1) m_items[idx] = std::move(m_items[m_count - 1]);
  m_items[m_count - 1].~T();
  m_count--;
  return out;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const ArrayList<T>& arr) {
  os << "[";
  for (size_t i = 0; i < arr.count(); i++) {
    os << arr[i];
    if (i != arr.count() - 1) os << ", ";
  }
  os << "]";
  return os;
}

template <typename T>
template <typename It>
void ArrayList<T>::construct_range(It first, It last) {
#if __cpp_exceptions
  try {
#endif
    for (; first != last; first++) {
      new (m_items + m_count) T(*first);
      m_count++;
    }
#if __cpp_exceptions
  } catch(...) {
    for (size_t i = 0; i < m_count; ++i) m_items[i].~T();
    ::operator delete(m_items);
    throw;
  }
#endif
}
// Dynamic Arrays implementation end

bool CommandBuilder::run(CmdRunOptions opts) {
  if (opts.is_log) println("INFO: Running: {}", *this);

  os::Process proc(*this);
  if (proc.state() != os::ProcessState::CONSTRUCTED) {
    eprintln("ERROR: Cannot create process: {}", strerror(errno));
    return false;
  }
  if (!proc.spawn()) return false;

  int exit_code = proc.wait();
  if (exit_code != 0) {
    eprintln("ERROR: Command failed with code {}: {}", exit_code, strerror(errno));
    return false;
  }

  if (opts.is_reset) set_count(0);
  return true;
}

template<typename... Args>
bool _rebuild_urself(int argc, char** argv, const char* file_name, Args... args) {
  const char* bin_name = argv[0];
  char old_bin[1024];
  snprintf(old_bin, sizeof old_bin, "%s.old", bin_name);

  bool needs_rebuild = false;
  if (os::access(old_bin, os::f_ok) != 0) {
    needs_rebuild = true;
  } else {
    struct os::stat st_file_name, st_bin_name;
    if (os::stat(file_name, &st_file_name) != 0) {
      eprintln("ERROR: cannot stat '{}': {}", file_name, strerror(errno));
      return false;
    }

    if (os::stat(bin_name, &st_bin_name) != 0) {
      eprintln("ERROR: cannot stat '{}': {}", bin_name, strerror(errno));
      return false;
    }

    needs_rebuild = (st_file_name.st_mtime - st_bin_name.st_mtime) >= 0;
  }

  if (!needs_rebuild) return true;
  println("INFO: Change detected in build script, rebuilding itself.");

  // Rename the binary to old one
  println("INFO: Renaming: '{}' -> '{}'", bin_name, old_bin);
  if (std::rename(bin_name, old_bin) != 0) {
    eprintln("ERROR: cannot rename '{}': {}", bin_name, strerror(errno));
    return false;
  }

  // Construct and run rebuild command
  std::pair<const char*, Compiler> comp = get_native_compiler();
  if (comp.second == Compiler::UNKNOWN) {
    eprintln("ERROR: Unknown compiler: '{}'", comp.first);
    return false;
  }
  CommandBuilder cmd;
  cmd.push(comp.first);
  cmd.push(file_name);
  cmd.push_many(std::forward<Args>(args)...); // custom flags if you need
  cmd.push_many("-o", bin_name);

  // Run rebuild command
  if (!cmd.run()) {
    eprintln("ERROR: cannot rebuild itself.");
    return false;
  }

  // Run the new binary and exit this old one
#ifdef PLATFORM_WINDOWS
  os::Process proc(bin_name, argc, argv);
#else
  os::Process proc(bin_name, argc, argv);
#endif
  if (proc.state() != os::ProcessState::CONSTRUCTED) {
    eprintln("ERROR: cannot run new binary: {}", strerror(errno));
    return false;
  }
  if (!proc.spawn()) return false;
  std::exit(proc.wait());
}

const char* to_string(Compiler e) {
  switch (e) {
  case Compiler::UNKNOWN: return "<unknown>";
  case Compiler::GCC: return "gcc";
  case Compiler::CLANG: return "clang";
  case Compiler::INTEL_LLVM: return "icpx";
  case Compiler::INTEL_CLASSIC: return "icpc";
  default: return "<invalid>";
  }
  UNREACHABLE("const char* to_string(icl::Compiler e)");
}

Compiler compiler_from_cstr(const char* str) {
  if (strcmp(str, "g++") == 0) return Compiler::GCC;
  if (strcmp(str, "clang++") == 0) return Compiler::CLANG;
  if (strcmp(str, "icpx") == 0) return Compiler::INTEL_LLVM;
  if (strcmp(str, "icpc") == 0) return Compiler::INTEL_CLASSIC;
  return Compiler::UNKNOWN;
}

// Get bootstrapped C++ compiler
std::pair<const char*, Compiler> get_native_compiler() {
#if NATIVE_COMPILER != NULL
  return {NATIVE_COMPILER, compiler_from_cstr(NATIVE_COMPILER)};
#else
  #if defined(__INTEL_LLVM_COMPILER)
    return {"icpx", Compiler::INTEL_LLVM};
  #elif defined(__INTEL_COMPILER)
    return {"icpc", Compiler::INTEL_CLASSIC};
  #elif defined(__clang__)
    return {"clang++", Compiler::CLANG};
  #elif defined(__GNUC__)
    return {"g++", Compiler::GCC};
  #else
    #error "Unknown compiler, define NATIVE_COMPILER manually"
  #endif
  UNREACHABLE("std::pair<const char*, icl::Compiler> get_native_compiler()");
#endif
}

// io impl start
namespace io {
bool mkdir_if_not_exists(const char* path) {
  for (const char* p = path + 1; *p != '\0'; p++) {
    if (*p == '/') {
      size_t i = (size_t)(p - path);
      if (i >= PATH_MAX) return false;
      char buf[PATH_MAX] = {0};
      std::memcpy(buf, path, i);
      if (os::mkdir(buf, 0775) != 0 && errno != EEXIST) return false;
    }
  }
  return true;
}

// no recursion here
Result<File> read_directory(const char* dir_path, bool read_content) {
  // open directory
  os::DIR *dir_ptr = os::opendir(dir_path);
  if (!dir_ptr) return Err({errno, "opendir failed", __FILE__, __LINE__});
  defer { os::closedir(dir_ptr); };

  // stat of directory itself
  auto ret = read_file_metadata(dir_path);
  if (ret.is_err()) return Err(ret.error());
  File dir = std::move(ret).value();

  // iterate directory (no recursive look)
  struct os::dirent *entry;
  while ((entry = os::readdir(dir_ptr)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 ||
        strcmp(entry->d_name, "..") == 0) continue;

    // Construct full path
    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%.*s/%s",
      (int)dir.name.size(), dir.name.c_str(), entry->d_name);

    // Read and push the file
    Result<File> ret;
    if (read_content) ret = read_entire_file(full_path, false);
    else ret = read_file_metadata(full_path, false);
    if (ret.is_err()) return Err(ret.error());
    dir.files.push(std::move(ret).value());
  }

  return Ok(dir);
}

Result<File> read_entire_directory(const char* dir_path, bool read_content) {
  static_assert(std::is_move_assignable_v<File>, "File is not move assignable!");
  auto ret = read_directory(dir_path, read_content);
  if (ret.is_err()) return std::move(ret);
  File top = std::move(ret).value();
  for (auto& f : top.files) {
    if (f.type != FileType::DIRECTORY) continue;
    auto ret = read_entire_directory(f.name.c_str(), read_content);
    if (ret.is_err()) return std::move(ret);
    f = std::move(ret).value();
  }
  return Ok(std::move(top));
}

Result<std::string> read_file_content(const char* file_path, size_t file_size) {
  std::FILE *f = std::fopen(file_path, "rb");
  if (!f) return Err({errno, "fopen failed", __FILE__, __LINE__});
  defer { std::fclose(f); };

  if (file_size == 0) {
    if (std::fseek(f, 0, SEEK_END) != 0) return Err({errno, "fseek failed", __FILE__, __LINE__});
    long size = std::ftell(f);
    if (size == -1L) return Err({errno, "ftell failed", __FILE__, __LINE__});
    if (std::fseek(f, 0, SEEK_SET) != 0) return Err({errno, "fseek failed", __FILE__, __LINE__});
    file_size = (size_t)size;
  }

  std::string result;
  result.resize(file_size);
  size_t n = std::fread(result.data(), 1, file_size, f);
  if (n != file_size) {
    if (feof(f) != 0 || ferror(f) != 0) return Err({errno, "fread failed", __FILE__, __LINE__});
  }
  result.resize(n);
  return Ok(std::move(result));
}

Result<File> read_file_metadata(const char* file_path, bool follow_symlink) {
  File result;
  struct os::stat st;
  if (follow_symlink) {
    if (os::stat(file_path, &st) != 0) return Err({errno, "stat failed", __FILE__, __LINE__});
  } else {
    if (os::lstat(file_path, &st) != 0) return Err({errno, "lstat failed", __FILE__, __LINE__});
  }

  result.type = to_filetype((os::mode_t)st.st_mode);
  result.name = std::string(file_path);
  result.mtime = st.st_mtime;
  result.inode = st.st_ino;
  result.permissions = to_filepermission(st.st_mode);
  result.size = st.st_size;

  if (result.type == FileType::SYMLINK) {
    char buf[PATH_MAX];
    ssize_t n = os::readlink(file_path, buf, sizeof(buf));
    if (n < 0) { return Err({errno, "readlink failed"}); }
    result.content = std::string(buf, n);
  }
  return Ok(std::move(result));
}

Result<File> read_entire_file(const char* file_path, bool follow_symlink) {
  auto ret = read_file_metadata(file_path, follow_symlink);
  if (ret.is_err()) return std::move(ret);
  File result = std::move(ret).value();

  if (result.type == FileType::REGULAR) {
    auto ret = read_file_content(file_path, result.size);
    if (ret.is_err()) return Err(ret.error());
    result.content = std::move(ret).value();
  }
  return Ok(std::move(result));
}

FileType to_filetype(os::mode_t mode) {
  switch (mode & os::ifmt) {
  case os::ifreg: return FileType::REGULAR;
  case os::ifdir: return FileType::DIRECTORY;
  case os::ifchr: return FileType::CHARDEV;
  case os::iflnk: return FileType::SYMLINK;
  case os::ifblk: return FileType::BLOCKDEV;
  default:        return FileType::UNKNOWN;
  }
  UNREACHABLE("to_filetype");
}

FilePermission to_filepermission(os::mode_t mode){
  FilePermission perms = FilePermission::NONE;
  if (mode & os::irusr) perms |= FilePermission::READ;
  if (mode & os::iwusr) perms |= FilePermission::WRITE;
  if (mode & os::ixusr) perms |= FilePermission::EXECUTE;
  return perms;
}

const char* to_string(FileType ft) {
  switch(ft) {
  case FileType::REGULAR: return "regular";
  case FileType::DIRECTORY: return "directory";
  case FileType::CHARDEV: return "character_device";
  case FileType::BLOCKDEV: return "block_device";
  case FileType::SYMLINK: return "symlink";
  default: return "<unknown>";
  }
  UNREACHABLE("const char* to_string(icl::io::FileType e)");
}

} // namespace io
// io impl end

} // namespace icl

#if __cplusplus >= 202002L || (defined(__cpp_lib_format) && __cpp_lib_format >= 201907L)
template <typename T>
auto std::formatter<icl::ArrayList<T>>::format(
  const icl::ArrayList<T>& arr,
  std::format_context& ctx
) const -> decltype(ctx.out())
{
  std::string result = "[";
  for (size_t i = 0; i < arr.count(); i++) {
    result += std::format("{}", arr[i]);
    if (i != arr.count() - 1) result += ", ";
  }
  result += "]";
  return std::formatter<std::string>::format(result, ctx);
}

auto std::formatter<icl::CommandBuilder>::format(
  const icl::CommandBuilder& cmd,
  std::format_context& ctx
) const -> decltype(ctx.out())
{
  std::string result;
  for (size_t i = 0; i < cmd.count(); i++) {
    const std::string& elem = cmd[i];
#ifdef PLATFORM_POSIX
    result += elem;
#else
    std::wstring welem = win32::to_wstring(elem);
    win32::cmdline_escape_if_needed(welem);
    result += win32::to_string(welem);
#endif
    if (i != cmd.count() - 1) result += " ";
  }
  return std::formatter<std::string>::format(result, ctx);
}
#endif

#endif // ICL_IMPLEMENTATION

#endif // ICL_HPP
