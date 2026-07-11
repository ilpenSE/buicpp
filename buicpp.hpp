#ifndef BUICPP_HPP
#define BUICPP_HPP

/*
  This is a C++ library for building C++ in C++.
  You can steal this header and use it like a stb-style single-header library:
  ```cpp
    #define BUICPP_IMPLEMENTATION
    #include "buicpp.hpp"
    buicpp::CommandBuilder cmd;
    cmd.push("clang++");
    ...
  ```

  It has additional STL things (for example dynamic arrays) you can use them
  But they're not as production-ready as STL.
*/

#include <algorithm>
#include <utility>
#include <ostream>
#include <cassert>
#include <cstddef>
#include <format>
#include <version>

#if defined(__unix__) || defined(__unix)
  #define PLATFORM_UNIX 1
  #define PLATFORM_POSIX 1
#elif defined(__APPLE__) || defined(__MACH__)
  #define PLATFORM_APPLE 1
  #define PLATFORM_POSIX 1
#elif defined(_WIN32)
  #define PLATFORM_WINDOWS 1
#endif

#include <time.h>
#ifdef PLATFORM_POSIX
  #include <unistd.h>
  #include <sys/wait.h>
  #include <sys/stat.h>
  namespace buicpp {
    inline int execvp(const char* file_name, const char* const* argv) {
      return ::execvp(file_name, const_cast<char* const*>(argv));
    }
  }
#else // PLATFORM_WINDOWS
  #include <direct.h>
  #include <io.h>
  #include <process.h>
  namespace buicpp {
    inline int execvp(const char* file_name, const char* const* argv) {
      ::_execvp(file_name, argv);
    }
  }
#endif

/*
  You can define NATIVE_COMPILER in command line while bootstrapping
*/
#ifndef NATIVE_COMPILER
  #define NATIVE_COMPILER NULL /* Detect compiler in runtime */
#endif

namespace buicpp {

// Either start
struct LTag {};
struct RTag {};

template <typename L, typename R>
class Either {
public:
  Either(const L& value, LTag) : m_is_left(true), m_left(value) {}
  Either(L&& value, LTag) : m_is_left(true), m_left(std::move(value)) {}
  Either(const R& value, RTag) : m_is_left(false), m_right(value) {}
  Either(R&& value, RTag) : m_is_left(false), m_right(std::move(value)) {}

  ~Either() { destruct_active(); }

  // copy ctor
  Either(const Either& other) : m_is_left(other.m_is_left)
  {
    if (other.m_is_left) new (&m_left) L(other.m_left);
    else new (&m_right) R(other.m_right);
  }

  // copy assignment
  Either& operator =(const Either& other)
    noexcept(std::is_nothrow_copy_constructible_v<L> &&
             std::is_nothrow_copy_constructible_v<R>)
  {
    if (this == &other) return *this; // prevent self-assignment
    // same sides (both are left or right)
    if (m_is_left == other.m_is_left) {
      if (m_is_left) m_left = other.m_left;
      else m_right = other.m_right;
      return *this;
    }

    // different sides (one is left other is right or vice versa)
    if (other.m_is_left) {
      L tmp(other.m_left); // it can throw exception here but this stays same
      destruct_active();
      new (&m_left) L(std::move(tmp));
    } else {
      R tmp(other.m_right); // it can throw exception here but this stays same
      destruct_active();
      new (&m_right) R(std::move(tmp));
    }

    m_is_left = !m_is_left;
    return *this;
  }

  // move ctor
  Either(Either&& other) noexcept : m_is_left(other.m_is_left)
  {
    if (other.m_is_left) new (&m_left) L(std::move(other.m_left));
    else new (&m_right) R(std::move(other.m_right));
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
  bool operator ==(const Either& other) const {
    if (m_is_left != other.m_is_left) return false;
    if (m_is_left) return m_left == other.m_left;
    else return m_right == other.m_right;
  }
  bool operator !=(const Either& other) const {
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
};

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
#ifndef ARRAYLIST_DEFAULT_CAPACITY
#define ARRAYLIST_DEFAULT_CAPACITY 64
#endif

// TODO: Do not allocate items with new[]
// use placement new and calling ctors manually instead
// to avoid calling default constructors for nothing.
template <typename T>
class ArrayList {
public:
  // Constructor / Destructor
  ArrayList(size_t init_capacity = ARRAYLIST_DEFAULT_CAPACITY) :
    m_items(new T[init_capacity]), m_count(0), m_capacity(init_capacity) {}
  ArrayList(std::initializer_list<T> list) :
    m_items(new T[list.size()]), m_count(0), m_capacity(list.size())
  {
    for (const T& val : list)
      m_items[m_count++] = val;
  }
  ~ArrayList() { delete[] m_items; }

  // Copy constructor
  ArrayList(const ArrayList& other) :
    m_items(new T[other.m_capacity]), m_count(other.m_count), m_capacity(other.m_capacity)
  {
    std::copy(other.m_items, other.m_items + other.m_count, m_items);
  }

  // Copy operator
  ArrayList& operator =(const ArrayList& other) {
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

  // Reserve extra "extra" amount of capacity
  bool reserve(size_t extra);

  // Add element into idx (shifts array if you're not pushing to back)
  bool add(const T& item, size_t idx) { return add_impl(item, idx); }
  bool add(T&& item, size_t idx) { return add_impl(std::move(item), idx); }
  bool push(const T& item) { return add_impl(item, m_count); }
  bool push(T&& item) { return add_impl(std::move(item), m_count); }
  template<typename... Args>
  bool push_many(Args&&... args);

  // Remove element from idx (shifts array if you're not popping from back)
  bool pop(T* out = nullptr) { return remove(m_count - 1, out); }
  bool remove(size_t idx, T* out = nullptr);
  bool remove_unord(size_t idx, T* out = nullptr);

  // shift "amount" elements in range [start, end) to left or right
  bool shift_right(size_t start, size_t end, size_t amount = 1);
  bool shift_left(size_t start, size_t end, size_t amount = 1);

  // Access an element (with boundary check)
  T* at(size_t idx) {
    if (idx >= m_count) return nullptr;
    return &m_items[idx];
  }

  // [begin, end) iterators
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

  template <typename U>
  bool add_impl(U&& item, size_t idx);
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const ArrayList<T>& arr);
// Dynamic Arrays end

// Buic start
enum class Compiler {
  UNKNOWN = 0, GCC, CLANG, MSVC, INTEL_LLVM, INTEL_CLASSIC, Count
};

const char* to_string(buicpp::Compiler e);
buicpp::Compiler compiler_from_cstr(const char* str);
std::pair<const char*, buicpp::Compiler> get_native_compiler();

struct CmdRunOptions {
  bool is_log = true;
  bool is_reset = true;
};

class CommandBuilder : public ArrayList<std::string> {
public:
  using ArrayList<std::string>::ArrayList;

  // Return null-terminated const char* array (case for execvp/execve)
  ArrayList<const char*> to_argv() const {
    ArrayList<const char*> argv;
    argv.reserve(count() + 1);
    for (const auto& s : *this)
      argv.push(s.c_str());
    argv.push(nullptr);
    return argv;
  }

  bool run(CmdRunOptions opts = {});
};

// TODO: Add recursive directory walker (returns array of files)
// Those files can be folders (so it's cross-refering)
namespace io {
bool mkdir_if_not_exists(const char* path);
#ifdef PLATFORM_POSIX
#define PATH_SEP '/'
using stat_t = struct stat;
inline bool mkdir(const char* path) { return ::mkdir(path, 0775) == 0; }
inline bool stat(const char* file_path, struct stat *st) { return ::stat(file_path, st) == 0; }
inline bool access(const char* file_path, int mode) { return ::access(file_path, mode) == 0; }

#else // PLATFORM_WINDOWS
#define PATH_SEP '\\'
inline bool mkdir(const char* path) { return ::_mkdir(path) == 0; }
inline bool stat(const char* file_path, struct stat *st) { return ::_stat(file_path, st) == 0; }
inline bool access(const char* file_path, int mode) { return ::_access(file_path, mode) == 0; }
using stat = struct ::_stat;
#endif
}

time_t compare_mtimes(const char* f1, const char* f2);

#define REBUILD_URSELF(argc, argv, ...) \
  buicpp::_buic_rebuild_urself((argc), (argv), __FILE__, ##__VA_ARGS__)

template<typename... Args>
bool _buic_rebuild_urself(int argc, char** argv, const char* file_name, Args... args);

// Buic end

}

#if __cplusplus >= 202002L
template <typename T>
struct std::formatter<buicpp::ArrayList<T>> : std::formatter<std::string> {
  auto format(const buicpp::ArrayList<T>& arr, std::format_context& ctx) const -> decltype(ctx.out());
};
#endif

#ifdef BUICPP_IMPLEMENTATION

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#ifdef PLATFORM_POSIX
  #include <limits.h>
  #ifndef PATH_MAX
    #define PATH_MAX 4096
  #endif
#else
  #define PATH_MAX __MAX_PATH
#endif

namespace buicpp {

// Dynamic Arrays implementation start
template <typename T>
bool ArrayList<T>::reserve(size_t extra) {
  size_t needed = m_count + extra;
  if (m_capacity >= needed) return true; // enough capacity, no need to realloc

  // calculate new capacity
  size_t new_cap = m_capacity;
  while (new_cap < needed)
    new_cap += (new_cap >> 1) + 1;

  T* new_items = new T[new_cap];
  for (size_t i = 0; i < m_count; i++)
    new_items[i] = std::move(m_items[i]);

  delete[] m_items;
  m_items = new_items;
  m_capacity = new_cap;
  return true;
}

template <typename T>
bool ArrayList<T>::shift_left(size_t start, size_t end, size_t amount) {
  if (start > end) return false;
  if (start < amount) return false;
  T* first = m_items + start;
  T* last  = m_items + end;
  T* d_first = m_items + start - amount;
  while (first != last)
    *(d_first++) = std::move(*(first++));
  return true;
}

template <typename T>
bool ArrayList<T>::shift_right(size_t start, size_t end, size_t amount) {
  if (start > end) return false;
  if (end + amount > m_capacity) return false;
  T* first = m_items + start;
  T* last  = m_items + end;
  T* d_last = m_items + end + amount;
  while (first != last)
    *(--d_last) = std::move(*(--last));
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
  m_items[idx] = std::forward<U>(item);
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
bool ArrayList<T>::remove(size_t idx, T* out) {
  if (idx >= m_count) return false;
  if (out != nullptr) *out = std::move(m_items[idx]);
  bool ok = shift_left(idx + 1, m_count);
  assert(ok && "Shifting left in ArrayList<T>::remove failed, this should never fail");
  (void)ok;
  m_count--;
  return true;
}

template <typename T>
bool ArrayList<T>::remove_unord(size_t idx, T* out) {
  if (idx >= m_count) return false;
  if (out != nullptr) *out = std::move(m_items[idx]);
  if (idx != m_count - 1) m_items[idx] = std::move(m_items[m_count - 1]);
  m_count--;
  return true;
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
// Dynamic Arrays implementation end

// Buic impl start
bool CommandBuilder::run(CmdRunOptions opts) {
  if (opts.is_log) {
    printf("[BUIC/INFO] Running: ");
    for (size_t i = 0; i < count(); i++) {
      printf("%s", (*this)[i].c_str());
      if (i != count() - 1) printf(" ");
    }
    printf("\n");
  }

#ifdef PLATFORM_POSIX
  pid_t pid = ::fork();
  if (pid == 0) {
    // child
    auto arr = to_argv();
    buicpp::execvp(arr[0], arr.items());
    fprintf(stderr, "[BUIC/ERROR] ");
    ::perror("execvp");
    exit(1);
  } else if (pid > 0) {
    // parent
    int wstatus = 0;
    ::waitpid(pid, &wstatus, 0);
    if (WIFEXITED(wstatus)) {
      int exit_code = WEXITSTATUS(wstatus);
      if (exit_code != 0) {
        fprintf(stderr, "[BUIC/ERROR] Command failed with code %d\n", exit_code);
        return false;
      }
    }
  } else {
    fprintf(stderr, "[BUIC/ERROR] ");
    ::perror("fork");
    return false;
  }

#else // WINDOWS
  auto arr = to_argv();
  int ret = ::_spawnvp(_P_WAIT, arr[0], arr.items());
  if (ret == -1) {
    perror("_spawnvp");
    return false;
  }
  if (ret != 0) {
    fprintf(stderr, "[BUIC/ERROR] Command failed with code %lld\n",
    static_cast<long long>(ret));
    return false;
  }
#endif

  if (opts.is_reset) set_count(0);
  return true;
}

template<typename... Args>
bool _buic_rebuild_urself(int argc, char** argv, const char* file_name, Args... args) {
  const char* bin_name = argv[0];
  char old_bin[1024];
  snprintf(old_bin, sizeof old_bin, "%s.old", bin_name);

  bool needs_rebuild = false;
  if (!buicpp::io::access(old_bin, F_OK)) {
    needs_rebuild = true;
  } else {
    needs_rebuild = buicpp::compare_mtimes(file_name, bin_name) >= 0;
  }

  if (!needs_rebuild) return true;
  printf("INFO: Change detected in build script, rebuilding itself.\n");

  // Rename the binary to old one
  printf("INFO: Renaming: '%s' -> '%s'\n", bin_name, old_bin);
  if (rename(bin_name, old_bin) != 0) {
    fprintf(stderr, "ERROR: cannot rename '%s': %s\n", bin_name, strerror(errno));
    return false;
  }

  // Construct and run rebuild command
  std::pair<const char*, buicpp::Compiler> comp = buicpp::get_native_compiler();
  if (comp.second == buicpp::Compiler::UNKNOWN) {
    fprintf(stderr, "ERROR: Unknown compiler: '%s'\n", comp.first);
    return false;
  }
  CommandBuilder cmd;
  cmd.push(comp.first);
  cmd.push(file_name);
  cmd.push_many(std::forward<Args>(args)...); // custom flags if you need

  if (comp.second == buicpp::Compiler::MSVC) {
    std::string out = "/Fe:";
    out += bin_name;
    cmd.push(std::move(out));
  } else cmd.push_many("-o", bin_name);

  // Run rebuild command
  if (!cmd.run()) {
    fprintf(stderr, "ERROR: cannot rebuild itself.\n");
    return false;
  }

  // Run the new binary and exit this old one
  buicpp::execvp(bin_name, argv);
  fprintf(stderr, "ERROR: cannot run new binary: %s\n", strerror(errno));
  return false;
}

time_t compare_mtimes(const char* f1, const char* f2) {
  struct stat st_f1, st_f2;
  if (!buicpp::io::stat(f1, &st_f1)) {
    fprintf(stderr, "ERROR: cannot stat '%s': %s\n", f1, strerror(errno));
    return false;
  }

  if (!buicpp::io::stat(f2, &st_f2)) {
    fprintf(stderr, "ERROR: cannot stat '%s': %s\n", f2, strerror(errno));
    return false;
  }
  return (time_t)(st_f1.st_mtime - st_f2.st_mtime);
}

const char* to_string(buicpp::Compiler e) {
  switch (e) {
  case buicpp::Compiler::UNKNOWN: return "<unknown>";
  case buicpp::Compiler::GCC: return "gcc";
  case buicpp::Compiler::CLANG: return "clang";
  case buicpp::Compiler::MSVC: return "cl";
  case buicpp::Compiler::INTEL_LLVM: return "icpx";
  case buicpp::Compiler::INTEL_CLASSIC: return "icpc";
  default: return "<invalid>";
  }
  assert(false && "unreachable: const char* to_string(buicpp::Compiler e)");
}

buicpp::Compiler compiler_from_cstr(const char* str) {
  if (strcmp(str, "cl") == 0) return buicpp::Compiler::MSVC;
  if (strcmp(str, "g++") == 0) return buicpp::Compiler::GCC;
  if (strcmp(str, "clang++") == 0) return buicpp::Compiler::CLANG;
  if (strcmp(str, "icpx") == 0) return buicpp::Compiler::INTEL_LLVM;
  if (strcmp(str, "icpc") == 0) return buicpp::Compiler::INTEL_CLASSIC;
  return buicpp::Compiler::UNKNOWN;
}

// Get bootstrapped C++ compiler
std::pair<const char*, buicpp::Compiler> get_native_compiler() {
#if NATIVE_COMPILER != NULL
  return {NATIVE_COMPILER, compiler_from_cstr(NATIVE_COMPILER)};
#else
  const char* env = getenv("CC");
  if (env && *env) return {env, compiler_from_cstr(env)};

  #if defined(__INTEL_LLVM_COMPILER)
    return {"icpx", buicpp::Compiler::INTEL_LLVM};
  #elif defined(__INTEL_COMPILER)
    return {"icpc", buicpp::Compiler::INTEL_CLASSIC};
  #elif defined(__clang__)
    return {"clang++", buicpp::Compiler::CLANG};
  #elif defined(__GNUC__)
    return {"g++", buicpp::Compiler::GCC};
  #elif defined(_MSC_VER)
    return {"cl", buicpp::Compiler::MSVC};
  #else
    #error "Unknown compiler, define NATIVE_COMPILER or CC manually"
  #endif
  assert(false && "unreachable: std::pair<const char*, buicpp::Compiler> get_native_compiler()");
#endif
}
// Buic impl end

namespace io {
  bool mkdir_if_not_exists(const char* path) {
    for (const char* p = path + 1; *p != '\0'; p++) {
      if (*p == PATH_SEP) {
        size_t i = (size_t)(p - path);
        if (i >= PATH_MAX) return false;
        char buf[PATH_MAX] = {0};
        memcpy(buf, path, i);
        if (!buicpp::io::mkdir(buf) && errno != EEXIST) return false;
      }
    }
    return true;
  }
}

}

#if __cplusplus >= 202002L || (defined(__cpp_lib_format) && __cpp_lib_format >= 201907L)
template <typename T>
auto std::formatter<buicpp::ArrayList<T>>::format(
  const buicpp::ArrayList<T>& arr,
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
#endif

#endif // BUICPP_IMPLEMENTATION

#endif // BUICPP_HPP
