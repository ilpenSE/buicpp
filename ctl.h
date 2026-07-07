/* SPDX-License-Identifier: GPL-3.0-only */
/* This is an amalgamation of libctl headers. */
#ifndef CTL_H
#define CTL_H

#ifdef _MSC_VER
  #error "This header does not support MSVC, please don't use garbage slop compilers."
#endif

#ifdef __cplusplus
  #define CTLDEF extern "C"
#else
  #define CTLDEF extern
#endif

/*
  Rust's Result<T, E> implementation in C.
  This library DOES NOT DEPEND ANYTHING EXCEPT LIBC!
  This header supposed to be used ONLY FOR C!
  DO NOT USE RAW POINTERS IN EITHER/OPTION/RESULT NAME PARAMS!
  Instead, use (for int*) int_ptr or if you have a special pointer type
  define typedef of it. Because something like this: Result(int*)
  gives some errors. But this is safe: Result(int_ptr)
*/

#include <stddef.h>
#include <stdbool.h>

#define ERROR_CODES \
  X(INVALID_ARG, "Invalid argument") \
  X(NOT_FOUND, "Not found") \
  X(VALIDATION_FAILED, "Validation failed") \
  X(INTERNAL, "Internal error") \
  X(NULL_PTR, "Null pointer") \
  X(NO_SUCH_FILE, "No such file or directory") \
  X(MEM_CORRUPT, "Memory is corrupted")

/* Error Code enum, you can add or change here */
typedef enum {
  ERR_NOERR = 0,
#define X(name, _) ERR_##name,
  ERROR_CODES
#undef X
  _ErrorCode_count,
} ErrorCode;

typedef struct {
  ErrorCode code;
  const char* message;
} Error;

CTLDEF const char* err_tostr(ErrorCode err_code);

#define MAKE_ERR(ecode, emsg) \
  ((Error){.code=(ecode), .message=(emsg)})

/*
  DISCLAIMER: LT, RT or T is a type name so this means
  if one of them is pointer, you have to use typedef of it.
  For example: DECL_EITHER(int*, bool) won't work
  But, DECL_EITHER(int_ptr, bool) will work
  This pattern is everywhere I put generic type like "T"
*/

/*
  Either(L, R) holds 2 different typed value
*/
#define Either(LT, RT) Either_##LT##_##RT

#define DECL_EITHER(LT, RT) \
  typedef struct {                              \
    bool is_left;                               \
    union {                                     \
      LT left;                               \
      RT right;                              \
    } as;                                       \
  } Either(LT, RT);

/* Producing Either structs for returns at functions */
#define EITHER_L(LName, RName, lval)                          \
  (Either(LName, RName)){ .is_left = 1, .as.left = (lval) }
#define EITHER_R(LName, RName, rval)                            \
  (Either(LName, RName)){ .is_left = 0, .as.right = (rval) }

/* Unwrap mechanism to get R (right) and L (left) */
#define EITHER_GETL(either)                     \
  ((either).as.left)
#define EITHER_GETR(either)                     \
  ((either).as.right)

/* we have already is_left field to query value is left or right */

/* Result(T) aka Either(T, Error) (Wrapper of Either) */
#define Result(T) Either(T, Error)
#define DECL_RESULT(T) DECL_EITHER(T, Error)

/* Producing Result objects for returns */
#define RES_OK(T, val)                      \
  EITHER_L(T, Error, (val))
#define RES_ERR_MSG(T, errcode, errmsg)         \
  EITHER_R(T, Error, MAKE_ERR(errcode, errmsg))
#define RES_ERR(T, errcode)         \
  EITHER_R(T, Error, MAKE_ERR(errcode, NULL))

/* Unwraps blindly, just unwrap */
#define RES_UNWRAP(res)                         \
  ((res).as.left)

/* Left: Value, Right: Error */
#define RES_ISERR(res)                          \
  (!(res).is_left)

/* Get error without asking */
#define RES_GETE(res) \
  (EITHER_GETR((res)))

/* Option(T) */
#define Option(T) Option_##T

#define DECL_OPTION(T)               \
  typedef struct {                              \
    bool is_some;                               \
    T value;                                \
  } Option(T);

/* Producing Option structs for returns */
#define OPT_SOME(T, val)                     \
  ((Option(T)){ .is_some = true, .value = (val) })
#define OPT_NONE(T)                         \
  ((Option(T)){ .is_some = false })

/* Unwrap blindly */
#define OPT_UNWRAP(opt)                         \
  ((opt).value)

/* we have already is_some to query the value is either Some or None */

/*
  like Option<void>,
  instead of using Option<bool>, use this
  (if bool is just an error state)
*/
typedef struct {
  bool is_error;
  Error error;
} ErrorOrNot;

#define EON_SUCCESS ((ErrorOrNot){ false, { ERR_NOERR, NULL } })
#define EON_ERROR(EEnum, EMessage) ((ErrorOrNot){ true, { .code=EEnum, .message=EMessage } })


#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define ARR_INITIAL_CAPACITY 64
static_assert(ARR_INITIAL_CAPACITY > 1, "ARR_INITIAL_CAPACITY must be >1");

#ifndef __arr_allocator_t_defined
#define __arr_allocator_t_defined
typedef void* (*arr_allocator_t)(size_t);
#endif /* __arr_allocator_t_defined */

#ifndef __arr_reallocator_t_defined
#define __arr_reallocator_t_defined
typedef void* (*arr_reallocator_t)(void*, size_t);
#endif /* __arr_reallocator_t_defined */

#ifndef __arr_freer_t_defined
#define __arr_freer_t_defined
typedef void  (*arr_freer_t)(void*);
#endif /* __arr_freer_t_defined */

typedef struct {
  arr_allocator_t allocator;
  arr_reallocator_t reallocator;
  arr_freer_t freer;
} ArrayMemory;

typedef struct {
  size_t cap;
  size_t len;
  bool is_freed;
  ArrayMemory memory;
} ArrayHeader;

typedef struct {
  ArrayHeader* header;
  void** items;
  size_t elem_size;
} ArrayGeneric;

#define ARR_TO_GENERIC(v) ((ArrayGeneric){&(v)->h, (void**)&(v)->items, sizeof(*(v)->items)})

/*
  DISCLAIMER: LT, RT or T is a type name so this means
  if one of them is pointer, you have to use typedef of it.
  For example: DECL_EITHER(int*, bool) won't work
  But, DECL_EITHER(int_ptr, bool) will work
  This pattern is everywhere I put generic type like "T"
*/

#define Array(T) Array_##T

#define DECL_ARRAY(T) \
  typedef struct {    \
    ArrayHeader h;    \
    T* items;         \
  } Array(T);

/* User-space macros, you want to use them: */

#define arr_init(T, ...) (Array(T)){ .h.memory={__VA_ARGS__}}

#define arr_push(v, ...)                                           \
  __extension__({                                                       \
    __typeof__(*(v)->items) _arr[] = {__VA_ARGS__};                \
    _arr_push(ARR_TO_GENERIC((v)), _arr, sizeof(_arr)/sizeof(_arr[0])); \
  })

/*
  Not used this because of double evaluation of v1 and v2:
  #define arr_merge(v1, v2) (
    __builtin_types_compatible_p(__typeof__(*(v1)->items), __typeof__(*(v2)->items))
      ? _arr_merge(ARR_TO_GENERIC((v1)), ARR_TO_GENERIC((v2)))
      : false)
  Used statement expression extension: (only evaluated once)
*/
#define arr_merge(v1, v2) \
  __extension__({ \
    __typeof__(v1) _v1 = (v1); \
    __typeof__(v2) _v2 = (v2); \
    __builtin_types_compatible_p(__typeof__(*(_v1)->items), __typeof__(*(_v2)->items)) \
        ? _arr_merge(ARR_TO_GENERIC((_v1)), ARR_TO_GENERIC((_v2))) \
        : false; \
  })

#define arr_remove_unord(v, idx) \
  _arr_remove_unord(ARR_TO_GENERIC((v)), (idx))

#define arr_remove_idx(v, idx) \
  _arr_remove_idx(ARR_TO_GENERIC((v)), (idx))

#define arr_at(v, index)                        \
  ((__typeof__(*(v)->items)*)                \
   _arr_at(ARR_TO_GENERIC((v)), (index)))

#define arr_find(v, item)                       \
  __extension__({                               \
      __typeof__(*(v)->items) _tmp = (item); \
      _arr_find(ARR_TO_GENERIC((v)), &_tmp);    \
    })

#define arr_contains(v, item) (arr_find(v, item) != -1)

#define arr_pop(v, out)                         \
  _arr_pop(ARR_TO_GENERIC((v)), (out))

#define arr_reserve(v, extra)                   \
  _arr_reserve(ARR_TO_GENERIC((v)), (extra))

#define arr_free(v) _arr_free(ARR_TO_GENERIC((v)))
#define arr_isfreed(v) _arr_isfreed(ARR_TO_GENERIC((&v)))

/* Array(T) arr_cast(T type, Array(E) arr) */
#define arr_cast(T, arr) (Array(T)){.items=(T*)((arr).items), .h=(arr).h}

#define arr_equals(lhs, rhs)                                \
  _arr_equals(ARR_TO_GENERIC((lhs)), ARR_TO_GENERIC((rhs)))

/* Convenience accessors */
#define arr_len(v)   ((v).h.len)
#define arr_cap(v)   ((v).h.cap)
#define arr_esize(v) (sizeof(*(v).items))

/* They'll return pointer */
#define arr_last(v) (arr_at((v), arr_len((*v)) - 1))
#define arr_first(v) (arr_at((v), 0))

#define arr_islast(v, item) (arr_last((v)) ? (*arr_last((v)) == item) : false)
#define arr_isfirst(v, item) (arr_first((v)) ? (*arr_first((v)) == item) : false)

#define arr_foreach(v, it)                                              \
  for(__typeof__(*(v).items)* it =                                  \
        (assert(!arr_isfreed((v)) && "Array shouldn't be freed (possible use-after-free)"), (v).items); \
      it < (v).items + arr_len((v)); it++)

/* Raw Functions */

CTLDEF bool _arr_push(ArrayGeneric v, const void* values, size_t count);
CTLDEF bool _arr_remove_unord(ArrayGeneric v, size_t idx);
CTLDEF bool _arr_remove_idx(ArrayGeneric v, size_t idx);
CTLDEF void* _arr_at(ArrayGeneric v, size_t index);
CTLDEF int _arr_find(ArrayGeneric v, const void* item);
CTLDEF bool _arr_pop(ArrayGeneric v, void* out);
CTLDEF bool _arr_reserve(ArrayGeneric v, size_t extra);
CTLDEF void _arr_free(ArrayGeneric v);
CTLDEF bool _arr_isfreed(ArrayGeneric v);
CTLDEF bool _arr_equals(ArrayGeneric lhs, ArrayGeneric rhs);
CTLDEF bool _arr_merge(ArrayGeneric v1, ArrayGeneric v2);

#include <stddef.h>
#include <stdbool.h>

#define DECL_BUFFER(T, TName, SIZE) \
  typedef struct {                  \
    T items[SIZE];                  \
    size_t len;                     \
  } Buffer_##TName##_##SIZE;

#define Buffer(TName, S) Buffer_##TName##_##S

typedef struct {
  void** items;
  const size_t elem_size;
  const size_t nmemb;
  size_t* len;
} BufferGeneric;

#define BUF_TO_GENERIC(buffer) \
  (BufferGeneric){.items = (void**)(&(buffer).items), \
                  .elem_size = sizeof(*(buffer).items), \
                  .nmemb = sizeof((buffer).items) / sizeof(*(buffer).items), \
                  .len = &(buffer).len,}

#define buf_push(buffer, ...) \
  __extension__({ \
    __typeof__(*(buffer).items) _arr[] = {__VA_ARGS__}; \
    _buf_push(BUF_TO_GENERIC((buffer)), _arr, sizeof(_arr)/sizeof(*_arr)); \
  })

CTLDEF bool _buf_push(BufferGeneric buf, const void* values, size_t count);


#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(__unix__) || defined(__unix)
  #define PLATFORM_UNIX 1
  #define PLATFORM_POSIX 1
#elif defined(__APPLE__) || defined(__MACH__)
  #define PLATFORM_APPLE 1
  #define PLATFORM_POSIX 1
#elif defined(_WIN32)
  #define PLATFORM_WINDOWS 1
#endif

#ifdef PLATFORM_POSIX
  #include <time.h>
  typedef time_t ctl_time_t;
  #define ctl_execvp(file_name, argv) execvp((file_name), (argv))
#elif defined(PLATFORM_WINDOWS)
  #include <process.h>
  typedef long long ctl_time_t;
  #define ctl_execvp(file_name, argv) _execvp((file_name), (const char* const*)(argv))
#endif

/* Custom malloc, realloc and free function types (if you have some sort of an arena) */
#ifndef __str_allocator_t_defined
#define __str_allocator_t_defined
typedef void* (*str_allocator_t)(size_t);
#endif

#ifndef __str_reallocator_t_defined
#define __str_reallocator_t_defined
typedef void* (*str_reallocator_t)(void*, size_t);
#endif

#ifndef __str_freer_t_defined
#define __str_freer_t_defined
typedef void  (*str_freer_t)(void*);
#endif

#ifndef __StringMemory_defined
#define __StringMemory_defined
typedef struct {
  str_allocator_t allocator;
  str_reallocator_t reallocator;
  str_freer_t freer;
} StringMemory;
#endif

#ifndef __String_defined
#define __String_defined
typedef struct {
  char* data;
  size_t len; /* does not include \0 */
  size_t cap;
  StringMemory memory;
} String;
#endif

#ifndef __StringView_defined
#define __StringView_defined
typedef struct {
  const char* data; /* not null terminated */
  size_t len;
} StringView;
#endif

#ifndef __StringSlice_defined
#define __StringSlice_defined
typedef struct {
  size_t start;
  size_t end;
} StringSlice;
#endif

#ifndef __Result_String_defined
#define __Result_String_defined
DECL_RESULT(String);
#endif

#ifndef __Result_double_defined
#define __Result_double_defined
DECL_RESULT(double);
#endif

#ifndef __Result_bool_defined
#define __Result_bool_defined
DECL_RESULT(bool);
#endif

/* Predefined typedefs to use in DECL_ARRAY and Array() */
#ifndef __char_ptr_defined
#define __char_ptr_defined
typedef char* char_ptr;
#endif

#ifndef __cchar_ptr_defined
#define __cchar_ptr_defined
typedef const char* cchar_ptr;
#endif

#ifndef __int_ptr_defined
#define __int_ptr_defined
typedef int* int_ptr;
#endif

#ifndef __uchar_t_defined
#define __uchar_t_defined
typedef unsigned char uchar_t;
#endif

/* Array declerations */
#ifndef __Array_cchar_ptr_defined
#define __Array_cchar_ptr_defined
DECL_ARRAY(cchar_ptr);
#endif

#ifndef __Array_char_ptr_defined
#define __Array_char_ptr_defined
DECL_ARRAY(char_ptr);
#endif

/* Utility macros */
/* TODO macro, use this for not-implemented features */
#define TODO(fmt, ...) \
  do { \
    fprintf(stderr, "%s:%d: TODO: "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS); \
    abort(); \
  } while (0)

/* UNREACHABLE macro, use it for regions that are not meant to be reached */
#define UNREACHABLE(fmt, ...) \
  do { \
    fprintf(stderr, "%s:%d: UNREACHABLE: "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    __builtin_unreachable(); \
  } while(0)

#define STRINGIFY(macro) #macro

/* Temporary buffer */
#define BASIC_TEMP_BUFFER_SIZE 1025

/* Math */
#define _BASIC_TYPES_WITH_BITS(X) \
  X(u64, uint64_t) \
  X(u32, uint32_t) \
  X(u16, uint16_t) \
  X(u8,  uint8_t) \
  \
  X(i64, int64_t) \
  X(i32, int32_t) \
  X(i16, int16_t) \
  X(i8,  int8_t)

/* Declerations */
CTLDEF char* temp_sprintf(const char* fmt, ...);
CTLDEF char* temp_vsprintf(const char* fmt, va_list args);
CTLDEF size_t temp_len();
CTLDEF void temp_reset();
CTLDEF void temp_clear();
CTLDEF const char* argv_shift(int* argc, char*** argv);

#define _BASIC_CLAMP_DECLARATION(TName, T) \
  T clamp##TName(T a, T lower, T upper); \
  typedef T TName;
_BASIC_TYPES_WITH_BITS(_BASIC_CLAMP_DECLARATION)

#define _BASIC_CLAMP_CASE(TName, T) T: clamp##TName,
#define clamp(a, lower, upper) _Generic((a), \
    _BASIC_TYPES_WITH_BITS(_BASIC_CLAMP_CASE) \
    default: clampi32 \
)(a, lower, upper)

int powii(int base, int exp);

#include <string.h>
#include <stddef.h>

#define SV_ARG(sv) (int)(sv).len, (sv).data
#define SV_FMT "%.*s"

#define sv_foreach(sv, it) \
  for (const char* it = (sv)->data; it < (sv)->data + (sv)->len; it++)

/*
  Usage: sv_from_str(s, .start = 0, .end = s->len)
  or: sv_from_str(s), sv_from_str(s, .end = 10) (default values'll be 0)
*/
#define svs(s, ...) \
  sv_from_str_impl((s), (StringSlice){.start=0, .end=(s)->len, ##__VA_ARGS__ })

/*
  Slices heap-allocated string, wrapper to sv_from_cstr
*/
CTLDEF StringView sv_from_str_impl(const String* s, StringSlice slice);

#define svn(buf, len, ...) \
  sv_from_cstrn_impl((buf), (len), (StringSlice){.end=len-1,__VA_ARGS__})

/*
  Slices string in range [start, end) and makes new StringView
  Length, start and end are checked
*/
CTLDEF StringView sv_from_cstrn_impl(const char* buf, size_t len, StringSlice slice);

#define sv(buf, ...) \
  sv_from_cstr_impl((buf), (StringSlice){.end=(strlen((buf))), ##__VA_ARGS__})

/*
  Produce StringView from zero-ended const strings without explicit start and end
*/
CTLDEF StringView sv_from_cstr_impl(const char* buf, StringSlice slice);

/*
  Take sub string from string view and return the sub string
*/
CTLDEF StringView sv_substr(StringView sv, size_t start, size_t len);

/*
  Trims the string view from left or right or both sides
*/
CTLDEF StringView sv_trim_left(StringView sv);
CTLDEF StringView sv_trim_right(StringView sv);
CTLDEF StringView sv_trim(StringView sv);

/*
  Chop n characters from left or right
  modifies given sv and constructs removed part then returns it
*/
CTLDEF StringView sv_chop_left(StringView* sv, size_t n);
CTLDEF StringView sv_chop_right(StringView* sv, size_t n);

/* Synonyms for chops */
#define sv_remove_prefix sv_chop_left
#define sv_remove_suffix sv_chop_right

/*
  Start from left, chop until hitting the first delimiter character
  Removes chopped part and delimiter char from string view
  Returns that chopped part
  If no delimiter char found, empty StringView'll be returned
*/
CTLDEF StringView sv_chop_by_delim(StringView* sv, char delim);

/*
  Chop string view, starting from left, by a function
  Function can be from ctype.h (isspace, isblank etc.)
*/
CTLDEF StringView sv_chop_by_func(StringView* sv, int (*func)(int));

/*
  Writes sv's data with length into zero-terminated string
  DICLAIMER: make sure that out buffer's size is enough
  otherwise you will get buffer overflow
*/
CTLDEF void sv_tostr(StringView sv, char* out);

/*
  Compares 2 string views and check if they're equals
  Basically: sv_cmp(lhs, rhs) == 0 like strcmp(lhs, rhs) == 0
*/
CTLDEF bool sv_equals(StringView lhs, StringView rhs);
CTLDEF bool sv_equals_cstr(StringView lhs, const char* rhs);

/*
  Compares 2 string views
  returns zero     if lhs == rhs
  returns positive if lhs >  rhs
  returns negative if lhs <  rhs
  First, diff lengths: lhs.len - rhs.len (will be pos if lhs's length is greater)
  Then check buffers byte by byte and if any different byte found in same length,
  return it. Simply: memcmp(lhs, rhs, lhs.len)
*/
CTLDEF int sv_cmp(StringView lhs, StringView rhs);

/*
  Checks if the buffer which the string view looks at starts or ends with "buf"
  bufsz is strlen(buf)
  If bufsz is equal to 0 or greater than view's length, returns false
*/
CTLDEF bool sv_starts_withn(StringView view, const char* buf, size_t bufsz);
CTLDEF bool sv_starts_with(StringView view, const char* buf);
CTLDEF bool sv_ends_withn(StringView view, const char* buf, size_t bufsz);
CTLDEF bool sv_ends_with(StringView view, const char* buf);

/*
  Clears the string view, making length zero
  and pointer to null
*/
CTLDEF void sv_clear(StringView *sv);

/*
  Check if data pointer points to zero and length is also zero
*/
CTLDEF bool sv_isempty(StringView sv);

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define str_foreach(s, name) for(char* name = (s).data; name < (s).data + (s).len; name++)

/*
  Functions that take String context
  as first argument and pointer DO NOT CHECK NULL CONTEXT!
  Like: bool str_reserve(NULL, 10); RESULTS WITH UNDEFINED BEHAVIOR!
  It's your responsibility to check them
*/
/*
  Converts StringView into heap-allocated String
  Allocates memory for String, copies StringView's data and length
*/
#define str_from_sv(sv, ...) \
  str_from_sv_impl(sv, (StringMemory){__VA_ARGS__})
CTLDEF String str_from_sv_impl(const StringView* sv, StringMemory memory);

/*
  Makes and returns string struct from C-strings with length
  If you have len value, use this
  But if you dont have and if you're using strlen, you can use
  str_from_cstr, it does strlen
 */
#define str_newn(buf, len, ...) \
  str_newn_impl(buf, len, (StringMemory){__VA_ARGS__})
CTLDEF String str_newn_impl(const char* buf, size_t len, StringMemory memory);

/*
  str_newn with no len value, it calculates len via strlen
  If you dont have your String's length value or just dont want
  to use strlen by yourself, you can use this
 */
#define str_new(buf, ...) \
  str_new_impl(buf, (StringMemory){__VA_ARGS__})
CTLDEF String str_new_impl(const char* buf, StringMemory memory);

/*
  Reservers needed memory for the String
  It usually multiplies it by 2 if String is too long
  Extra is additional bytes to append
 */
CTLDEF bool str_reserve(String* s, size_t extra);

/*
  Shrinks the memory (cap field) to length + 1
*/
CTLDEF bool str_shrink_to_fit(String* s);

/*
  Returns char in that String by a pos value
  pos stands for position (the index, starts from zero)
 */
CTLDEF char str_idx(const String* s, size_t pos);

/*
  It appends char into that String
  Automatically puts \0 at the end
 */
CTLDEF bool str_append(String* s, char c);

/*
  Combine all c-strings into single heap-allocated string with delimeter.
  You can have const char* or char* arrays use str_join to dispatch
  between them in compile-time.
*/
#define str_join(str, arr, delim) _Generic((arr), \
                                   Array(cchar_ptr)*: str_join_cc, \
                                   Array(char_ptr)*: str_join_c)(str, arr, delim)
CTLDEF bool str_join_cc(String* str, Array(cchar_ptr)* arr, char delim);
CTLDEF bool str_join_c(String* str, Array(char_ptr)* arr, char delim);

/*
  Str_cat but with C-type Strings (char*)
  "bufsz" is length of buf.
*/
CTLDEF bool str_catn(String* s, const char* buf, size_t bufsz);

/*
  Wrapper of str_append_manyn, calls strlen on buf
 */
CTLDEF bool str_cat(String* s, const char* buf);

/*
  It concatenates dest String and src String
  It puts src String's chars into dest String and
  puts \0 at the end and updates count and capacity (if needed)
*/
CTLDEF bool str_cat_str(String* dest, const String* src);

/*
  It gets "data" field in that String
  You can always use s->data or s.data
  But this checks NULL or empty conditions
*/
CTLDEF char* str_to_cstr(const String* s);

/*
  Compares 2 strings their lengths and buffers
  zero     if they're equal (length and buffer)
  positive if lhs > rhs (length or buffer)
  negative if rhs > lhs
*/
CTLDEF int str_cmp(const String* lhs, const String* rhs);

/*
  Calls str_cmp with check if return value is 0 or not
*/
CTLDEF bool str_equals(const String* lhs, const String* rhs);

/*
  Checks if string starts with the buf while bufsz is strlen(buf)
  If bufsz is equal to 0 or greater than view's length, returns false
*/
CTLDEF bool str_starts_withn(const String* str, const char* buf, size_t bufsz);
CTLDEF bool str_starts_with(const String* str, const char* buf);

/*
  Free the String
*/
CTLDEF void str_free(String* s);

/*
  It clears the String, sets len to zero and make data \0
  Capacity still there and this function DOES NOT FREE THE MEMORY!
*/
CTLDEF void str_clear(String* s);

/*
  Trims the whitespaces given string from left or right or both sides
*/
CTLDEF void str_trim(String* s);
CTLDEF void str_trim_left(String* s);
CTLDEF void str_trim_right(String* s);

/*
  Closes the String if not closed or corrupted
  Inserts \0 at the end (depends on count field)
*/
CTLDEF void str_close(String* s);

/*
  Checks if String is ended with \0
*/
CTLDEF bool str_is_closed(const String* s);

/*
  Repeats String with provided count, modifies original String
  It appends that String at the end count - 1 times
  that means if you provide "xx" as String and 4 as count,
  you get: "xxxxxxxx" (=4*"xx")
*/
CTLDEF void str_repeat(String* s, size_t count);

/*
  Formats the String with given format String and variadics
  Acts like running a temporary snprintf and applying it to String
*/
CTLDEF void str_format_into(String* s, const char* fmt, ...);

/*
  Makes all alpha characters in that String lowercased - ASCII only
 */
CTLDEF void str_tolower(String* s);

/*
  Makes all alpha characters in that String uppercased - ASCII only
*/
CTLDEF void str_toupper(String* s);

/*
  Checks if all of chars in that String is alpha - ASCII only
 */
CTLDEF bool str_isalpha(const String* s);

/*
  Checks if all chars in String is alphanumeric - ASCII only
 */
CTLDEF bool str_isalphanum(const String* s);

/*
  Capitalizes String like this:
  hello -> Hello
  heLLo -> Hello
  hell1o -> Hello1o
  Doesn't touch non-alpha chars
  Uppers first char if it is alpha and lowers the rest (alpha ones)
  1hello -> 1hello (because 1 is not alpha)
 */
CTLDEF void str_capitalize(String* s);

#include <stddef.h>
#include <stdbool.h>

#ifdef PLATFORM_POSIX
  #include <sys/stat.h>
  #include <limits.h>
  #include <unistd.h>
  typedef struct stat futil_struct_stat;
  #define PATH_SEP '/'
  #define PATH_SEP_DQ "/"
  #define futil_mkdir(path) (mkdir((path), 0775) == 0)
  #define futil_stat(file_path, st) (stat((file_path), (st)) == 0)
  #define futil_access(file_path, mode) (access((file_path), (mode)) == 0)

#elif defined(PLATFORM_WINDOWS)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <direct.h>
  #include <io.h>
  #include <sys/stat.h>
  #define PATH_SEP '\\'
  #define PATH_SEP_DQ "\\"
  #ifndef PATH_MAX
    #define PATH_MAX MAX_PATH
  #endif
  typedef struct _stat64 futil_struct_stat;
  #define futil_mkdir(path) (_mkdir((path)) == 0)
  #define futil_stat(file_path, st) (_stat64((file_path), (st)) == 0)
  #define futil_access(file_path, mode) (_access((file_path), (mode)) == 0)
#endif

#define concat_path(path, ...) \
  concat_path_impl((path), __VA_ARGS__, NULL)

CTLDEF Result(String) read_entire_file(const char* path);
CTLDEF bool mkdir_if_not_exists(const char* path);
CTLDEF bool is_valid_path(const char* path);
CTLDEF bool concat_path_impl(String* path, const char* first, ...);

CTLDEF size_t count_lines_from_file(const char* file_path);
CTLDEF size_t count_lines_from_str(const String* s);

#include <time.h>
#include <stdbool.h>

/*
  You can define NATIVE_COMPILER in command line while bootstrapping
*/
#ifndef NATIVE_COMPILER
  #define NATIVE_COMPILER NULL /* Detect compiler in runtime */
#endif

typedef struct {
  bool reset;
  bool log;
} CommandRunOptions;

typedef Array(cchar_ptr) CommandBuilder;
#define cmd_push arr_push
#define cmd_free arr_free

#define cmd_run(cmd, ...) \
  cmd_run_impl((cmd), (CommandRunOptions){.reset=true, .log=true, __VA_ARGS__})
CTLDEF bool cmd_run_impl(CommandBuilder* cmd, CommandRunOptions opts);

#define cmd_insta_run(cmd, ...) cmd_insta_run_impl((cmd), __VA_ARGS__, NULL)
bool cmd_insta_run_impl(CommandBuilder* cmd, const char* first, ...);

#define BUIC_REBUILD_URSELF(argc, argv, ...) \
  buic_rebuild_urself((argc), (argv), __FILE__, ##__VA_ARGS__, NULL)

CTLDEF ctl_time_t buic_compare_mtimes(const char* f1, const char* f2);
CTLDEF bool buic_rebuild_urself(int argc, char** argv, const char* file_name, const char* first, ...);
CTLDEF const char* buic_get_native_compiler(void);
CTLDEF String buic_search_path(const char* bin);

#include <stdbool.h>
#include <stddef.h>

#define JSON_TOKEN_TYPES \
  X(OCURLY) /* { */ \
  X(CCURLY) /* } */ \
  X(OBRACKET) /* [ */ \
  X(CBRACKET) /* ] */ \
  X(STRING) \
  X(NUMBER) \
  X(TRUE) \
  X(FALSE) \
  X(NULL) \
  X(COMMA) \
  X(COLON) \
  X(UNKNOWN)

typedef enum {
  JTK_EOF,
  #define X(eenum) \
    JTK_##eenum,
  JSON_TOKEN_TYPES
  #undef X
  _JsonTokenType_count,
} JsonTokenType;

typedef enum {
  JSON_NOVALUE = 0,
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_STRING,
  JSON_NUMBER,
  JSON_TRUE,
  JSON_FALSE,
  JSON_NULL,
  _JsonValueType_count,
} JsonValueType;

typedef struct JsonValue JsonValue;
typedef struct {
  String key;
  JsonValue* value;
} JsonPair;

#ifndef __Array_JsonPair_defined
#define __Array_JsonPair_defined
DECL_ARRAY(JsonPair);
#endif /* __Array_JsonPair_defined */

#ifndef __Array_JsonValue_defined
#define __Array_JsonValue_defined
DECL_ARRAY(JsonValue);
#endif /* __Array_JsonValue_defined */

#ifndef __JsonObject_defined
#define __JsonObject_defined
typedef Array(JsonPair) JsonObject;
#endif /* __JsonObject_defined */

#ifndef __JsonArray_defined
#define __JsonArray_defined
typedef Array(JsonValue) JsonArray;
#endif /* __JsonArray_defined */

struct JsonValue {
  JsonValueType type;
  union {
    String string;
    JsonObject object;
    JsonArray array;
    double number;
    bool boolean;
  } as;
};

typedef struct {
  StringView path;
  size_t row; /* keep track of line count */
  size_t col; /* keep track of column number */
} JsonFile;

typedef struct {
  JsonTokenType type;
  StringView lexeme;
  char ch; /* Used when type is JTK_UNKNOWN */
  JsonFile file; /* Also track the file and we can get better error reporting in parser */
} JsonToken;

/*
  Context structure of JSON Deserializer
*/
typedef struct {
  String content;
  JsonFile file;
  JsonValue value;
  size_t lx_curr; /* Cursor of lexer */
  JsonToken ps_curr; /* Cursor of parser */
} Json;

/* Declaring the Results */
#ifndef __Result_Json_defined
#define __Result_Json_defined
DECL_RESULT(Json);
#endif /* __Result_Json_defined */

#ifndef __Result_JsonValue_defined
#define __Result_JsonValue_defined
DECL_RESULT(JsonValue);
#endif /* __Result_JsonValue_defined */

#ifndef __Result_JsonObject_defined
#define __Result_JsonObject_defined
DECL_RESULT(JsonObject);
#endif /* __Result_JsonObject_defined */

#ifndef __Result_JsonArray_defined
#define __Result_JsonArray_defined
DECL_RESULT(JsonArray);
#endif /* __Result_JsonArray_defined */

CTLDEF Result(Json) json_read(const char* file_path);
CTLDEF void json_free(Json* json);
CTLDEF void json_value_free(JsonValue* value);
CTLDEF bool json_parse(Json* json);
CTLDEF const char* json_tokentype_to_cstr(JsonTokenType type);

/*
  Check if given JsonValue is a specific type
  Simply checks JsonValue.type field.
*/
CTLDEF bool json_is_string(const JsonValue* json_value);
CTLDEF bool json_is_number(const JsonValue* json_value);
CTLDEF bool json_is_boolean(const JsonValue* json_value);
CTLDEF bool json_is_null(const JsonValue* json_value);
CTLDEF bool json_is_object(const JsonValue* json_value);
CTLDEF bool json_is_array(const JsonValue* json_value);

/*
  Extracts specific type from JsonValue
  It checks the type (calls json_is* functions) that's why it returns Result(T)
  There's no "as_null" because null means there's no value preset.
  After checking it return JsonValue.as.T field. (T can be string, number etc.)
*/
CTLDEF Result(String) json_as_string(const JsonValue* json_value);
CTLDEF Result(double) json_as_number(const JsonValue* json_value);
CTLDEF Result(bool) json_as_boolean(const JsonValue* json_value);
CTLDEF Result(JsonObject) json_as_object(const JsonValue* json_value);
CTLDEF Result(JsonArray) json_as_array(const JsonValue* json_value);

/*
  Gets specific type without checking. (Returns JsonValue.as.T)
  That's why it's called "unwrap" (It can lead to UB if there's no check)
*/
CTLDEF String json_unwrap_string(const JsonValue* json_value);
CTLDEF double json_unwrap_number(const JsonValue* json_value);
CTLDEF bool json_unwrap_boolean(const JsonValue* json_value);
CTLDEF JsonObject json_unwrap_object(const JsonValue* json_value);
CTLDEF JsonArray json_unwrap_array(const JsonValue* json_value);

#ifdef CTL_IMPLEMENTATION

// either.c start
#include <assert.h>

const char* err_tostr(ErrorCode err_code) {
  switch(err_code) {
#define X(name, msg) case ERR_##name: return msg;
ERROR_CODES
#undef X
  case ERR_NOERR: return "(no error)";
  default: return NULL;
  }
  assert(false && "unreachable: err_tostr");
}
// either.c end

// array.c start
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool _arr_reserve(ArrayGeneric v, size_t extra) {
  // no need to check UAF
  if (v.header->cap >= SIZE_MAX / 2) return false;
  size_t needed = v.header->len + extra;

  // enough capacity, no need to realloc
  if (v.header->cap >= needed) return true;

  // calculate new capacity
  size_t new_cap = v.header->cap ? v.header->cap : ARR_INITIAL_CAPACITY;
  while (new_cap < needed)
    new_cap += (new_cap >> 1); // roughly multiply by 1.5

  // Use realloc from stdlib.h if no custom realloc function preset.
  arr_reallocator_t realloc_fn = v.header->memory.reallocator;
  if (realloc_fn == NULL) realloc_fn = realloc;
  void* tmp = realloc_fn(*v.items, new_cap * v.elem_size);
  if (!tmp) return false;

  *v.items = tmp;
  v.header->cap = new_cap;
  return true;
}

bool _arr_push(ArrayGeneric v, const void* values, size_t count) {
  if (_arr_isfreed(v) || !_arr_reserve(v, count)) return false;

  memcpy(
    (char*)(*v.items) + v.header->len * v.elem_size,
    values,
    v.elem_size * count
    );
  v.header->len += count;
  return true;
}

bool _arr_pop(ArrayGeneric v, void* out) {
  if (v.header->len == 0) return false; // already checks UAF
  if (out) memcpy(out, _arr_at(v, v.header->len - 1), v.elem_size);
  v.header->len -= 1;
  return true;
}

bool _arr_remove_unord(ArrayGeneric v, size_t idx) {
  if (idx >= v.header->len) return false; // already checks UAF
  memmove(_arr_at(v, idx), _arr_at(v, v.header->len - 1), v.elem_size);
  v.header->len -= 1;
  return true;
}

bool _arr_remove_idx(ArrayGeneric v, size_t idx) {
  if (idx >= v.header->len) return false; // already checks UAF
  memmove(_arr_at(v, idx), _arr_at(v, idx + 1), (v.header->len - idx - 1)*v.elem_size);
  v.header->len -= 1;
  return true;
}

void _arr_free(ArrayGeneric v) {
  // Use free from stdlib.h if no custom free function preset.
  arr_freer_t free_fn = v.header->memory.freer;
  if (free_fn == NULL) free_fn = free;
  free_fn(*v.items); // it'll trigger double free then abortion
  *v.items = NULL;
  v.header->len = 0;
  v.header->cap = 0;
  v.header->is_freed = true;
}

bool _arr_isfreed(ArrayGeneric v) {
  return v.header->is_freed;
}

void* _arr_at(ArrayGeneric v, size_t idx) {
  if (idx >= v.header->len) return NULL; // already checks UAF
  return (char*)(*v.items) + idx * v.elem_size;
}

int _arr_find(ArrayGeneric v, const void* item) {
  for (size_t i = 0; i < v.header->len; i++) { // already checks boundaries and UAF
    if (memcmp(_arr_at(v, i), item, v.elem_size) == 0) return (int)i;
  }
  return -1;
}

bool _arr_equals(ArrayGeneric lhs, ArrayGeneric rhs) {
  if (_arr_isfreed(lhs) || _arr_isfreed(rhs)) return false;
  if (*lhs.items == *rhs.items) return true;
  if (lhs.elem_size != rhs.elem_size
      || lhs.header->len != rhs.header->len) return false;
  return memcmp(*lhs.items, *rhs.items, lhs.header->len * lhs.elem_size) == 0;
}

bool _arr_merge(ArrayGeneric v1, ArrayGeneric v2) {
  assert(v1.elem_size == v2.elem_size);
  if (v2.header->len + v1.header->len >= v1.header->cap)
    _arr_reserve(v1, v2.header->len);
  _arr_push(v1, *v2.items, v2.header->len);
  return true;
}
// array.c end

// buffer.c start
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool _buf_push(BufferGeneric buf, const void* values, size_t count) {
  if (*buf.len + count > buf.nmemb) return false;
  memcpy(buf.items, values, count * buf.elem_size);
  *buf.len += count;
  return true;
}
// buffer.c end

// basic.c start
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

// Temp
static char _basic_temp[BASIC_TEMP_BUFFER_SIZE];
static size_t _basic_temp_len = 0;

char* temp_sprintf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char* result = temp_vsprintf(fmt, args);
  va_end(args);
  return result;
}

char* temp_vsprintf(const char* fmt, va_list args) {
  int n = vsnprintf(_basic_temp, sizeof(_basic_temp), fmt, args);
  if (n < 0) return NULL;
  _basic_temp_len = clamp((size_t)n, 0, sizeof(_basic_temp) - 1);
  return _basic_temp;
}

size_t temp_len() {
  return _basic_temp_len;
}

void temp_reset() {
  _basic_temp[0] = '\0';
  _basic_temp_len = 0;
}

void temp_clear() {
  memset(_basic_temp, 0, sizeof(_basic_temp));
  _basic_temp_len = 0;
}

// Args Parsing
const char* argv_shift(int* argc, char*** argv) {
  if (*argc < 1) return NULL;
  *argc -= 1;
  return *(*argv)++;
}

// Math
#define X(TName, T) \
  T clamp##TName(T a, T lower, T upper) { \
    return a > upper ? upper : a < lower ? lower : a; \
  }
_BASIC_TYPES_WITH_BITS(X)
#undef X

int powii(int base, int exp) {
  int result = 1;
  while (exp > 0) {
    if (exp & 1)
    result *= base;
    base *= base;
    exp >>= 1;
  }
  return result;
}
// basic.c end

// sv.c start

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

StringView sv_from_cstrn_impl(const char* buf, size_t len, StringSlice slice) {
  StringView sv = {0};
  if (slice.start > slice.end || slice.start > len || slice.end > len) return sv;
  sv.data = buf + slice.start;
  sv.len = slice.end - slice.start;
  return sv;
}

StringView sv_from_cstr_impl(const char* buf, StringSlice slice) {
  if (!buf) return (StringView){0};
  size_t buflen = strlen(buf);
  return sv_from_cstrn_impl(buf, buflen, slice);
}

StringView sv_from_str_impl(const String* s, StringSlice slice) {
  return sv_from_cstrn_impl(s->data, s->len, slice);
}

void sv_tostr(StringView sv, char* out) {
  if (!out) return;
  memcpy(out, sv.data, sv.len);
  out[sv.len] = '\0';
}

void sv_clear(StringView *sv) {
  if (sv == NULL) return;
  sv->len = 0;
  sv->data = "";
}

bool sv_isempty(StringView sv) {
  return sv.len == 0 && (!sv.data ? true : *sv.data == 0);
}

int sv_cmp(StringView lhs, StringView rhs) {
  int sdiff = (int)lhs.len - (int)rhs.len;
  if (sdiff != 0) return sdiff; // size diff
  return memcmp(lhs.data, rhs.data, lhs.len);
}

bool sv_starts_withn(StringView view, const char* buf, size_t bufsz) {
  if (view.len == 0) return false;
  if (0 == bufsz || bufsz > view.len) return false;
  return memcmp(view.data, buf, bufsz) == 0;
}

bool sv_starts_with(StringView view, const char* buf) {
  return sv_starts_withn(view, buf, strlen(buf));
}

bool sv_ends_withn(StringView view, const char* buf, size_t bufsz) {
  if (view.len == 0) return false;
  if (0 == bufsz || bufsz > view.len) return false;
  return memcmp(view.data + view.len - bufsz, buf, bufsz) == 0;
}

bool sv_ends_with(StringView view, const char* buf) {
  return sv_ends_withn(view, buf, strlen(buf));
}

bool sv_equals(StringView lhs, StringView rhs) {
  return sv_cmp(lhs, rhs) == 0;
}

bool sv_equals_cstr(StringView lhs, const char* rhs) {
  StringView view = sv(rhs);
  return sv_equals(lhs, view);
}

StringView sv_chop_by_delim(StringView* sv, char delim) {
  StringView chopped = {0};
  const char* found = memchr(sv->data, (uchar_t)delim, sv->len);
  if (!found) return chopped;
  size_t i = (size_t)(found - sv->data);

  chopped.data = sv->data;
  chopped.len = i;

  size_t remaining = sv->len - i - 1;
  sv->len = remaining;
  if (remaining == 0) {
    sv->data = NULL;
  } else {
    sv->data += i + 1;
  }

  return chopped;
}

StringView sv_chop_by_func(StringView* sv, int (*func)(int)) {
  StringView chopped = {0};
  size_t i = 0;
  for (; i < sv->len; i++)
    if (func((uchar_t)sv->data[i])) break;
  if (i == sv->len) return chopped;

  chopped.data = sv->data;
  chopped.len = i;

  size_t remaining = sv->len - i - 1;
  sv->len = remaining;
  if (remaining == 0) {
    sv->data = NULL;
  } else {
    sv->data += i + 1;
  }

  return chopped;
}

StringView sv_chop_left(StringView* sv, size_t n) {
  if (n >= sv->len) return (StringView) {0};
  StringView res = {.data=sv->data, .len=n};
  sv->data += n;
  sv->len -= n;
  return res;
}

StringView sv_chop_right(StringView* sv, size_t n) {
  StringView res = {.data=sv->data + sv->len - n, .len=n};
  sv->len -= n;
  return res;
}

StringView sv_trim_left(StringView sv) {
  while (sv.len > 0 && isspace((uchar_t)sv.data[0])) {
    sv.data++;
    sv.len--;
  }
  return sv;
}

StringView sv_trim_right(StringView sv) {
  while (sv.len > 0 && isspace((uchar_t)sv.data[sv.len - 1])){
    sv.len--;
  }
  return sv;
}

StringView sv_trim(StringView sv) {
  return sv_trim_right(sv_trim_left(sv));
}
// sv.c end

// str.c start

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#ifdef __StringView_defined
String str_from_sv_impl(const StringView* sv, StringMemory memory) {
  return str_newn_impl(sv->data, sv->len, memory);
}
#endif // __StringView_defined

String str_newn_impl(const char* buf, size_t len, StringMemory memory) {
  String s = {.memory=memory};
  size_t cap = len + 16; // this 16 is for pre-allocation
  if (!str_reserve(&s, cap)) return s;
  memcpy(s.data, buf, len);
  s.data[len] = '\0';
  s.len = len;
  return s;
}

String str_new_impl(const char* buf, StringMemory memory) {
  return str_newn_impl(buf, strlen(buf), memory);
}

void str_free(String* s) {
  if (!s->data) return;
  // Use free from stdlib.h if no custom free function preset.
  str_freer_t free_fn = s->memory.freer;
  if (free_fn == NULL) free_fn = free;
  free_fn(s->data);
  s->data = NULL;
  s->len = 0;
  s->cap = 0;
}

void str_clear(String *s) {
  s->len = 0;
  if (s->data) {
    s->data[0] = '\0';
  }
}

bool str_shrink_to_fit(String* s) {
  // Use realloc from stdlib.h if no custom realloc function preset.
  str_reallocator_t realloc_fn = s->memory.reallocator;
  if (realloc_fn == NULL) realloc_fn = realloc;

  size_t new_cap = s->len + 1;
  void* tmp = realloc_fn(s->data, new_cap);
  if (!tmp) return false;
  else s->data = (char*)tmp;
  s->cap = new_cap;
  return true;
}

char str_idx(const String* s, size_t pos) {
  if (!s->data) return '\0';
  if (pos >= s->len) return '\0';
  return s->data[pos];
}

bool str_append(String* s, char c) {
  if (!str_reserve(s, 1)) return false;

  s->data[s->len++] = c;
  s->data[s->len] = '\0';
  return true;
}

bool str_reserve(String* s, size_t extra) {
  size_t sum = extra + s->len + 1;

  // if sum is enough to fit cap, dont update cap
  if (sum <= s->cap) return true;

  // for size_t overflows
  if (sum > SIZE_MAX / 2) return false;

  // sum does NOT fit
  size_t new_cap = sum * 2;
  // Use realloc from stdlib.h if no custom realloc function preset.
  str_reallocator_t realloc_fn = s->memory.reallocator;
  if (realloc_fn == NULL) realloc_fn = realloc;

  char *tmp = (char*)realloc_fn(s->data, new_cap);
  if (!tmp) return false;

  s->data = tmp;
  s->cap = new_cap;
  return true;
}

bool str_join_cc(String* str, Array(cchar_ptr)* arr, char delim) {
  if (delim == '\0') return false;
  size_t len = arr_len(*arr);
  for (size_t i = 0; i < len; i++) {
    if (!str_cat(str, arr->items[i])) return false;
    if (i != len - 1)
      if (!str_append(str, delim)) return false;
  }
  return true;
}

bool str_join_c(String* str, Array(char_ptr)* arr, char delim) {
  Array(cchar_ptr) _arr = arr_cast(cchar_ptr, *arr);
  bool ok = str_join_cc(str, &_arr, delim);
  *arr = arr_cast(char_ptr, _arr);
  return ok;
}

bool str_catn(String* s, const char* buf, size_t bufsz) {
  if (!buf) return false;
  if (bufsz == 0) return true;
  if (!str_reserve(s, bufsz)) return false;

  memcpy(s->data + s->len, buf, bufsz);
  s->len += bufsz;
  s->data[s->len] = '\0';
  return true;
}

bool str_cat(String* s, const char* buf) {
  if (!buf) return false;
  return str_catn(s, buf, strlen(buf));
}

bool str_cat_str(String* s, const String* c) {
  return str_catn(s, c->data, c->len);
}

int str_cmp(const String* lhs, const String* rhs) {
  if (lhs == rhs) return 0;
  int sdiff = (int)lhs->len - (int)rhs->len;
  if (sdiff != 0) return sdiff; // size diff
  return memcmp(lhs->data, rhs->data, lhs->len);
}

bool str_equals(const String* lhs, const String* rhs) {
  return str_cmp(lhs, rhs) == 0;
}

bool str_starts_withn(const String* str, const char* buf, size_t bufsz) {
  if (str->len == 0) return false;
  if (0 == bufsz || bufsz > str->len) return false;
  return memcmp(str->data, buf, bufsz) == 0;
}

bool str_starts_with(const String* str, const char* buf) {
  return str_starts_withn(str, buf, strlen(buf));
}

bool str_ends_withn(const String* str, const char* buf, size_t bufsz) {
  if (str->len == 0) return false;
  if (0 == bufsz || bufsz > str->len) return false;
  return memcmp(str->data + str->len - bufsz, buf, bufsz) == 0;
}

bool str_ends_with(const String* str, const char* buf) {
  return str_ends_withn(str, buf, strlen(buf));
}

void str_trim_left(String* s) {
  // count how many spaces at the begin
  size_t start = 0;
  while (start < s->len && isspace((uchar_t)s->data[start])) {
    start += 1;
  }

  // move String by "start" characters
  if (start != 0) {
    memmove(s->data, s->data + start, s->len - start + 1);
    s->len -= start;
  }
}

void str_trim_right(String* s) {
  while (s->len != 0 && isspace((uchar_t)s->data[s->len - 1])) {
    s->len -= 1;
  }
  s->data[s->len] = '\0';
}

void str_trim(String* s) {
  str_trim_left(s);
  str_trim_right(s);
}

void str_repeat(String* s, size_t multiplier) {
  if (multiplier == 1) return;
  if (multiplier == 0) return;

  size_t new_cnt = s->len * multiplier;
  if (!str_reserve(s, new_cnt - s->len)) return;

  for (size_t i = 1; i < multiplier; ++i) {
    memmove(s->data + (i * s->len), s->data, s->len);
  }

  s->len = new_cnt;
  s->data[new_cnt] = '\0';
}

void str_tolower(String* s) {
  if (!s->data) return;
  for (size_t i = 0; i < s->len; ++i) {
    uchar_t c = (uchar_t)s->data[i];
    uchar_t is_upper = (c >= 'A') & (c <= 'Z');
    s->data[i] += is_upper * 32;
  }
}

void str_toupper(String* s) {
  if (!s->data) return;
  for (size_t i = 0; i < s->len; ++i) {
    uchar_t c = (uchar_t)s->data[i];
    uchar_t is_lower = (c >= 'a') & (c <= 'z');
    s->data[i] -= is_lower * 32;
  }
}

void str_capitalize(String* s) {
  if (!s->data) return;
  for (size_t i = 0; i < s->len; ++i) {
    uchar_t c = (uchar_t)s->data[i];
    if (i == 0) {
      uchar_t is_lower = (c >= 'a') & (c <= 'z');
      s->data[i] -= is_lower * 32;
    } else {
      uchar_t is_upper = (c >= 'A') & (c <= 'Z');
      s->data[i] += is_upper * 32;
    }
  }
}

void str_format_into(String* s, const char* fmt, ...) {
  if (!s->data || s->cap == 0) return;

  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(s->data, s->cap, fmt, ap);
  va_end(ap);

  if (n < 0) {
    s->len = 0;
  } else if ((size_t)n >= s->cap) {
    size_t cap = s->cap;
    if (cap >= 1) {
      s->data[cap - 1] = '\0'; // guarenttes \0
      s->len = cap - 1;
    } else {
      s->len = 0;
    }
  } else {
    s->len = (size_t)n;
  }
}

bool str_isalpha(const String* s) {
  if (!s->data) return false;
  for (size_t i = 0; i < s->len; ++i) {
    uchar_t c = (uchar_t) s->data[i];
    uchar_t is_lower = (c >= 'a') & (c <= 'z');
    uchar_t is_upper = (c >= 'A') & (c <= 'Z');
    // is not alpha mask
    uchar_t mask = !(is_lower | is_upper);
    if (mask) return false;
  }
  return true;
}

bool str_isalphanum(const String* s) {
  if (!s->data) return false;
  for (size_t i = 0; i < s->len; ++i) {
    uchar_t c = (uchar_t) s->data[i];
    uchar_t is_lower = (c >= 'a') & (c <= 'z');
    uchar_t is_upper = (c >= 'A') & (c <= 'Z');
    uchar_t is_num = (c >= '0') & (c <= '9');
    // is not alphanumeric mask
    uchar_t mask = !(is_lower | is_upper | is_num);
    if (mask) return false;
  }
  return true;
}

void str_close(String* s) {
  if (!s->data) return;
  s->data[s->len] = '\0';
}

bool str_is_closed(const String* s) {
  return s->data[s->len] == '\0' ? true : false;
}

char* str_to_cstr(const String* s) {
  return s->data;
}
// str.c end

// futil.c start

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

bool mkdir_if_not_exists(const char* path) {
  if (!is_valid_path(path)) return false;
  for (const char* p = path + 1; *p != '\0'; p++) {
    if (*p == PATH_SEP) {
      size_t i = (size_t)(p - path);
      if (i >= PATH_MAX) return false;
      char buf[PATH_MAX] = {0};
      memcpy(buf, path, i);
      if (!futil_mkdir(buf) && errno != EEXIST) return false;
    }
  }
  return true;
}

bool is_valid_path(const char* path) {
  if (!path || !*path) return false;
  // not fully implemented yet
  return true;
}

bool concat_path_impl(String* path, const char* first, ...) {
  if (!first) return false;
  str_cat(path, first);
  va_list args;
  va_start(args, first);
  const char* arg;
  while ((arg = va_arg(args, const char*)) != NULL) {
    size_t arg_len = strlen(arg);
    if (arg[arg_len - 1] != PATH_SEP) {
      str_append(path, PATH_SEP);
    }
    str_cat(path, arg);
  }
  va_end(args);
  return true;
}

Result(String) read_entire_file(const char* file_path) {
  FILE* f = fopen(file_path, "rb");
  if (!f) return RES_ERR(String, ERR_NO_SUCH_FILE);

  if (fseek(f, 0, SEEK_END) != 0) goto fail_close;
  long size = ftell(f);
  if (size == -1L) goto fail_close;
  if (fseek(f, 0, SEEK_SET) != 0) goto fail_close;

  String s = {0};
  if (!str_reserve(&s, size)) return RES_ERR(String, ERR_MEM_CORRUPT);
  s.len = (size_t)size;
  size_t nread = fread(s.data, 1, size, f);
  if (nread < s.len) goto fail_str;
  if (fclose(f) != 0) goto fail_str;

  return RES_OK(String, s);
fail_str:
  str_free(&s);
fail_close:
  fclose(f);
  return RES_ERR(String, ERR_INTERNAL);
}

size_t count_lines_from_str(const String* s) {
  size_t lines = 0;
  str_foreach(*s, it) {
    if (*it == '\n') lines += 1;
  }
  return lines;
}

size_t count_lines_from_file(const char* file_path) {
  Result(String) res = read_entire_file(file_path);
  if (RES_ISERR(res)) return -1;
  String s = RES_UNWRAP(res);
  size_t lines = count_lines_from_str(&s);
  str_free(&s);
  return lines;
}
// futil.c end

// buic.c start

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef PLATFORM_POSIX
  #include <unistd.h>
  #include <sys/wait.h>
  #include <sys/stat.h>
  #include <time.h>
#elif PLATFORM_WINDOWS
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #error "Your platform is not supported."
#endif

bool cmd_run_impl(CommandBuilder* cmd, CommandRunOptions opts) {
  if (arr_len(*cmd) == 0) return false;

#ifdef PLATFORM_POSIX
  if (opts.log) {
    printf("[BUIC/INFO] Running: ");
    size_t len = arr_len(*cmd);
    for (size_t i = 0; i < len; i++) {
      printf("%s", cmd->items[i]);
      if (i != len - 1) printf(" ");
    }
    printf("\n");
  }

  pid_t pid = fork();
  if (pid == 0) {
    // child
    arr_push(cmd, NULL);
    execvp(cmd->items[0], (char* const*)cmd->items);
    fprintf(stderr, "[BUIC/ERROR] ");
    perror("execvp");
    exit(1);
  } else if (pid > 0) {
    // parent
    int wstatus = 0;
    waitpid(pid, &wstatus, 0);
    if (WIFEXITED(wstatus)) {
      int exit_code = WEXITSTATUS(wstatus);
      if (exit_code != 0) {
        fprintf(stderr, "[BUIC/ERROR] Command failed with code %d\n", exit_code);
        return false;
      }
    }
  } else {
    fprintf(stderr, "[BUIC/ERROR] ");
    perror("fork");
    return false;
  }

#elif defined(PLATFORM_WINDOWS)
  // Since this is a microslop product, you have to suffer
  // Concat the command line string and let the kernel parse again to have performance issues
  String cmdline_str = {0};
  size_t len = arr_len(*cmd);
  for (size_t i = 0; i < len; i++) {
    str_cat(&cmdline_str, cmd->items[i]);
    if (i != len - 1) str_append(&cmdline_str, ' ');
  }

  if (opts.log) {
    printf("[BUIC/INFO] Running: %.*s\n", (int)cmdline_str.len, cmdline_str.data);
  }

  STARTUPINFOA si = {0};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {0};
  BOOL ok = CreateProcessA(NULL, cmdline_str.data, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
  str_free(&cmdline_str);
  if (!ok) {
    fprintf(stderr, "[BUIC/ERROR] CreateProcess failed: %lu\n", GetLastError());
    return false;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 0;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  if (exit_code != 0) {
    fprintf(stderr, "[BUIC/ERROR] Command failed with code %lu\n", exit_code);
    return false;
  }
#endif

  // Reset if needed
  if (opts.reset) {
    arr_len(*cmd) = 0;
  }

  return true;
}

bool cmd_insta_run_impl(CommandBuilder* cmd, const char* first, ...) {
  if (!cmd || !first) return false;
  cmd_push(cmd, first);

  va_list args;
  va_start(args, first);
  const char* arg;
  while ((arg = va_arg(args, const char*)) != NULL) {
    cmd_push(cmd, arg);
  }
  va_end(args);
  return cmd_run(cmd);
}

#define _rebuild_urself_error(fmt, ...) \
  do { \
    fprintf(stderr, "ERROR: Cannot rebuild urself: "fmt": %s\n", ##__VA_ARGS__, strerror(errno)); \
  } while (0)

ctl_time_t buic_compare_mtimes(const char* f1, const char* f2) {
  futil_struct_stat st_f1, st_f2 = {0};
  if (!futil_stat(f1, &st_f1)) {
    _rebuild_urself_error("cannot stat '%s'", f1);
    return false;
  }

  if (!futil_stat(f2, &st_f2)) {
    _rebuild_urself_error("cannot stat '%s'", f2);
    return false;
  }
  return (ctl_time_t)(st_f1.st_mtime - st_f2.st_mtime);
}

bool buic_rebuild_urself(int argc, char** argv, const char* file_name, const char* first, ...) {
  char** orig_argv = argv;
  const char* bin_name = argv_shift(&argc, &argv);
  char old_bin[1024] = {0};
  snprintf(old_bin, sizeof(old_bin), "%s.old", bin_name);
  bool needs_rebuild = false;

  // If old binary exists, take its last modification time and compare it to source file
  if (!futil_access(old_bin, F_OK)) {
    needs_rebuild = true; // no .old binary
  } else {
    // fail-safe over fail-silent policy
    // Even if both script and binary has same last mtime, rebuild anyway.
    // Because in other way, if you for example extract these files from
    // a zip or you are so fast that you edited and instantly run build
    // binary and so their mtimes can be equal. At this point, we'll try
    // to rebuild anyway because one additional build doesn't kill
    needs_rebuild = buic_compare_mtimes(file_name, bin_name) >= 0;
  }

  // Early return if no rebuild needed
  if (!needs_rebuild) return true;
  printf("INFO: Change detected in build script, rebuilding itself.\n");

  // Rename the binary to old one
  printf("INFO: Renaming: '%s' -> '%s'\n", bin_name, old_bin);
  if (rename(bin_name, old_bin) != 0) {
    _rebuild_urself_error("cannot rename '%s'", bin_name);
    return false;
  }

  // Construct and run rebuild command
  CommandBuilder cmd = {0};
  if (NATIVE_COMPILER) cmd_push(&cmd, NATIVE_COMPILER);
  else cmd_push(&cmd, buic_get_native_compiler());
  cmd_push(&cmd, file_name);
  // custom flags if you need
  if (first) {
    cmd_push(&cmd, first);
    va_list args;
    va_start(args, first);
    const char* arg;
    while ((arg = va_arg(args, const char*)) != NULL)
      cmd_push(&cmd, arg);
    va_end(args);
  }
  if (strcmp(cmd.items[0], "cl.exe") == 0 ||
      strcmp(cmd.items[0], "cl") == 0)
    cmd_push(&cmd, temp_sprintf("/Fe:%s", bin_name));
  else cmd_push(&cmd, "-o", bin_name);

  // Run rebuild command
  if (!cmd_run(&cmd)) {
    _rebuild_urself_error("cannot rebuild itself.");
    return false;
  }

  // Run the new binary and exit this old one
  ctl_execvp(bin_name, orig_argv);
  _rebuild_urself_error("cannot run new binary: %s", strerror(errno));
  return false;
}

const char* buic_get_native_compiler(void) {
  const char *env = getenv("CC");
  if (env && *env) return env; // use user's definition if exists

#ifdef PLATFORM_UNIX
  // Search for "cc", "gcc", "clang"
  const char* candidates[] = {"cc","gcc","clang",NULL};
#elif defined(PLATFORM_APPLE)
  const char* candidates[] = {"clang",NULL};
#else
  const char* candidates[] = {"cl.exe","gcc.exe","cc.exe","clang.exe",NULL};
#endif

  for (int i = 0; candidates[i]; i++) {
    String s = buic_search_path(candidates[i]);
    bool found = s.data != NULL;
    str_free(&s);
    if (found) return candidates[i];
  }
  return NULL;
}

String buic_search_path(const char* bin) {
#ifdef PLATFORM_WINDOWS
  char full[PATH_MAX]; // NOTE: bin should ends with .exe
  size_t fullsz = (size_t)SearchPathA(NULL, bin, NULL, PATH_MAX, full, NULL);
  if (fullsz == 0) goto fail;
  return str_newn(full, fullsz);
#else
  const char* path_env = getenv("PATH");
  if (!path_env) goto fail;
  StringView env_sv = sv(path_env);
  StringView dir = sv_chop_by_delim(&env_sv, ':');
  while (dir.len > 0) {
    char full[PATH_MAX];
    int n = snprintf(full, sizeof(full), SV_FMT PATH_SEP_DQ"%s", SV_ARG(dir), bin);
    assert(n >= 0);
    if (futil_access(full, F_OK)) return str_newn(full, (size_t)n);
    dir = sv_chop_by_delim(&env_sv, ':');
  }
#endif
fail:
  return (String){0};
}
// buic.c end

// json.c start

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>

// Lexer functions
static JsonToken jlx_next(Json* json);
static void jlx_skip_whitespace(Json* json);
static JsonToken jlx_read_str(Json* json);
static JsonToken jlx_read_num(Json* json);
static JsonToken jlx_read_key(Json* json);
static char jlx_peek(Json* json);
static char jlx_advance(Json* json);

// Parser functions
static Result(JsonArray) jps_parse_array(Json* json);
static Result(JsonObject) jps_parse_object(Json* json);
static Result(JsonValue) jps_parse_value(Json* json);
static bool jps_expect(Json* json, JsonTokenType expected_type);
static JsonToken jps_advance(Json* json);
static bool jps_interpret_str(Json* json, StringView orig_sv, String* str);

#define JSON_MALLOC malloc
#define JSON_REALLOC realloc
#define JSON_FREE free

#define JSON_STR_MEMORY_MANAGER \
  (StringMemory){.allocator=JSON_MALLOC, .freer=JSON_FREE, .reallocator=JSON_REALLOC}

#define JSON_STR_MEMORY_MANAGER_FLAT \
  .allocator=JSON_MALLOC, .freer=JSON_FREE, .reallocator=JSON_REALLOC

#define JSON_ARR_MEMORY_MANAGER \
  (ArrayMemory){.allocator=JSON_MALLOC, .freer=JSON_FREE, .reallocator=JSON_REALLOC}

#define jlx_report_error(json, fmt, ...) \
  do { \
    fprintf(stderr, SV_FMT":%zu:%zu: ERROR: "fmt"\n", \
      SV_ARG((json)->file.path), (json)->file.row + 1, (json)->file.col, ##__VA_ARGS__); \
  } while (0)

#define jps_report_error(token, fmt, ...) \
  do { \
    fprintf(stderr, SV_FMT":%zu:%zu: ERROR: "fmt"\n", \
      SV_ARG((token).file.path), (token).file.row + 1, (token).file.col, ##__VA_ARGS__); \
  } while (0)

#define jps_report_error_exp(token, expected) \
  do { \
    if ((token).ch != '\0') { \
      jps_report_error((token), "Expected %s but got '%c'", \
        (expected), (token).ch); \
    } else { \
      jps_report_error((token), "Expected %s but got '"SV_FMT"'", \
        (expected), SV_ARG((token).lexeme)); \
    } \
  } while (0)

Result(Json) json_read(const char* file_path) {
  if (!file_path) return RES_ERR(Json, ERR_INVALID_ARG);
  Result(String) readen_res = read_entire_file(file_path);
  if (RES_ISERR(readen_res)) return RES_ERR(Json, readen_res.as.right.code);
  String content = readen_res.as.left;

  JsonFile file = {.path = sv(file_path)};
  Json json = {
    .content = content,
    .file = file,
  };
  return RES_OK(Json, json);
}

void json_value_free(JsonValue* value) {
  switch (value->type) {
  case JSON_OBJECT: {
    JsonObject obj = value->as.object;
    arr_foreach(obj, it) {
      // typeof(*it) == JsonPair;
      json_value_free(it->value);
      JSON_FREE(it->value);
    }
    arr_free(&obj);
  } break;
  case JSON_ARRAY: {
    JsonArray arr = value->as.array;
    arr_foreach(arr, it) {
      json_value_free(it);
    }
    arr_free(&arr);
  } break;
  case JSON_STRING: {
    str_free(&value->as.string);
  }; break;
  default: break;
  }
}

void json_free(Json* json) {
  if (!json) return;
  json_value_free(&json->value);
  str_free(&json->content);
  *json = (Json){0};
}

const char* json_tokentype_to_cstr(JsonTokenType type) {
  switch (type) {
  case JTK_EOF: return "JTK_EOF";
  #define X(eenum) \
    case JTK_##eenum: return "JTK_" STRINGIFY(eenum);
  JSON_TOKEN_TYPES
  #undef X
  default: return NULL;
  }
  UNREACHABLE("json_tokentype_to_cstr");
}

bool json_parse(Json* json) {
  if (!json) return false;
  json->ps_curr = jlx_next(json);
  Result(JsonValue) val_res = jps_parse_value(json);
  if (RES_ISERR(val_res)) return false;
  if (json->ps_curr.type != JTK_EOF) return false;
  json->value = RES_UNWRAP(val_res);
  return true;
}

void json_dump(Json* json) {
  while (1) {
    JsonToken tok = jlx_next(json);
    if (tok.type == JTK_EOF) break;
    printf("JsonToken{type=%s, lexeme=\""SV_FMT"\", ROW=%zu, COL=%zu}\n",
      json_tokentype_to_cstr(tok.type), SV_ARG(tok.lexeme), tok.file.row, tok.file.col);
  }
  json->lx_curr = 0;
}

// Lexer
static JsonToken jlx_next(Json* json) {
  jlx_skip_whitespace(json);
  char c = jlx_advance(json);

  switch (c) {
    case '{': return (JsonToken){.type = JTK_OCURLY, .lexeme=sv("{"), .file=json->file};
    case '}': return (JsonToken){.type = JTK_CCURLY, .lexeme=sv("}"), .file=json->file};
    case '[': return (JsonToken){.type = JTK_OBRACKET, .lexeme=sv("["), .file=json->file};
    case ']': return (JsonToken){.type = JTK_CBRACKET, .lexeme=sv("]"), .file=json->file};
    case ':': return (JsonToken){.type = JTK_COLON, .lexeme=sv(":"), .file=json->file};
    case ',': return (JsonToken){.type = JTK_COMMA, .lexeme=sv(","), .file=json->file};
    case '"': return jlx_read_str(json);
    case '\0': return (JsonToken){.lexeme=sv("<eof>"), .file=json->file};
  }

  bool is_num_related = c == '-' || c == '.';

  if (isalpha((uchar_t)c) && !is_num_related) {
    return jlx_read_key(json);
  }

  if (isdigit((uchar_t)c) || is_num_related) {
    return jlx_read_num(json);
  }

  jlx_report_error(json, "Unexpected token: '%c'", c);
  return (JsonToken){.type=JTK_UNKNOWN, .ch=c, .file=json->file};
}

static JsonToken jlx_read_str(Json* json) {
  char c = jlx_peek(json);
  size_t i = 0;
  while (c != '"') {
    bool is_escape = c == '\\';
    if (is_escape) jlx_advance(json);
    jlx_advance(json);
    c = jlx_peek(json);
    if (is_escape) i += 2;
    else i += 1;
  }
  StringView view = svs(&json->content, .start=json->lx_curr - i, .end=json->lx_curr);
  jlx_advance(json);
  return (JsonToken){.type=JTK_STRING, .lexeme=view, .file=json->file};
}

static JsonToken jlx_read_num(Json* json) {
  char c = jlx_peek(json);
  size_t i = 1;
  while (isdigit((uchar_t)c) || c == 'e' || c == 'E' || c == '.' || c == '-') {
    jlx_advance(json);
    c = jlx_peek(json);
    i++;
  }

  StringView view = svs(&json->content, .start=json->lx_curr - i, .end=json->lx_curr);
  return (JsonToken){.type=JTK_NUMBER, .lexeme=view, .file=json->file};
}

static JsonToken jlx_read_key(Json* json) {
  char c = jlx_peek(json);
  size_t i = 1;
  while (isalnum((uchar_t)c)) {
    jlx_advance(json);
    c = jlx_peek(json);
    i++;
  }

  StringView view = svs(&json->content, .start=json->lx_curr - i, .end=json->lx_curr);
  if (sv_equals_cstr(view, "true")) return (JsonToken){.type=JTK_TRUE, .file=json->file};
  if (sv_equals_cstr(view, "false")) return (JsonToken){.type=JTK_FALSE, .file=json->file};
  if (sv_equals_cstr(view, "null")) return (JsonToken){.type=JTK_NULL, .file=json->file};

  return (JsonToken){.type=JTK_UNKNOWN, .lexeme=view, .file=json->file};
}

// Lexer helper functions
static char jlx_peek(Json* json) {
  String str = json->content;
  if (json->lx_curr >= str.len) return '\0';
  return str.data[clamp(str.len, 0, json->lx_curr)];
}

static char jlx_advance(Json* json) {
  char c = jlx_peek(json);
  if (c != '\0') json->lx_curr += 1;
  if (c == '\n') {
    json->file.row += 1;
    json->file.col = 0;
  } else json->file.col += 1;
  return c;
}

static void jlx_skip_whitespace(Json* json) {
  for (;isspace((uchar_t)jlx_peek(json)); jlx_advance(json))
    ;;
}

// Parser
static Result(JsonArray) jps_parse_array(Json* json) {
  JsonArray arr = {.h.memory=JSON_ARR_MEMORY_MANAGER};
  assert(json->ps_curr.type == JTK_OBRACKET &&
        "Cursor of parser doesn't look at open bracket ('['), maybe called from outside?");
  jps_advance(json);

  for (size_t idx = 1; true; idx++) {
    // Parse value
    Result(JsonValue) value_res = jps_parse_value(json);
    if (RES_ISERR(value_res)) goto fail;
    JsonValue value = RES_UNWRAP(value_res);
    if (value.type == JSON_NOVALUE) {
      jps_report_error(json->ps_curr, "Error while parsing %zu%s value!",
        idx, (idx == 1 ? "st" : idx == 2 ? "nd" : idx == 3 ? "rd" : "th"));
      goto fail;
    }
    arr_push(&arr, value);

    // Handle ending or commas
    if (!jps_expect(json, JTK_COMMA)) {
      if (jps_expect(json, JTK_CBRACKET)) break;
      jps_report_error_exp(json->ps_curr, "',' or ']'");
      goto fail;
    }
  }

  return RES_OK(JsonArray, arr);
fail:
  return RES_ERR(JsonArray, ERR_INTERNAL);
}

static Result(JsonObject) jps_parse_object(Json* json) {
  JsonObject obj = {.h.memory=JSON_ARR_MEMORY_MANAGER};
  assert(json->ps_curr.type == JTK_OCURLY &&
         "Cursor of parser doesn't look at open curly brace ('{'), maybe called from outside?");
  jps_advance(json);

  while (1) {
    // STRING, COLON, VALUE
    StringView key_sv = json->ps_curr.lexeme;
    String str = {.memory=JSON_STR_MEMORY_MANAGER};
    JsonPair pair = {0};
    if (json->ps_curr.type != JTK_STRING) {
      jps_report_error_exp(json->ps_curr, "string");
      goto fail;
    }
    if (!jps_interpret_str(json, key_sv, &str)) goto fail;
    pair.key = str;
    jps_advance(json);

    // Expect :
    if (!jps_expect(json, JTK_COLON)) {
      jps_report_error_exp(json->ps_curr, "':'");
      goto fail;
    }

    // Parse value
    Result(JsonValue) value_res = jps_parse_value(json);
    if (RES_ISERR(value_res)) goto fail;
    JsonValue* value = JSON_MALLOC(sizeof(JsonValue));
    *value = RES_UNWRAP(value_res);
    if (value->type == JSON_NOVALUE) {
      jps_report_error(json->ps_curr, "Error while parsing value of `"SV_FMT"`", SV_ARG(key_sv));
      goto fail;
    }
    pair.value = value;
    arr_push(&obj, pair);

    // Handle ending or commas
    if (!jps_expect(json, JTK_COMMA)) {
      if (jps_expect(json, JTK_CCURLY)) break;
      jps_report_error_exp(json->ps_curr, "',' or '}'");
      goto fail;
    }
  }

  return RES_OK(JsonObject, obj);
fail:
  return RES_ERR(JsonObject, ERR_INTERNAL);
}

Result(JsonValue) jps_parse_value(Json* json) {
  JsonToken tok = json->ps_curr;
  JsonValue val = {0};

  switch (tok.type) {
  case JTK_STRING: {
    String str = {0};
    if (!jps_interpret_str(json, tok.lexeme, &str)) goto fail;
    val.type = JSON_STRING;
    val.as.string = str;
  } break;
  case JTK_NUMBER: {
    // TODO: Develop this (fix leading zeros, dangling dots)
    // These are passing: (should not pass)
    // 013
    // 13.
    // 13.e
    // 13e1.2
    val.type = JSON_NUMBER;
    double result = 0.0;
    StringView view = tok.lexeme;
    assert(view.len >= 1);
    int sign = 1;
    int exp = 0, exp_sign = 1;
    int scale = 1;
    int state = 0; // 0 int, 1 frac, 2 exp
    double value = 0.0;

    for (int i = 0; i < (int)view.len; i++) {
      char c = view.data[i];
      if (c == 'e' || c == 'E') {
        if (state == 2) {
          jps_report_error(tok, "Duplication of '%c' symbol.", c);
          goto fail;
        }
        state = 2;
        continue;
      } else if (c == '.') {
        if (state == 1) {
          jps_report_error(tok, "Duplication of '%c' symbol.", c);
          goto fail;
        }
        state = 1;
        continue;
      } else if (c == '-') {
        if (i == 0) {
          sign = -1;
          continue;
        }
        char back = view.data[i - 1];
        if (state == 2) {
          if (back == 'E' || back == 'e') exp_sign = -1;
          else {
            jps_report_error(tok, "Invalid location of '-' in exponent.");
            goto fail;
          }
        } else {
          jps_report_error(tok, "Invalid location of '-' in number.");
          goto fail;
        }
      } else if ('0' <= c && c <= '9') {
        int n = c - '0';
        if (state == 2) {
          exp = exp * 10 + n;
          continue;
        }
        if (state == 1) scale *= 10;
        value = value * 10 + n;
      } else {
        jps_report_error(tok, "Unexpected symbol '%c' in number format.", c);
        goto fail;
      }
    }

    result = (value / scale) * powii(10, exp * exp_sign) * sign;
    val.as.number = result;
  } break;
  case JTK_TRUE: {val.type = JSON_TRUE; val.as.boolean = true;} break;
  case JTK_FALSE: {val.type = JSON_FALSE; val.as.boolean = false;} break;
  case JTK_NULL: {val.type = JSON_NULL;} break;
  case JTK_OCURLY: {
    Result(JsonObject) obj_res = jps_parse_object(json);
    if (RES_ISERR(obj_res)) goto fail;
    val.type = JSON_OBJECT;
    val.as.object = RES_UNWRAP(obj_res);
  } break;
  case JTK_OBRACKET: {
    Result(JsonArray) arr_res = jps_parse_array(json);
    if (RES_ISERR(arr_res)) goto fail;
    val.type = JSON_ARRAY;
    val.as.array = RES_UNWRAP(arr_res);
  } break;
  case JTK_UNKNOWN: {
    val.type = JSON_NOVALUE;
    jps_report_error_exp(tok, "'true', 'false' or 'null'");
  } break;
  default: jps_report_error(tok, "Unexpected end of file"); goto fail;
  }

  if (tok.type != JTK_OCURLY && tok.type != JTK_OBRACKET)
    jps_advance(json);
  return RES_OK(JsonValue, val);
fail:
  return RES_ERR(JsonValue, ERR_INTERNAL);
}

// Parser helper functions
static bool jps_expect(Json* json, JsonTokenType expected_type) {
  if (json->ps_curr.type != expected_type) return false;
  jps_advance(json);
  return true;
}

static JsonToken jps_advance(Json* json) {
  JsonToken tok = jlx_next(json);
  json->ps_curr = tok;
  return tok;
}

static bool jps_interpret_str(Json* json, StringView orig_sv, String* str) {
  JsonToken tok = json->ps_curr;
  *str = (String){.memory=JSON_STR_MEMORY_MANAGER};
  for (size_t i = 0; i < orig_sv.len; i++) {
    char ch = orig_sv.data[i];
    if (ch != ' ' && isspace((uchar_t)ch)) {
      jps_report_error(tok, "Invalid space character in a string: '%c'", ch);
      return false;
    }
    if (ch == '\\') {
      i++;
      ch = orig_sv.data[i];
      switch (ch) {
      case 'n': str_append(str, '\n'); break;
      case 'f': str_append(str, '\f'); break;
      case 't': str_append(str, '\t'); break;
      case 'r': str_append(str, '\r'); break;
      case 'b': str_append(str, '\b'); break;
      case '\\': str_append(str, '\\'); break;
      default: jps_report_error(tok, "Invalid escape character: '\\%c'", ch); return false;
      }
    } else {
      str_append(str, ch);
    }
  }
  return true;
}

// String
bool json_is_string(const JsonValue* json_value) {
  return json_value->type == JSON_STRING;
}
Result(String) json_as_string(const JsonValue* json_value) {
  if (!json_is_string(json_value)) return RES_ERR(String, ERR_INVALID_ARG);
  return RES_OK(String, json_value->as.string);
}
String json_unwrap_string(const JsonValue* json_value) {
  return json_value->as.string;
}

// Number
bool json_is_number(const JsonValue* json_value) {
  return json_value->type == JSON_NUMBER;
}
Result(double) json_as_number(const JsonValue* json_value) {
  if (!json_is_number(json_value)) return RES_ERR(double, ERR_INVALID_ARG);
  return RES_OK(double, json_value->as.number);
}
double json_unwrap_number(const JsonValue* json_value) {
  return json_value->as.number;
}

// Boolean
bool json_is_boolean(const JsonValue* json_value) {
  return json_value->type == JSON_TRUE || json_value->type == JSON_FALSE;
}
Result(bool) json_as_boolean(const JsonValue* json_value) {
  if (!json_is_boolean(json_value)) return RES_ERR(bool, ERR_INVALID_ARG);
  return RES_OK(bool, json_value->as.boolean);
}
bool json_unwrap_boolean(const JsonValue* json_value) {
  return json_value->as.boolean;
}

// Object
bool json_is_object(const JsonValue* json_value) {
  return json_value->type == JSON_OBJECT;
}
Result(JsonObject) json_as_object(const JsonValue* json_value) {
  if (!json_is_object(json_value)) return RES_ERR(JsonObject, ERR_INVALID_ARG);
  return RES_OK(JsonObject, json_value->as.object);
}
JsonObject json_unwrap_object(const JsonValue* json_value) {
  return json_value->as.object;
}

// Array
bool json_is_array(const JsonValue* json_value) {
  return json_value->type == JSON_ARRAY;
}
Result(JsonArray) json_as_array(const JsonValue* json_value) {
  if (!json_is_array(json_value)) return RES_ERR(JsonArray, ERR_INVALID_ARG);
  return RES_OK(JsonArray, json_value->as.array);
}
JsonArray json_unwrap_array(const JsonValue* json_value) {
  return json_value->as.array;
}

// Null (type)
bool json_is_null(const JsonValue* json_value) {
  return json_value->type == JSON_NULL;
}
// json.c end

#endif /* CTL_IMPLEMENTATION */
#endif /* CTL_H */
