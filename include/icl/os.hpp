/**
 * @file os.hpp
 * @author ilpeN
 * @since 1.0.0
 * @brief Thin, cross-platform wrappers around OS-level primitives
 * @details
 * Provides POSIX-style type aliases (mode_t, stat, dirent, DIR, ...)
 * and functions (opendir, stat, access, mkdir, readlink) that work
 * the same way whether you compile on Linux/macOS (where they are
 * just aliases for the real POSIX types/functions) or on Windows
 * (where this file implements them on top of the Win32 API).
 * Also provides icl::os::Process, a small abstraction for spawning
 * and waiting on child processes.
*/
#ifndef ICL_OS_HPP
#define ICL_OS_HPP

#include "base.hpp"
#include "array.hpp"

#ifdef PLATFORM_WINDOWS
#ifndef PATH_MAX
  #define PATH_MAX MAX_PATH
#endif // PATH_MAX
#include <windows.h>
#else // PLATFORM_POSIX
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <time.h>
#include <dirent.h>
#endif // PLATFORM_WINDOWS

namespace icl {
struct CmdRunOptions {
  bool is_log = true;
  bool is_reset = true;
  // FILE *fstdout = nullptr;
  // FILE *fstderr = nullptr;
  // FILE *fstdin = nullptr;
};

class CommandBuilder : public ArrayList<std::string> {
public:
  using ArrayList<std::string>::ArrayList;
  bool run(CmdRunOptions opts = {});
};
}

#if __cplusplus >= 202002L
#include <format>
template <>
struct std::formatter<icl::CommandBuilder> : std::formatter<std::string> {
  auto format(const icl::CommandBuilder& cmd, std::format_context& ctx) const -> decltype(ctx.out());
};
#endif

/**
 * @brief os namespace
 * @details
 * Assumes that you have x86_64/ARM64 CPU (64-bits)
*/
namespace icl::os {
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
  extern int lstat(const char *path, struct stat *st);
  extern int stat(const char *path, struct stat *st);

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
  [[noreturn]] extern ssize_t readlink(const char *path, char *buf, size_t bufsz);
  [[noreturn]] extern int access(const char *path, mode_t mode);
  [[noreturn]] extern int mkdir(const char *path, mode_t mode);

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
  [[noreturn]] extern DIR *opendir(const char *dirname);
  [[noreturn]] extern struct dirent *readdir(DIR *dirp);
  [[noreturn]] extern int closedir(DIR *dirp);

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

/**
 * @brief Portable signal names, used with Process::signal()
 * @details
 * These map to the closest matching real signal/API call on each
 * platform (see to_posix() below for the POSIX mapping).
*/
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

namespace win32 {
#ifdef PLATFORM_WINDOWS
extern int last_error_to_errno();
[[noreturn]] extern int stat(const char *path, struct stat *st, bool follow_symlink);

extern int wlen_from_cstr(const std::string& str);
extern int wlen_from_cstr(const char *buffer);
extern int wlen_from_cstr(const char *buffer, int size);

extern int len_from_wstr(const std::wstring& wstr);
extern int len_from_wstr(const wchar_t *wbuffer);
extern int len_from_wstr(const wchar_t *wbuffer, int wsize);

extern bool wide_to_utf8(const wchar_t* wbuffer, int wsize, char* buffer, int size);
extern bool utf8_to_wide(const char* buffer, int size, wchar_t* wbuffer, int wsize);
extern std::wstring to_wstring(const std::string& str);
extern std::string to_string(const std::wstring& str);

extern void cmdline_escape_if_needed(std::wstring& wstr);
#endif // PLATFORM_WINDOWS
} // namespace win32

/**
 * @brief The lifecycle state of a Process
 * @details
 * A Process moves through these states in order: CONSTRUCTED (built
 * but not started) -> RUNNING (after spawn()) -> REAPED (after wait()
 * or signal()). INVALID means something went wrong (for example, the
 * constructor failed) and the Process should not be used.
*/
enum class ProcessState { INVALID, CONSTRUCTED, RUNNING, REAPED, Count };

/**
 * @class Process
 * @since 1.0.0
 * @brief Spawns and waits on a child process, cross-platform
 * @details
 * A small wrapper around fork()+execvp() on POSIX and CreateProcessW()
 * on Windows. Build a Process with the command you want to run, call
 * spawn() to start it, then wait() to block until it finishes and get
 * its exit code. You can also send it a signal with signal().
 *
 * @code{.cpp}
 * icl::os::Process proc("ls", "-la", "/tmp");
 * if (proc.state() != icl::os::ProcessState::CONSTRUCTED) {
 *   icl::eprintln("failed to construct process");
 * } else if (proc.spawn()) {
 *   int exit_code = proc.wait();
 *   icl::println("ls exited with code {}", exit_code);
 * }
 * @endcode
 * @todo Add stdin/stdout/stderr redirecting
*/
class Process {

public:
  /**
   * @brief Build a Process for the command you want to run
   * @details
   * Three ways to build one, all equivalent:
   * - Process(const char *file, const char *args...), variadic argument list
   * - Process(const char *file, int argc, char **argv), reuse an existing argv array (handy
   *   for forwarding this program's own argv when rebuilding itself)
   * - Process(const CommandBuilder& cmd), build the command with icl::CommandBuilder
   *   and hand it over as one piece
   *
   * The process is not started yet after construction — call spawn()
   * for that. Check state() to make sure construction actually
   * succeeded (it can fail on Windows, where argument conversion can
   * fail if the arguments are not valid UTF-8).
  */
#ifdef PLATFORM_POSIX
  template <typename... Args,
    typename = std::enable_if_t<(std::is_convertible_v<Args, const char *> && ...)>>
  Process(const char *file, Args... args) : m_argv(sizeof...(Args) + 2), m_state(ProcessState::CONSTRUCTED)
  { m_argv.push_many(file, args...); }

  Process(const char *file, int argc, char **argv) : m_argv(argc + 2), m_state(ProcessState::CONSTRUCTED)
  { m_argv.push(file); for (int i = 0; i < argc; i++) m_argv.push(argv[i]); }

  Process(const CommandBuilder& cmd);

#else // PLATFORM_WINDOWS
  template <typename... Args,
    typename = std::enable_if_t<(std::is_convertible_v<Args, const char *> && ...)>>
  Process(const char *file, Args... args) : m_state(ProcessState::CONSTRUCTED)
  {
    std::wstring wfile = win32::to_wstring(file);
    win32::cmdline_escape_if_needed(wfile);
    m_cmdline += wfile;
    m_cmdline += L' ';

    ([&] {
      std::wstring warg = win32::to_wstring(args);
      win32::cmdline_escape_if_needed(warg);
      m_cmdline += warg;
      m_cmdline += L' ';
    }(), ...);
  }

  Process(const char *file, int argc, char **argv) : m_state(ProcessState::CONSTRUCTED)
  {
    std::wstring wfile = win32::to_wstring(file);
    win32::cmdline_escape_if_needed(wfile);
    m_cmdline += wfile;
    m_cmdline += L' ';

    for (int i = 0; i < argc; i++) {
      std::wstring warg = win32::to_wstring(argv[i]);
      win32::cmdline_escape_if_needed(warg);
      m_cmdline += warg;
      m_cmdline += L' ';
    }
  }

  Process(const CommandBuilder& cmd);
#endif

  ~Process() {
#ifdef PLATFORM_WINDOWS
    if (m_hProcess) CloseHandle(m_hProcess);
#endif
    if (m_state == ProcessState::RUNNING) wait();
    m_state = ProcessState::INVALID;
  }

  /**
   * @function os::ProcessState os::Process::state()
   * @brief Current lifecycle state of this process, see ProcessState
  */
  ProcessState state() { return m_state; }

  /**
   * @function int os::Process::spawn()
   * @brief Actually starts the process
   * @details
   * Only works when state() is CONSTRUCTED or REAPED. Returns false
   * (and sets errno) if starting the process failed, for example when
   * the executable was not found.
   * @return true if the process started successfully
  */
  bool spawn();

  /**
   * @function int os::Process::signal(os::SignalType signal_type)
   * @brief Sends a signal to the running process
   * @details
   * Only works when state() is RUNNING. After a successful call, the
   * process is considered REAPED — you don't need to call wait()
   * afterwards.
   * @param signal_type which signal to send, see SignalType
   * @return true if the signal was sent successfully
  */
  bool signal(SignalType signal_type);

  /**
   * @function int os::Process::wait()
   * @brief Blocks until the process finishes, then returns its exit code
   * @details
   * Only works when state() is RUNNING. After this call, the process
   * is REAPED.
   * @return the process's exit code, or -1 on error
  */
  int wait();

private:
#ifdef PLATFORM_WINDOWS
  std::wstring m_cmdline;
  HANDLE m_hProcess = nullptr;
#else
  pid_t m_pid = 0;
  ArrayList<const char *> m_argv{};
#endif
  int m_exit_code = 0;
  ProcessState m_state = ProcessState::INVALID;
}; // class Process
} // namespace icl::os

#endif // ICL_OS_HPP
