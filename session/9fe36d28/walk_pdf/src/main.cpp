#include "manual.hpp"
#include "walker.hpp"

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

// The start of an element's first line, with bytes outside printable ASCII shown as '.'
std::string preview(std::string_view bytes, std::size_t max_length = 60) {
  std::string result;
  for (auto const c : bytes) {
    if (c == '\n' || c == '\r' || result.size() == max_length) {
      break;
    }
    result += (c >= ' ' && c <= '~') ? c : '.';
  }
  return result;
}

} // namespace

// walk_pdf <file> lists the file's top-level PDF elements.
// walk_pdf -h or --help prints the manual. Without arguments it prints its
// name and version. Anything else is an error (exit code 2).
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

  if (arguments.size() != 1 || argument.starts_with('-')) {
    std::println(stderr, "Error: unknown argument '{}'\nSee walk_pdf --help", argument);
    return 2;
  }

  auto const bytes = read_file(std::filesystem::path{argument});
  if (!bytes) {
    std::println(stderr, "Error: cannot read '{}'", argument);
    return 1;
  }

  walk_pdf::Walker{*bytes}.walk([](walk_pdf::Path const& path, walk_pdf::Content const& content) {
    std::println("{:>10} {:>10}  {:<20} {}", content.offset, content.bytes.size(), path.to_string(),
                 preview(content.bytes));
  });
  return 0;
}
