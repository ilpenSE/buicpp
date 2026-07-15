#include <cstdio>
#include <stdio.h>
#define BUICPP_IMPLEMENTATION
#include "buicpp.hpp"
using namespace buicpp;

int main() {
  io::read_entire_directory("./").match(
  [](auto root){
    printf("root.name = %s\n", root.info.name.c_str());
    for (size_t i = 0; i < root.files.count(); i++) {
      const auto& entry = root.files[i];
      printf("entry.name = %s\n", entry.name.c_str());
    }
  },
  [](auto err){
    fprintf(stderr, "ERROR: cannot read_entire_directory: %s: %s\n", err.msg, strerror(err.code));
  }
  );
}
