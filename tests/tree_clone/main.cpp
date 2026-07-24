#include <cinttypes>
#include <print>
#include <memory>

#define BUICPP_IMPLEMENTATION
#include "buicpp.hpp"
using namespace buicpp;

void print_tree(const io::File& file, std::string_view prefix = "", bool is_last = true, bool is_root = true) {
  std::string_view display_name = file.name;
  size_t pos = display_name.find_last_of('/');
  if (pos != std::string_view::npos) display_name = display_name.substr(pos + 1);

  if (!is_root) {
    std::print("{}{}{}", prefix, is_last ? "└── " : "├── ", display_name);
  } else {
    std::print("{}", display_name);
  }

  if (file.type == io::FileType::DIRECTORY) {
    std::println("/");
  } else if (file.type == io::FileType::SYMLINK) {
    std::println(" -> {}", file.content);
  } else {
    std::println(" ({} bytes)", file.size);
  }

  if (file.type == io::FileType::DIRECTORY) {
    std::string child_prefix = std::string(prefix);
    if (!is_root) child_prefix += is_last ? "    " : "│   ";

    for (size_t i = 0; i < file.files.count(); i++) {
      bool child_is_last = (i == file.files.count() - 1);
      print_tree(file.files[i], std::string_view(child_prefix), child_is_last, false);
    }
  }
}

int main() {
  io::read_entire_directory("./some_dir").match(
  [](auto dir){
    print_tree(dir);
  }, [](auto err){
    std::println("{}:{}: read_entire_directory failed: {}: {}\n",
      err.file, err.line, err.msg, strerror(err.code));
  });
}
