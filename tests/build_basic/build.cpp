#define BUICPP_IMPLEMENTATION
#include "buicpp.hpp"

int main(int argc, char** argv) {
  // Enables rebuilding itself (this build script) with it's bootstrapped compiler
  REBUILD_URSELF(argc, argv);

  // Just build a command and just run it
  // You can call other commands by the way not always build commands
  buicpp::CommandBuilder cmd;
  cmd.push_many("g++", "-std=c++23", "-o", "main", "main.cpp");
  if (!cmd.run()) return 1;
}
