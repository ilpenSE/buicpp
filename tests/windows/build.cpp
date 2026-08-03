#define ICL_IMPLEMENTATION
#include "icl.hpp"

int main(int argc, char **argv) {
  REBUILD_URSELF(argc, argv);
  icl::CommandBuilder cmd;
  cmd.push_many("x86_64-w64-mingw32-gcc", "main.c", "-o", "main");
  if (!cmd.run()) return 1;
}
