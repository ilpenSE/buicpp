#define ICL_IMPLEMENTATION
#include "icl.hpp"

#define DEBUG 1

int main(int argc, char** argv) {
  REBUILD_URSELF(argc, argv);

  icl::CommandBuilder cmd;
  cmd.push("clang++");

#if DEBUG == 1
  cmd.push_many("-O0", "-ggdb", "-fsanitize=address,undefined");
#else
  cmd.push_many("-O2");
#endif

  cmd.push("-std=c++23");
  cmd.push("main.cpp");
  cmd.push_many("-o", "main");
  if (!cmd.run()) return 1;
}
