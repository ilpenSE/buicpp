#ifndef BUICPP_HPP
#define BUICPP_HPP

/*
  This is a C++ library for building C++ in C++.
  It has additional STL things (for example dynamic arrays) you can use them
  But they're not as production-ready as STL.
*/

#include <algorithm>
#include <utility>
#include <ostream>
#include <cstddef>
#include <version>

namespace buicpp {

// Either start
struct LTag {};
struct RTag {};
inline constexpr LTag EitherLeft = LTag{};
inline constexpr RTag EitherRight = RTag{};

template <typename L, typename R>
class Either {
public:
  Either(const L& value, LTag)
    : m_left(value), m_is_left(true) {}
  Either(L&& value, LTag)
    : m_left(std::move(value)), m_is_left(true) {}

  Either(const R& value, RTag)
    : m_right(value), m_is_left(false) {}
  Either(R&& value, RTag)
    : m_right(std::move(value)), m_is_left(false) {}

  ~Either() { if (m_is_left) m_left.~L(); else m_right.~R(); }

  Either(const Either& other) : m_is_left(other.m_is_left)
  {
    if (other.m_is_left) new (&m_left) L(other.m_left);
    else new (&m_right) R(other.m_right);
  }

  Either& operator =(const Either& other)
  {
    if (this == &other) return *this;
    if (m_is_left == other.m_is_left) {
      if (m_is_left) m_left = other.m_left;
      else m_right = other.m_right;
    } else {
      if (m_is_left) m_left.~L();
      else m_right.~R();
      if (other.m_is_left) new (&m_left) L(other.m_left);
      else new (&m_right) R(other.m_right);
      m_is_left = other.m_is_left;
    }
    return *this;
  }

  Either(Either&& other) noexcept :
    m_is_left(other.m_is_left)
  {
    if (other.m_is_left) new (&m_left) L(std::move(other.m_left));
    else new (&m_right) R(std::move(other.m_right));
  }

  Either& operator =(Either&& other) noexcept
  {
    if (this == &other) return *this;
    if (m_is_left == other.m_is_left) {
      if (m_is_left) {
        m_left = std::move(other.m_left);
      } else {
        m_right = std::move(other.m_right);
      }
    } else {
      if (m_is_left) m_left.~L();
      else m_right.~R();
      if (other.m_is_left) new (&m_left) L(std::move(other.m_left));
      else new (&m_right) R(std::move(other.m_right));
      m_is_left = other.m_is_left;
    }
    return *this;
  }

  bool is_left() { return m_is_left; }
  bool is_right() { return !m_is_left; }
  L left() { return m_left; }
  R right() { return m_right; }

  bool operator ==(const Either& other) const {
    if (m_is_left != other.m_is_left) return false;
    if (m_is_left) return m_left == other.m_left;
    else return m_right == other.m_right;
  }
  bool operator !=(const Either& other) const {
    return !(*this == other);
  }

  void swap(Either& other) {
    if (this == &other) return;
    Either tmp(std::move(*this));
    *this = std::move(other);
    other = std::move(tmp);
  }

private:
  bool m_is_left = true;
  union {
    L m_left;
    R m_right;
  };
};

// TODO: Write different constructor for Option/Result
// struct None {};
// template<typename T>
// using Option = Either<T, None>;

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
  bool push(T item) { return add(item, m_count); }
  bool add(T item, size_t idx);

  // Remove element from idx (shifts array if you're not popping from back)
  bool pop(T* out = nullptr) { return remove(m_count, out); }
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
  T* begin() { return m_items; }
  T* end() { return m_items + m_count; }
  const T* cbegin() const { return m_items; }
  const T* cend()   const { return m_items + m_count; }

  size_t count() const { return m_count; }
  size_t capacity() const { return m_capacity; }

private:
  T* m_items;
  size_t m_count = 0;
  size_t m_capacity;
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const ArrayList<T>& arr);
}

#if __cplusplus >= 202002L || (defined(__cpp_lib_format) && __cpp_lib_format >= 201907L)
template <typename T>
struct std::formatter<buicpp::ArrayList<T>> : std::formatter<std::string> {
  auto format(const buicpp::ArrayList<T>& arr, std::format_context& ctx) const -> decltype(ctx.out());
};
#endif
// Dynamic Arrays end

#ifdef BUICPP_IMPLEMENTATION

#include <cassert>
#include <cstdlib>

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
bool ArrayList<T>::add(T item, size_t idx) {
  if (idx > m_count) return false;
  if (!reserve(1)) return false;
  bool ok = shift_right(idx, m_count);
  assert(ok && "Shifting right in ArrayList<T>::add failed, this should never fail");
  (void)ok;
  m_items[idx] = std::move(item);
  m_count++;
  return true;
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
// Dynamic Arrays implementation end

#endif // BUICPP_IMPLEMENTATION

#endif // BUICPP_HPP
