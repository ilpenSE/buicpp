#include <print>
#include <iostream>
#include <cstdio>
#define BUICPP_IMPLEMENTATION
#include "buicpp.hpp"
using namespace buicpp;

Either<int, int> func() {
  if (0) return Either<int, int>(1, EitherLeft);
  return Either<int, int>(2, EitherRight);
}

int main() {
  Either<int, int> result = func();
  if (result.is_left()) std::println("It's left: {}", result.left());
  else std::println("It's right: {}", result.right());

  //////////////////////////////

  ArrayList<std::string> strings{"hello", "world", "merhaba", "dünya"};
  ArrayList<std::string> editors{"emacs", "vim", "neovim", "nano", "vscode"};
  ArrayList<std::string> names{"bob", "marley", "ilpeN", "emo", "halilibo"};
  std::println("original names = {}", names);
  std::println("original editors = {}", editors);
  std::println("original strings = {}", strings);
  ArrayList<ArrayList<std::string>> arr{strings, editors, names};
  std::println("arr = {}", arr);
  names.remove(1);
  std::println("arr = {}", arr);
  arr[2].remove(1);
  std::println("arr = {}", arr);

  ArrayList<int> a{1,2,3};
  ArrayList<int> b = a;
  b.remove(0);
  std::println("a = {}", a);
  std::println("b = {}", b);
}
