#include <icl/io>
#include <icl/os>
#include <cstdio>
#include <cstring>

namespace icl::io {
bool mkdir_if_not_exists(const char* path) {
  for (const char* p = path + 1; *p != '\0'; p++) {
    if (*p == '/') {
      size_t i = (size_t)(p - path);
      if (i >= PATH_MAX) return false;
      char buf[PATH_MAX] = {0};
      std::memcpy(buf, path, i);
      if (os::mkdir(buf, 0775) != 0 && errno != EEXIST) return false;
    }
  }
  return true;
}

// no recursion here
Result<File> read_directory(const char* dir_path, bool read_content) {
  // open directory
  os::DIR *dir_ptr = os::opendir(dir_path);
  if (!dir_ptr) return Err({errno, "opendir failed", __FILE__, __LINE__});
  defer { os::closedir(dir_ptr); };

  // stat of directory itself
  auto ret = read_file_metadata(dir_path);
  if (ret.is_error()) return Err(ret.error());
  File dir = std::move(ret).ok();

  // iterate directory (no recursive look)
  struct os::dirent *entry;
  while ((entry = os::readdir(dir_ptr)) != NULL) {
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
    if (ret.is_error()) return Err(ret.error());
    dir.files.push(std::move(ret).ok());
  }

  return Ok(dir);
}

Result<File> read_entire_directory(const char* dir_path, bool read_content) {
  static_assert(std::is_move_assignable_v<File>, "File is not move assignable!");
  auto ret = read_directory(dir_path, read_content);
  if (ret.is_error()) return Err(ret.error());
  File top = std::move(ret).ok();

  for (auto& f : top.files) {
    if (f.type != FileType::DIRECTORY) continue;
    auto ret = read_entire_directory(f.name.c_str(), read_content);
    if (ret.is_error()) return Err(ret.error());
    f = std::move(ret).ok();
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
  struct os::stat st;
  if (follow_symlink) {
    if (os::stat(file_path, &st) != 0) return Err({errno, "stat failed", __FILE__, __LINE__});
  } else {
    if (os::lstat(file_path, &st) != 0) return Err({errno, "lstat failed", __FILE__, __LINE__});
  }

  result.type = to_filetype((os::mode_t)st.st_mode);
  result.name = std::string(file_path);
  result.mtime = st.st_mtime;
  result.inode = st.st_ino;
  result.permissions = to_filepermission(st.st_mode);
  result.size = st.st_size;

  if (result.type == FileType::SYMLINK) {
    char buf[PATH_MAX];
    os::ssize_t n = os::readlink(file_path, buf, sizeof(buf));
    if (n < 0) { return Err({errno, "readlink failed"}); }
    result.content = std::string(buf, n);
  }
  return Ok(std::move(result));
}

Result<File> read_entire_file(const char* file_path, bool follow_symlink) {
  auto ret = read_file_metadata(file_path, follow_symlink);
  if (ret.is_error()) return Err(ret.error());
  File result = std::move(ret).ok();

  if (result.type == FileType::REGULAR) {
    auto ret = read_file_content(file_path, result.size);
    if (ret.is_error()) return Err(ret.error());
    result.content = std::move(ret).ok();
  }
  return Ok(std::move(result));
}

FileType to_filetype(os::mode_t mode) {
  switch (mode & os::ifmt) {
  case os::ifreg: return FileType::REGULAR;
  case os::ifdir: return FileType::DIRECTORY;
  case os::ifchr: return FileType::CHARDEV;
  case os::iflnk: return FileType::SYMLINK;
  case os::ifblk: return FileType::BLOCKDEV;
  default:        return FileType::UNKNOWN;
  }
  UNREACHABLE("to_filetype");
}

FilePermission to_filepermission(os::mode_t mode){
  FilePermission perms = FilePermission::NONE;
  if (mode & os::irusr) perms |= FilePermission::READ;
  if (mode & os::iwusr) perms |= FilePermission::WRITE;
  if (mode & os::ixusr) perms |= FilePermission::EXECUTE;
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
  UNREACHABLE("const char* to_string(icl::io::FileType e)");
}

} // namespace icl::io
