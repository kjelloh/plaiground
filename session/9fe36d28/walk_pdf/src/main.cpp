#include "document.hpp"
#include "manual.hpp"
#include "path.hpp"
#include "peek.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>

namespace {

std::optional<std::string> read_file(std::filesystem::path const& path) {
  std::ifstream file{path, std::ios::binary};
  if (!file) {
    return std::nullopt;
  }
  return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

void print_row(walk_pdf::Content const& content) {
  std::println("{:>10} {:>10}  {:<24} {}", content.offset(), content.bytes().size(),
               content.path().to_string(), walk_pdf::peek(content));
}

// The content's children and their children, depth first
void print_children(walk_pdf::Content const& content) {
  for (auto const& [path, child] : content) {
    print_row(child);
    print_children(child);
  }
}

} // namespace

// walk_pdf <file> lists the file's elements; walk_pdf <file> <path> lists the
// element at path and what is inside it. walk_pdf -h or --help prints the
// manual. Without arguments it prints its name and version.
int main(int argc, char* argv[]) {
  auto const arguments = std::span(argv, static_cast<std::size_t>(argc)).subspan(1);

  if (arguments.empty()) {
    std::println("walk_pdf {}", WALK_PDF_VERSION);
    return 0;
  }

  std::string_view const first = arguments.front();
  if (arguments.size() == 1 && (first == "-h" || first == "--help")) {
    std::print("{}", walk_pdf::manual_text());
    return 0;
  }

  if (arguments.size() > 2) {
    std::println(stderr, "Error: too many arguments\nSee walk_pdf --help");
    return 2;
  }
  if (first.starts_with('-')) {
    std::println(stderr, "Error: unknown option '{}'\nSee walk_pdf --help", first);
    return 2;
  }

  auto path = walk_pdf::Path{};
  if (arguments.size() == 2) {
    auto const parsed = walk_pdf::Path::parse(arguments[1]);
    if (!parsed) {
      std::println(stderr, "Error: invalid path: {}\nSee walk_pdf --help", parsed.error());
      return 2;
    }
    path = *parsed;
  }

  auto bytes = read_file(std::filesystem::path{first});
  if (!bytes) {
    std::println(stderr, "Error: cannot read '{}'", first);
    return 1;
  }

  walk_pdf::Document const document{std::move(*bytes)};
  auto const content = document.find(path);
  if (!content) {
    std::println(stderr, "Error: '{}' has no element {}", first, path.to_string());
    return 1;
  }
  if (!path.is_root()) {
    print_row(*content);
  }
  print_children(*content);
  return 0;
}
