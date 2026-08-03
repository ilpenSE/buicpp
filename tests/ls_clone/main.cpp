// Basic ls clone
// Shows last modification date, size, permissions and file type
// Supports -l and -h flags only
// No user/group, UID/GUID specific permissions or alignment now
// No -a flag, shows everything except "." and ".."

#include <ctime>
#include <print>
#define ICL_IMPLEMENTATION
#include "icl.hpp"
using namespace icl;

char to_char(io::FileType type) {
  switch (type) {
  case io::FileType::DIRECTORY: return 'd';
  case io::FileType::SYMLINK: return 'l';
  case io::FileType::CHARDEV: return 'c';
  case io::FileType::BLOCKDEV: return 'b';
  default: return '-';
  }
}

std::string to_permission_str(io::FilePermission perms) {
  std::string res;
  if (is_set(perms, io::FilePermission::READ)) res += 'r';
  else res += '-';
  if (is_set(perms, io::FilePermission::WRITE)) res += 'w';
  else res += '-';
  if (is_set(perms, io::FilePermission::EXECUTE)) res += 'x';
  else res += '-';
  return res;
}

std::string to_human_readable(size_t bytes) {
  static const char* units[] = {"B", "K", "M", "G", "T", "P"};
  constexpr size_t unit_count = sizeof(units) / sizeof(units[0]);

  double size = static_cast<double>(bytes);
  size_t unit_idx = 0;

  while (size >= 1024.0 && unit_idx < unit_count - 1) {
    size /= 1024.0;
    unit_idx++;
  }

  char buf[32];
  if (unit_idx == 0) {
    snprintf(buf, sizeof(buf), "%zu%s", bytes, units[unit_idx]);
  } else {
    snprintf(buf, sizeof(buf), "%.1f%s", size, units[unit_idx]);
  }

  return std::string(buf);
}

const char* argv_shift(int* argc, char*** argv) {
  if (*argc < 1) return NULL;
  *argc -= 1;
  return *(*argv)++;
}

std::string to_ls_time(time_t mtime) {
  time_t now = std::time(nullptr);
  struct tm tm_buf;
#ifdef PLATFORM_WINDOWS
  localtime_s(&tm_buf, &mtime);
#else
  localtime_r(&mtime, &tm_buf);
#endif

  char buf[64];
  static constexpr time_t six_months = 60 * 60 * 24 * 30 * 6;

  if (now - mtime > six_months || mtime - now > six_months) {
    strftime(buf, sizeof(buf), "%b %e  %Y", &tm_buf);
  } else {
    strftime(buf, sizeof(buf), "%b %e %H:%M", &tm_buf);
  }

  return std::string(buf);
}

int main(int argc, char** argv) {
  const char* program_name = argv_shift(&argc, &argv);
  bool is_human = false;
  bool is_long = false;

  for (const char* flag = argv_shift(&argc, &argv); flag; flag = argv_shift(&argc, &argv)) {
    if (*flag != '-') continue;
    for (const char *c = flag + 1; *c; c++) {
      if (*c == 'l') is_long = true;
      if (*c == 'h') is_human = true;
    }
  }

  io::read_directory(".").match(
  [&](auto dir){
    if (is_long) {
      for (const auto& f : dir.files) {
        std::string size_str;
        if (is_human) size_str = to_human_readable(f.size);
        else size_str = std::to_string(f.size);
        std::printf("%c%s %s %s %s\n",
          to_char(f.type), to_permission_str(f.permissions).c_str(),
          size_str.c_str(), to_ls_time(f.mtime).c_str(), f.name.c_str());
      }
    } else {
      for (const auto& f : dir.files) {
        std::printf("%s\n", f.name.c_str());
      }
    }
  },
  [](auto err){
    std::printf("%s:%zu cannot read directory: %s: %s", err.file, err.line, err.msg, strerror(err.code));
  });
}
