#include <stdio.h>
#define BUICPP_IMPLEMENTATION
#include "buicpp.hpp"

#define BUILD_FOLDER "build/"

int main(int argc, char** argv) {
  REBUILD_URSELF(argc, argv);
  if (!buicpp::io::mkdir_if_not_exists(BUILD_FOLDER)) return 1;
  buicpp::CommandBuilder cmd;
  cmd.push("clang++");

  cmd.push_many("-ggdb", "-O0", "-fno-exceptions", "-fsanitize=address,undefined");

  cmd.push("main.cpp");
  cmd.push_many("-o", BUILD_FOLDER"main");
  if (!cmd.run()) return 1;
  return 0;
}
