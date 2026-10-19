#include <iostream>

int main(int argc, char* argv[]) {
  (void)argv;
  if (argc < 2) {
    std::cerr << "Usage: cratchit vat --period YYYYMM\n";
    return 1;
  }
  std::cerr << "cratchit: no commands implemented yet\n";
  return 1;
}
