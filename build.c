#include <stdio.h>
#define CTL_IMPLEMENTATION
#include "ctl.h"

int main(int argc, char** argv) {
  BUIC_REBUILD_URSELF(argc, argv);
  CommandBuilder cmd = {0};
  cmd_push(&cmd, "clang++", "-std=c++23");
  // cmd_push(&cmd, "-c");
  // cmd_push(&cmd, "-I"INCLUDE_FOLDER);
  cmd_push(&cmd, "-ggdb", "-O0", "-fsanitize=address,undefined");
  cmd_push(&cmd, "main.cpp");
  cmd_push(&cmd, "-o", "main");
  if (!cmd_run(&cmd)) return 1;
  return 0;
}
