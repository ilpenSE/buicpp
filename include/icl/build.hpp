/**
 * @file build.hpp
 * @author ilpeN
 * @since 1.0.0
*/
#ifndef ICL_BUILD_HPP
#define ICL_BUILD_HPP

#include "base.hpp"
#include "array.hpp"
#include "os.hpp"
#include <utility>
#include <cstdio>

namespace icl {
inline const char* shift(int& argc, char**& argv) {
  if (argc == 0) return nullptr;
  argc--; return *argv++;
}

enum class Compiler {
  UNKNOWN = 0, GCC, CLANG, MSVC, INTEL_LLVM, INTEL_CLASSIC, Count
};

extern const char* to_string(Compiler e);
extern Compiler compiler_from_cstr(const char* str);
extern std::pair<const char*, Compiler> get_native_compiler();

#define REBUILD_URSELF(argc, argv, ...) \
  icl::_rebuild_urself((argc), (argv), __FILE__, ##__VA_ARGS__)

extern bool _vrebuild_urself(int argc, char** argv, const char* file_name, const char *first, ...);

template <
  typename... Args,
  typename = std::enable_if_t<(std::is_convertible_v<Args, const char *> && ...)>
>
inline bool _rebuild_urself(int argc, char** argv, const char* file_name, Args&&... args)
{
  return _vrebuild_urself(argc, argv, file_name, std::forward<Args>(args)..., nullptr);
}

} // namespace icl

#endif // ICL_BUILD_HPP
