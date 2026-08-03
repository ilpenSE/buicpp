#include <stdio.h>
#define ICL_IMPLEMENTATION
#include "icl.hpp"

#define BUILD_FOLDER "build/"

int main(int argc, char** argv) {
  REBUILD_URSELF(argc, argv);
  if (!icl::io::mkdir_if_not_exists(BUILD_FOLDER)) return 1;
  icl::CommandBuilder cmd;
  cmd.push("g++");

  cmd.push_many("-ggdb", "-O0", "-fno-exceptions", "-fsanitize=address,undefined");

  cmd.push("main.cpp");
  cmd.push_many("-o", BUILD_FOLDER"main");
  if (!cmd.run()) return 1;
  return 0;
}
