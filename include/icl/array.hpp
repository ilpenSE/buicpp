/**
 * @file array.hpp
 * @author ilpeN
 * @since 1.0.0
*/
#ifndef ICL_ARRAY_HPP
#define ICL_ARRAY_HPP

#include "./base.hpp"
#include <ostream>
#include <utility>

namespace icl {
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
inline bool ArrayList<T>::reserve(size_t extra) {
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
    *(d_first++) = std::move_if_noexcept(*(first++));
  }
  return true;
}

template <typename T>
inline bool ArrayList<T>::shift_right(size_t start, size_t end, size_t amount) {
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
      *d_last = std::move_if_noexcept(*last);
    }
  }
  return true;
}

template <typename T>
template <typename U>
inline bool ArrayList<T>::add_impl(U&& item, size_t idx) {
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
inline bool ArrayList<T>::push_many(Args&&... args) {
  if (!reserve(sizeof...(args))) return false;
  bool ok = true;
  auto try_push = [&](auto&& item){
    if (ok) ok = push(std::forward<decltype(item)>(item));
  };
  (try_push(std::forward<Args>(args)), ...);
  return ok;
}

template <typename T>
inline T ArrayList<T>::remove(size_t idx) {
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
inline T ArrayList<T>::remove_unord(size_t idx) {
  assert(idx >= m_count && "Boundary check failed in remove_unord");
  T out = std::move_if_noexcept(m_items[idx]);
  if (idx != m_count - 1) m_items[idx] = std::move(m_items[m_count - 1]);
  m_items[m_count - 1].~T();
  m_count--;
  return out;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const ArrayList<T>& arr) {
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
inline void ArrayList<T>::construct_range(It first, It last) {
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
} // namespace icl

#if __cplusplus >= 202002L || (defined(__cpp_lib_format) && __cpp_lib_format >= 201907L)
#include <format>
template <typename T>
struct std::formatter<icl::ArrayList<T>> : std::formatter<std::string> {
  auto format(const icl::ArrayList<T>& arr, std::format_context& ctx) const -> decltype(ctx.out())
  {
    std::string result = "[";
    for (size_t i = 0; i < arr.count(); i++) {
      result += std::format("{}", arr[i]);
      if (i != arr.count() - 1) result += ", ";
    }
    result += "]";
    return std::formatter<std::string>::format(result, ctx);
  }
};
#endif // __cplusplus

#endif // ICL_ARRAY_HPP
