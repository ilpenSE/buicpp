#define ICL_IMPLEMENTATION
#include "icl.hpp"
using namespace icl;

#define DEBUG 1

int main(int argc, char** argv) {
  REBUILD_URSELF(argc, argv);
  CommandBuilder cmd;
  cmd.push("g++");

#if DEBUG == 1
  cmd.push_many("-O0", "-ggdb", "-fsanitize=address,undefined");
#else
  cmd.push_many("-O2", "-fno-exceptions");
#endif

  cmd.push_many("-o", "main");
  cmd.push_many("main.cpp");
  if (!cmd.run()) return 1;
}
