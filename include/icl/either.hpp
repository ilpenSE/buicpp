/**
 * @file either.hpp
 * @author ilpeN
 * @since 1.0.0
 * @brief Provides Rust-like Either, Result and Option interface
*/
#ifndef ICL_EITHER_HPP
#define ICL_EITHER_HPP

#include "./base.hpp"
#include <utility>

namespace icl {
/**
 * @brief Tag types used to pick a side when constructing an Either
 * @details
 * Pass LTag{} to build the left side, or RTag{} to build the right
 * side. These exist because the constructor otherwise couldn't tell
 * which side you mean, especially when L and R are the same type or
 * convertible into each other.
*/
struct LTag {};
struct RTag {};

/**
 * @class Either<L, R>
 * @since 1.0.0
 * @brief Holds either a value of type L or a value of type R, never both
 * @details
 * This works like a tagged union (similar to std::variant with 2
 * types), with a simple API:
 * - is_left() / is_right() to check which side is active
 * - left() / right() to get the stored value
 * - match() to handle both cases with two callbacks at once
 *
 * Either is the base type this library builds Option<T> (a value or
 * nothing) and Result<T, E> (a value or an error) on top of.
 *
 * @tparam L type stored on the left side
 * @tparam R type stored on the right side
*/
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

/**
 * @struct LeftOf<L>
 * @brief Helper used to build the left side of an Either via implicit conversion
 * @details
 * Wrap a value with LeftOf{value} and assign or return it as an
 * Either<L, R> — the conversion operator figures out L for you.
 * @tparam L type of the wrapped value
*/
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

/**
 * @struct RightOf<R>
 * @brief Helper used to build the right side of an Either via implicit conversion
 * @details
 * Wrap a value with RightOf{value} and assign or return it as an
 * Either<L, R> — the conversion operator figures out R for you.
 * @tparam R type of the wrapped value
*/
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

/**
 * @struct Nothing
 * @brief Empty type used as the "no value" side of Option<T>
*/
struct Nothing{};

/**
 * @class Option<T>
 * @since 1.0.0
 * @brief Rust-like Option<T>: holds either a value or nothing
 * @details
 * Option<T> is Either<T, Nothing> with a friendlier API:
 * is_some()/is_none() instead of is_left()/is_right(), and some() to
 * get the stored value. Build one with icl::Some{value} or icl::None.
 * @tparam T type of the value when present
*/
template <typename T>
class Option : public Either<T, Nothing> {
  using Base = Either<T, Nothing>;
public:
  using Base::Base;

  bool is_some() const { return this->is_left(); }
  bool is_none() const { return this->is_right(); }
  T&& some() && { return std::move(*this).left(); }
  const T& some() const& { return this->left(); }
};

/**
 * @struct Some
 * @brief Helper used to build an Option<T> that holds a value
 * @details
 * Wrap a value with icl::Some{value} and assign or
 * return it as an Option<T>.
 * @tparam T type of the wrapped value
*/
template <typename T>
struct Some {
  T value;
  Some(const T& v) : value(v) {}
  Some(T&& v) : value(std::move(v)) {}

  operator Option<T>() && { return Option<T>(std::move(value), LTag{}); }
  operator Option<T>() const& { return Option<T>(value, LTag{}); }
};

/**
 * @struct NoneTag
 * @brief Helper type used to build an empty Option<T>
 * @details
 * You should not need to use this type directly,
 * use the icl::None constant below instead.
*/
struct NoneTag {
  template <typename T>
  operator Option<T>() const { return Option<T>(Nothing{}, RTag{}); }
};

/**
 * @constant None
 * @brief Global constant meaning "no value", converts to any Option<T>
 * @code{.cpp}
 * icl::Option<int> opt = icl::None;
 * @endcode
*/
inline constexpr NoneTag None = {};

/**
 * @struct Error
 * @brief Default error type used by Result<T, E>
 * @details
 * A simple error struct carrying an error code, a message,
 * and the file/line where the error was created.
 * Useful for debugging without needing exceptions.
*/
struct Error {
  int code;
  const char* msg;
  const char* file;
  size_t line;
};

/**
 * @class Result
 * @since 1.0.0
 * @brief Rust-like Result<T, E>: holds either a valid value or an error
 * @details
 * Result<T, E> is Either<T, E> with a friendlier, error-handling API:
 * is_ok()/is_error() instead of is_left()/is_right(), ok() to get the
 * successful value, and error() to get the error. Build one with
 * Ok{value} or Err{error}.
 * You can always do pattern matching with lambdas
 *
 * E defaults to icl::Error, so a plain `Result<int>` means
 * `Result<int, icl::Error>`. You can use your own error type instead,
 * for example `Result<int, std::string>`.
 *
 * @code{.cpp}
 * icl::Result<int> divide(int a, int b) {
 *   if (b == 0) return icl::Err({1, "division by zero", __FILE__, __LINE__});
 *   return icl::Ok(a / b);
 * }
 *
 * auto r = divide(10, 0);
 * if (r.is_error()) icl::eprintln("error: {}", r.error().msg);
 *
 * divide(10, 0).match(
 * [](auto res){
 *   icl::println("division success, result = {}", res);
 * },
 * [](auto err){
 *   icl::eprintln("error: {}", r.error().msg);
 * });
 * if (r.is_error())
 * @endcode
 *
 * @tparam T type of the value on success
 * @tparam E type of the error (icl::Error by default)
*/
template <typename T, typename E = Error>
class Result : public Either<T, E> {
  using Base = Either<T, E>;
public:
  using Base::Base;

  bool is_ok() const { return this->is_left(); }
  bool is_error() const { return this->is_right(); }
  T&& ok() && { return std::move(*this).left(); }
  const T& ok() const& { return this->left(); }
  const E& error() const& { return this->right(); }
};

/**
 * @struct Ok
 * @since 1.0.0
 * @brief Helper used to build a successful Result<T, E>
 * @details
 * Wrap a value with Ok{value} and return it from a function whose
 * return type is Result<T, E>, the conversion operator builds the
 * successful side for you, and E defaults to icl::Error
 * so you usually don't have to name it.
 * @tparam T type of the wrapped value
*/
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

/**
 * @struct Err
 * @since 1.0.0
 * @brief Helper used to build a failed Result<T, E>
 * @details
 * Wrap an error with Err{error} and return it from a function whose
 * return type is Result<T, E> — the conversion operator builds the
 * error side for you. Unlike Ok, E is not defaulted here since the
 * error type is what Err itself is templated on.
 * @tparam E type of the wrapped error (icl::Error by default)
*/
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

} // namespace icl
#endif // ICL_EITHER_HPP
