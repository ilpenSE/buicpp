#include <iostream>
#include <cstdio>
#include <memory>
#define BUICPP_IMPLEMENTATION
#include "buicpp.hpp"
using namespace buicpp;

// struct Person {
//   u8 age;
//   const char* name;
// };

int main() {
  int a = 10;
  int b = 20;
  std::unique_ptr<int> pa = std::make_unique<int>(a);
  std::unique_ptr<int> pb = std::make_unique<int>(b);
  ArrayList<std::unique_ptr<int>> pointers;
  pointers.push(std::move(pa));
  pointers.push(std::move(pb));
  for (const auto& ptr : pointers) {
    std::cout << *ptr << std::endl;
  }

  // ArrayList<Person> persons1;
  // persons1.push(Person{1, "selam"});
  // ArrayList<Person> persons2;
  // persons2.push(Person{1, "selam"});
  // std::cout << "persons1 == persons2: " << (persons1 == persons2) << std::endl;

  ArrayList<int> ints1;
  ints1.push_many(1, 2);
  ArrayList<int> ints2;
  ints2.push_many(1, 2);
  std::cout << "ints1 == ints2: " << (ints1 == ints2) << std::endl;

  io::read_directory("./some_dir/").match(
  [](auto dir){
    printf("%s\n", dir.info.name.c_str());
    for (const auto& entry : dir.files) {
      if (entry.type == io::FileType::SYMLINK) {
        printf("  %s (symlink) -> %s\n", entry.name.c_str(), entry.symlink_target.c_str());
      } else {
        printf("  %s (%s)\n", entry.name.c_str(), to_string(entry.type));
      }
    }
  },
  [](auto err){
    fprintf(stderr, "ERROR: cannot read_entire_directory: %s: %s\n", err.msg, strerror(err.code));
  });
}
