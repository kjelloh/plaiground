#include "manual.hpp"

#include <cstdio>
#include <print>
#include <span>
#include <string_view>

// walk_pdf -h or --help prints the manual. Without arguments it prints its
// name and version. Any other argument is an error (exit code 2).
int main(int argc, char* argv[]) {
  auto const arguments = std::span(argv, static_cast<std::size_t>(argc)).subspan(1);

  if (arguments.empty()) {
    std::println("walk_pdf {}", WALK_PDF_VERSION);
    return 0;
  }

  std::string_view const argument = arguments.front();
  if (arguments.size() == 1 && (argument == "-h" || argument == "--help")) {
    std::print("{}", walk_pdf::manual_text());
    return 0;
  }

  std::println(stderr, "Error: unknown argument '{}'\nSee walk_pdf --help", argument);
  return 2;
}
