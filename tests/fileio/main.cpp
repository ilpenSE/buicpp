#include <cstdio>
#define BUICPP_IMPLEMENTATION
#include "buicpp.hpp"

#define NOTES_FILE "notes.txt"
#define TODOS_FILE "todos.txt"

void print_file(const buicpp::io::File& file) {
  printf("Properties of %s:\n", file.name.c_str());
  printf("Size: %zu\n", file.size);
  printf("Type: %s\n", to_string(file.type));
  if (file.type == buicpp::io::FileType::SYMLINK) {
    printf("Symlinked to: %s\n", file.content.c_str());
  } else {
    printf("Content: \"%s\"\n", file.content.c_str());
  }
}

int main() {
  printf("------------------------------\n");
  printf("reading entire file: %s\n", NOTES_FILE);
  buicpp::io::read_entire_file(NOTES_FILE).match(
  [](auto file){
    print_file(file);
  },
  [](auto err){
    fprintf(stderr, "%s:%zu: read_entire_file failed: %s: %s\n", err.file, err.line, err.msg, strerror(err.code));
  });
  printf("------------------------------\n");

  printf("------------------------------\n");
  printf("reading entire file: %s\n", TODOS_FILE);
  buicpp::io::read_entire_file(TODOS_FILE, false).match(
  [](auto file){
    print_file(file);
  },
  [](auto err){
    fprintf(stderr, "%s:%zu: read_entire_file failed: %s: %s\n", err.file, err.line, err.msg, strerror(err.code));
  });
  printf("------------------------------\n");
}
