#include <icl/build>
#include <icl/os>
#include <icl/print>

int main(int argc, char **argv) {
  REBUILD_URSELF(argc, argv, "-I../include",
    "-L../build", "-l:libicl.a", "-O2");
  icl::todo("implement generating single-header");
}
