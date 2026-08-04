#include <icl/build>

int main(int argc, char** argv) {
  // Enables rebuilding itself (this build script) with it's bootstrapped compiler
  REBUILD_URSELF(argc, argv, "-I../../include", "-L../../build", "-l:libicl.a");

  // Just build a command and just run it
  // You can call other commands by the way not always build commands
  icl::CommandBuilder cmd;
  cmd.push_many("g++", "-o", "main", "main.cpp");
  if (!cmd.run()) return 1;
}
