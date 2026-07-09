#include <stdio.h>
#define BUICPP_IMPLEMENTATION
#include "buicpp.hpp"
using namespace buicpp;
int main(int argc, char** argv) {
  REBUILD_URSELF(argc, argv);
  CommandBuilder cmd;
  cmd.push_many("clang++", "-std=c++23", "main.cpp", "-o", "main");
  if (!cmd.run()) return 1;
  return 0;
}
