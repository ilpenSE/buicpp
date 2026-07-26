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

#ifdef _WIN32
  #error "This library isn't ready for games"
#endif

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <time.h>
#include <dirent.h>
#include <algorithm>
#include <utility>
#include <ostream>
#include <format>
#include <version>
#include <type_traits>

namespace posix {
  using ::mode_t;
  using ::time_t;
  using ::ino_t;
  using ::ssize_t;
  using ::off_t;
  using ::DIR;
  using ::dirent;
  using ::stat;      // hem struct hem fonksiyon icin ayni isim, ikisi de gelir
  using ::lstat;
  using ::readlink;
  using ::mkdir;
  using ::opendir;
  using ::closedir;
  using ::readdir;
  using ::access;
  using ::fork;
  using ::execvp;
  using ::waitpid;

  constexpr auto ifmt  = S_IFMT;
  constexpr auto ifreg = S_IFREG;
  constexpr auto ifdir = S_IFDIR;
  constexpr auto ifchr = S_IFCHR;
  constexpr auto iflnk = S_IFLNK;
  constexpr auto ifblk = S_IFBLK;
  constexpr auto iwusr = S_IWUSR;
  constexpr auto irusr = S_IRUSR;
  constexpr auto ixusr = S_IXUSR;
  inline auto wifexited(int wstatus) { return WIFEXITED(wstatus); }
  inline auto wexitstatus(int wstatus) { return WEXITSTATUS(wstatus); }
} // namespace posix

#if defined(__unix__) || defined(__unix)
  #define PLATFORM_UNIX 1
#endif

#if defined(__linux__)
  #define PLATFORM_LINUX 1
#endif

#if defined(__APPLE__) || defined(__MACH__)
  #define PLATFORM_APPLE 1
#endif

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
  #define NATIVE_COMPILER NULL /* Detect compiler in runtime */
#endif

#ifdef BUICPP_NO_GLOBAL_NAMESPACE
namespace buicpp {
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
#ifdef BUICPP_NO_GLOBAL_NAMESPACE
}
#endif

namespace buicpp {

template <typename T, typename = void>
struct is_equality_comparable : std::false_type {};

template <typename T>
struct is_equality_comparable<T,
       std::void_t<decltype(std::declval<const T&>() == std::declval<const T&>())>>
       : std::true_type {};

template <typename T>
inline constexpr bool is_equality_comparable_v = is_equality_comparable<T>::value;

struct DeferHelper {
  template <typename F>
  struct Guard {
    F f;
    ~Guard() { f(); }
  };

  template <typename F>
  Guard<F> operator +(F f) { return Guard<F>{std::move(f)}; }
};

#define DEFER_CONCAT_(a, b) a##b
#define DEFER_CONCAT(a, b) DEFER_CONCAT_(a, b)
#define defer auto DEFER_CONCAT(_defer_, __LINE__) = buicpp::DeferHelper{} + [&]()

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
#ifndef ARRAYLIST_DEFAULT_CAPACITY
#define ARRAYLIST_DEFAULT_CAPACITY 64
#endif

template <typename T>
class ArrayList {
public:
  // Constructor / Destructor
  ArrayList(size_t init_capacity = ARRAYLIST_DEFAULT_CAPACITY) :
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

// Buic start
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
  posix::time_t mtime;
  posix::ino_t inode;
  ArrayList<File> files;
};

bool mkdir_if_not_exists(const char* path);

// Conversions
const char* to_string(FileType ft);
FilePermission to_filepermission(posix::mode_t mode);
FileType to_filetype(posix::mode_t mode);

// File utilities
Result<std::string> read_file_content(const char* file_path, size_t file_size = 0);
Result<File> read_file_metadata(const char* file_path, bool follow_symlink = true);
Result<File> read_entire_file(const char* file_path, bool follow_symlink = true);

// Directory utilities
Result<File> read_entire_directory(const char* dir_path, bool read_content = false);
Result<File> read_directory(const char* dir_path, bool read_content = false);

} // namespace io

posix::time_t compare_mtimes(const char* f1, const char* f2);

#define REBUILD_URSELF(argc, argv, ...) \
  buicpp::_buic_rebuild_urself((argc), (argv), __FILE__, ##__VA_ARGS__)

template<typename... Args>
bool _buic_rebuild_urself(int argc, char** argv, const char* file_name, Args... args);

// Buic end

} // namespace buicpp

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

  pid_t pid = posix::fork();
  if (pid == 0) {
    // child
    auto arr = to_argv();
    posix::execvp(arr[0], const_cast<char* const*>(arr.items()));
    std::fprintf(stderr, "[BUIC/ERROR] ");
    std::perror("execvp");
    std::exit(1);
  } else if (pid > 0) {
    // parent
    int wstatus = 0;

    posix::waitpid(pid, &wstatus, 0);
    if (posix::wifexited(wstatus)) {
      int exit_code = posix::wexitstatus(wstatus);
      if (exit_code != 0) {
        std::fprintf(stderr, "[BUIC/ERROR] Command failed with code %d\n", exit_code);
        return false;
      }
    }
  } else {
    std::fprintf(stderr, "[BUIC/ERROR] ");
    std::perror("fork");
    return false;
  }

  if (opts.is_reset) set_count(0);
  return true;
}

template<typename... Args>
bool _buic_rebuild_urself(int argc, char** argv, const char* file_name, Args... args) {
  const char* bin_name = argv[0];
  char old_bin[1024];
  snprintf(old_bin, sizeof old_bin, "%s.old", bin_name);

  bool needs_rebuild = false;
  if (posix::access(old_bin, F_OK) != 0) {
    needs_rebuild = true;
  } else {
    needs_rebuild = compare_mtimes(file_name, bin_name) >= 0;
  }

  if (!needs_rebuild) return true;
  printf("INFO: Change detected in build script, rebuilding itself.\n");

  // Rename the binary to old one
  printf("INFO: Renaming: '%s' -> '%s'\n", bin_name, old_bin);
  if (std::rename(bin_name, old_bin) != 0) {
    fprintf(stderr, "ERROR: cannot rename '%s': %s\n", bin_name, strerror(errno));
    return false;
  }

  // Construct and run rebuild command
  std::pair<const char*, Compiler> comp = get_native_compiler();
  if (comp.second == Compiler::UNKNOWN) {
    fprintf(stderr, "ERROR: Unknown compiler: '%s'\n", comp.first);
    return false;
  }
  CommandBuilder cmd;
  cmd.push(comp.first);
  cmd.push(file_name);
  cmd.push_many(std::forward<Args>(args)...); // custom flags if you need
  cmd.push_many("-o", bin_name);

  // Run rebuild command
  if (!cmd.run()) {
    fprintf(stderr, "ERROR: cannot rebuild itself.\n");
    return false;
  }

  // Run the new binary and exit this old one
  posix::execvp(bin_name, argv);
  fprintf(stderr, "ERROR: cannot run new binary: %s\n", strerror(errno));
  return false;
}

posix::time_t compare_mtimes(const char* f1, const char* f2) {
  struct posix::stat st_f1, st_f2;
  if (posix::stat(f1, &st_f1) != 0) {
    fprintf(stderr, "ERROR: cannot stat '%s': %s\n", f1, strerror(errno));
    return false;
  }

  if (posix::stat(f2, &st_f2) != 0) {
    fprintf(stderr, "ERROR: cannot stat '%s': %s\n", f2, strerror(errno));
    return false;
  }
  return (posix::time_t)(st_f1.st_mtime - st_f2.st_mtime);
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
  UNREACHABLE("const char* to_string(buicpp::Compiler e)");
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
  UNREACHABLE("std::pair<const char*, buicpp::Compiler> get_native_compiler()");
#endif
}
// Buic impl end

namespace io {
bool mkdir_if_not_exists(const char* path) {
  for (const char* p = path + 1; *p != '\0'; p++) {
    if (*p == '/') {
      size_t i = (size_t)(p - path);
      if (i >= PATH_MAX) return false;
      char buf[PATH_MAX] = {0};
      std::memcpy(buf, path, i);
      if (posix::mkdir(buf, 0775) != 0 && errno != EEXIST) return false;
    }
  }
  return true;
}

// no recursion here
Result<File> read_directory(const char* dir_path, bool read_content) {
  // open directory
  DIR *dir_ptr = posix::opendir(dir_path);
  if (!dir_ptr) return Err({errno, "opendir failed", __FILE__, __LINE__});
  defer { posix::closedir(dir_ptr); };

  // stat of directory itself
  auto ret = read_file_metadata(dir_path);
  if (ret.is_err()) return Err(ret.error());
  File dir = std::move(ret).value();

  // iterate directory (no recursive look)
  struct posix::dirent *entry;
  while ((entry = posix::readdir(dir_ptr)) != NULL) {
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
  struct posix::stat st;
  if (follow_symlink) {
    if (posix::stat(file_path, &st) != 0) return Err({errno, "stat failed", __FILE__, __LINE__});
  } else {
    if (posix::lstat(file_path, &st) != 0) return Err({errno, "lstat failed", __FILE__, __LINE__});
  }

  result.type = to_filetype((posix::mode_t)st.st_mode);
  result.name = std::string(file_path);
  result.mtime = st.st_mtime;
  result.inode = st.st_ino;
  result.permissions = to_filepermission(st.st_mode);
  result.size = st.st_size;

  if (result.type == FileType::SYMLINK) {
    char buf[PATH_MAX];
    ssize_t n = posix::readlink(file_path, buf, sizeof(buf));
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

FileType to_filetype(posix::mode_t mode) {
  switch (mode & posix::ifmt) {
  case posix::ifreg: return FileType::REGULAR;
  case posix::ifdir: return FileType::DIRECTORY;
  case posix::ifchr: return FileType::CHARDEV;
  case posix::iflnk: return FileType::SYMLINK;
  case posix::ifblk: return FileType::BLOCKDEV;
  default:      return FileType::UNKNOWN;
  }
  UNREACHABLE("to_filetype");
}

FilePermission to_filepermission(mode_t mode){
  FilePermission perms = FilePermission::NONE;
  if (mode & posix::irusr) perms |= FilePermission::READ;
  if (mode & posix::iwusr) perms |= FilePermission::WRITE;
  if (mode & posix::ixusr) perms |= FilePermission::EXECUTE;
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
  UNREACHABLE("const char* to_string(buicpp::io::FileType e)");
}

} // namespace io

} // namespace buicpp

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
