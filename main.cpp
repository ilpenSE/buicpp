#include <cstdio>
#include <stdio.h>
#define BUICPP_IMPLEMENTATION
#include "buicpp.hpp"
using namespace buicpp;

Either<std::string, int> func() {
  if (1) return LeftOf<std::string>("paper flowers");
  return RightOf<int>(21);
}

int main() {
  func().match(
  [](auto left){
    std::printf("LEFT value = %s\n", left.c_str());
  },
  [](auto right){
    std::printf("RIGHT value = %d\n", right);
  });

  //////////////////////////////

  Either<std::string, int> res = func();
  if (res.is_left()) {
    std::string s = std::move(res).left();
    std::printf("stealed s = \"%s\"\n", s.c_str());
    std::printf("orig res.left = \"%s\"\n", res.left().c_str());
  }
}
