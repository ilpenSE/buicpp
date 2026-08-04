#include <icl/build>
#include <icl/print>
#include <icl/os>
#include <cstring>

namespace icl {
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

bool _vrebuild_urself(int argc, char** argv, const char* file_name, const char *first, ...)
{
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

  if (first != nullptr) {
    cmd.push(first);
    va_list args; va_start(args, first);
    while (const char *arg = va_arg(args, const char *)) {
      cmd.push(arg);
    }
    va_end(args);
  }
  cmd.push_many(comp.second == Compiler::MSVC ? "/Fe" : "-o", bin_name);

  // Run rebuild command
  if (!cmd.run()) {
    eprintln("ERROR: cannot rebuild itself.");
    return false;
  }

  // Run the new binary and exit this old one
  os::Process proc(bin_name, argc, argv);
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
  case Compiler::MSVC: return "cl";
  default: return "<invalid>";
  }
  UNREACHABLE("const char* to_string(icl::Compiler e)");
}

Compiler compiler_from_cstr(const char* str) {
  if (strcmp(str, "g++") == 0) return Compiler::GCC;
  if (strcmp(str, "clang++") == 0) return Compiler::CLANG;
  if (strcmp(str, "icpx") == 0) return Compiler::INTEL_LLVM;
  if (strcmp(str, "icpc") == 0) return Compiler::INTEL_CLASSIC;
  if (strcmp(str, "cl") == 0) return Compiler::MSVC;
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
  #elif defined(_MSC_VER)
    return {"cl", Compiler::MSVC};
  #else
    #error "Unknown compiler, define NATIVE_COMPILER manually"
  #endif
  UNREACHABLE("std::pair<const char*, icl::Compiler> get_native_compiler()");
#endif
}
} // namespace icl

#if __cplusplus >= 202002L
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
    std::wstring welem = icl::os::win32::to_wstring(elem);
    icl::os::win32::cmdline_escape_if_needed(welem);
    result += icl::os::win32::to_string(welem);
    #endif

    if (i != cmd.count() - 1) result += " ";
  }
  return std::formatter<std::string>::format(result, ctx);
}
#endif // __cplusplus >= 202002L
