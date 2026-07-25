#define BUICPP_IMPLEMENTATION
#include "buicpp.hpp"

#define USE_MINGW 0
#define DEBUG 1

int main(int argc, char** argv) {
  REBUILD_URSELF(argc, argv);
  buicpp::CommandBuilder cmd;
#if USE_MINGW == 1
  cmd.push("x86_64-w64-mingw32-g++");
#else
  cmd.push("clang++");
#endif

#if DEBUG == 1
  cmd.push_many("-O0", "-ggdb");
  #if USE_MINGW == 0
  cmd.push("-fsanitize=address,undefined");
  #endif
#else
  cmd.push_many("-O2");
#endif

  cmd.push("main.cpp");
  cmd.push_many("-o", "main");
  if (!cmd.run()) return 1;
}
