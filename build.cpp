#include <stdio.h>
#define BUICPP_IMPLEMENTATION
#include "buicpp.hpp"

#define BUILD_FOLDER "build/"

#define USE_MINGW 0

int main(int argc, char** argv) {
  REBUILD_URSELF(argc, argv);
  if (!buicpp::io::mkdir_if_not_exists(BUILD_FOLDER)) return 1;
  buicpp::CommandBuilder cmd;
#if USE_MINGW == 1
  cmd.push("x86_64-w64-mingw32-g++");
#else
  cmd.push("clang++");
#endif

  cmd.push("-std=c++23");
  cmd.push_many("-ggdb", "-O0");

#if USE_MINGW == 0
  cmd.push("-fsanitize=address,undefined");
#endif

  cmd.push("main.cpp");
  cmd.push_many("-o", BUILD_FOLDER"main");
  if (!cmd.run()) return 1;
  return 0;
}
