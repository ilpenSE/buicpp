#include <icl/os>
#include <icl/print>
#include <cstring>
#include <cerrno>

namespace icl::os {
#ifdef PLATFORM_WINDOWS
// dirent.h
[[noreturn]] DIR *opendir(const char *dirname) {
  (void)dirname;
  TODO("opendir");
}

[[noreturn]] struct dirent *readdir(DIR *dirp) {
  (void)dirp;
  TODO("readdir");
}

[[noreturn]] int closedir(DIR *dirp) {
  (void)dirp;
  TODO("closedir");
}

// sys/stat.h
int lstat(const char *path, struct stat *st) { return win32::stat(path, st, false); }
int stat(const char *path, struct stat *st) { return win32::stat(path, st, true); }

// unistd.h
[[noreturn]] ssize_t readlink(const char *path, char *buf, size_t bufsz) {
  (void)(path); (void)(buf); (void)(bufsz);
  TODO("readlink");
}

[[noreturn]] int access(const char *path, mode_t mode) {
  (void)(path); (void)(mode);
  TODO("access");
}

[[noreturn]] int mkdir(const char *path, mode_t mode) {
  (void)path; (void)mode;
  TODO("mkdir");
}

namespace win32 {
[[noreturn]] int stat(const char *path, struct stat *st, bool follow_symlink) {
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

#ifdef PLATFORM_WINDOWS
Process::Process(const CommandBuilder& cmd) : m_state(ProcessState::CONSTRUCTED)
{
  for (const auto& elem : cmd) {
    std::wstring welem = win32::to_wstring(elem);
    win32::cmdline_escape_if_needed(welem);
    m_cmdline += welem; m_cmdline += L' ';
  }
}
#endif // PLATFORM_WINDOWS

#ifdef PLATFORM_POSIX
Process::Process(const CommandBuilder& cmd) : m_argv(cmd.count() + 1), m_state(ProcessState::CONSTRUCTED)
{
  for (const auto& item : cmd) m_argv.push(item.c_str());
}
#endif // PLATFORM_POSIX

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
} // namespace icl::os
