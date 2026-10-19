#include "core/version.hpp"

#include <print>

int main() {
  std::println("cratchit {}", cratchit::version());
  return 0;
}
