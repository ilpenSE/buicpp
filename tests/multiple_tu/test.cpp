#include <icl/print>
#include <icl/os>

void greet(const char *name) {
  icl::println("Hello, {}", name);
  icl::os::Process proc("ls", "-lah");
  proc.spawn();
  proc.wait();
}
