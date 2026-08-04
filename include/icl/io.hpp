/**
 * @file io.hpp
 * @author ilpeN
 * @since 1.0.0
*/
#ifndef ICL_IO_HPP
#define ICL_IO_HPP

#include "os.hpp"
#include "base.hpp"
#include "either.hpp"
#include "array.hpp"

namespace icl::io {
enum class FilePermission : u8 {
  NONE = 0,
  EXECUTE = 1 << 0,
  WRITE = 1 << 1,
  READ = 1 << 2,
  Count = 4,
};

inline constexpr FilePermission operator |(FilePermission lhs, FilePermission rhs) {
  return static_cast<FilePermission>(
    static_cast<std::underlying_type_t<FilePermission>>(lhs) |
    static_cast<std::underlying_type_t<FilePermission>>(rhs)
  );
}

inline constexpr FilePermission& operator |=(FilePermission& lhs, FilePermission rhs) {
  lhs = lhs | rhs;
  return lhs;
}

inline constexpr FilePermission operator &(FilePermission lhs, FilePermission rhs) {
  return static_cast<FilePermission>(
    static_cast<std::underlying_type_t<FilePermission>>(lhs) &
    static_cast<std::underlying_type_t<FilePermission>>(rhs)
  );
}

inline constexpr bool operator ==(FilePermission lhs, FilePermission rhs) {
  return static_cast<std::underlying_type_t<FilePermission>>(lhs) ==
         static_cast<std::underlying_type_t<FilePermission>>(rhs);
}

inline constexpr bool operator !=(FilePermission lhs, FilePermission rhs) {
  return !(lhs == rhs);
}

inline constexpr bool is_set(FilePermission perms, FilePermission value) {
  return (perms & value) != FilePermission::NONE;
}

/**
 * @brief Abstracted file types from OS
 * @since 1.0.0
 * @detail
 * SYMLINK, BLOCKDEV, CHARDEV are not used in Windows.
 * @todo Add FIFO and socket file types
*/
enum class FileType {
  UNKNOWN = 0,
  REGULAR, // Normal text/binary files (-)
  DIRECTORY, // Directories / Folders (d)
  CHARDEV, // Character device (c)
  BLOCKDEV, // Block device (b)
  SYMLINK, // Symbolic links (l)
  Count,
};

/**
 * @brief Abstracted File struct
 * @since 1.0.0
 * @detail
 * It has capability to have children if type == FileType::DIRECTORY
 * All functions that are related to read file or directory returns this struct
 * @todo Add atime, ctime and hard_links, fields
*/
struct File {
  FileType type;
  std::string name;
  std::string content;
  size_t size;
  FilePermission permissions;
  os::time_t mtime;
  os::ino_t inode;
  ArrayList<File> files;
};

/**
 * @brief Creates directories by given *path*
 * @detail
 * path can include non-existent sub-directories and this function will
 * create them one by one if one of them doesn't exist.
 * @param path the directory path to be created
*/
extern bool mkdir_if_not_exists(const char* path);

// Conversions
extern const char* to_string(FileType ft);
extern FilePermission to_filepermission(os::mode_t mode);
extern FileType to_filetype(os::mode_t mode);

// File utilities
extern Result<std::string> read_file_content(const char* file_path, size_t file_size = 0);
extern Result<File> read_file_metadata(const char* file_path, bool follow_symlink = true);
extern Result<File> read_entire_file(const char* file_path, bool follow_symlink = true);

// Directory utilities
extern Result<File> read_entire_directory(const char* dir_path, bool read_content = false);
extern Result<File> read_directory(const char* dir_path, bool read_content = false);

} // namespace icl::io

#endif // ICL_IO_HPP
