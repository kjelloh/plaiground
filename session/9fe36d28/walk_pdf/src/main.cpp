#include "document.hpp"
#include "manual.hpp"
#include "path.hpp"
#include "peek.hpp"
#include "plain_text.hpp"
#include "walk_pdf.hpp"

#include <charconv>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

// How many levels below the start path the walk goes, unless --depth says otherwise
constexpr std::size_t default_depth = 3;

std::optional<std::string> read_file(std::filesystem::path const& path) {
  std::ifstream file{path, std::ios::binary};
  if (!file) {
    return std::nullopt;
  }
  return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

// Inside a decoded stream the offset is in the decoded data, not the file: "+1234"
void print_row(walk_pdf::Content const& content) {
  auto const offset = content.in_file() ? std::format("{}", content.offset())
                                        : std::format("+{}", content.offset());
  std::println("{:>10} {:>10}  {:<32} {}", offset, content.bytes().size(), content.path().to_string(),
               walk_pdf::peek(content));
}

// Depth first: the element at path, then its children, down to depth levels below it.
// Acts on paths only; the document looks up each one.
void walk(walk_pdf::Document const& document, walk_pdf::Path const& path, std::size_t depth) {
  auto const content = document.find(path);
  if (!content) {
    return;
  }
  print_row(*content);
  if (depth == 0) {
    return;
  }
  for (auto const& child : document.children(path)) {
    walk(document, child.path, depth - 1);
  }
}

struct Arguments {
  std::string file;
  walk_pdf::Path path;
  std::size_t depth = default_depth;
  bool depth_given = false;
  bool text = false; // --text: the document's text, instead of its structure
};

// <file> [<path>] [--depth N] or --text <file>, in any order
std::expected<Arguments, std::string> parse_arguments(std::span<char*> arguments) {
  Arguments result;
  std::vector<std::string_view> positional;
  for (std::size_t i = 0; i < arguments.size(); ++i) {
    std::string_view const argument = arguments[i];
    if (argument == "--text") {
      result.text = true;
    } else if (argument == "--depth") {
      result.depth_given = true;
      if (i + 1 == arguments.size()) {
        return std::unexpected("--depth needs a number");
      }
      std::string_view const text = arguments[++i];
      auto const [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), result.depth);
      if (text.empty() || ec != std::errc{} || ptr != text.data() + text.size()) {
        return std::unexpected(std::format("--depth needs a number 0 or above, not '{}'", text));
      }
    } else if (argument.starts_with('-')) {
      return std::unexpected(std::format("unknown option '{}'", argument));
    } else {
      positional.push_back(argument);
    }
  }
  if (positional.empty()) {
    return std::unexpected("no file given");
  }
  if (positional.size() > 2) {
    return std::unexpected("too many arguments");
  }
  if (result.text && (positional.size() > 1 || result.depth_given)) {
    return std::unexpected("--text takes a file only, no path or --depth");
  }
  result.file = positional[0];
  if (positional.size() == 2) {
    auto const path = walk_pdf::Path::parse(positional[1]);
    if (!path) {
      return std::unexpected(std::format("invalid path: {}", path.error()));
    }
    result.path = *path;
  }
  return result;
}

} // namespace

// walk_pdf <file> [<path>] [--depth N] lists the element at path (default the
// whole file) and what is inside it, depth levels down (default 3).
// walk_pdf -h or --help prints the manual. Without arguments it prints its
// name and version.
int main(int argc, char* argv[]) {
  auto const arguments = std::span(argv, static_cast<std::size_t>(argc)).subspan(1);

  if (arguments.empty()) {
    std::println("walk_pdf {}", WALK_PDF_VERSION);
    return 0;
  }
  if (arguments.size() == 1 && (std::string_view{arguments[0]} == "-h" ||
                                std::string_view{arguments[0]} == "--help")) {
    std::print("{}", walk_pdf::manual_text());
    return 0;
  }

  auto const parsed = parse_arguments(arguments);
  if (!parsed) {
    std::println(stderr, "Error: {}\nSee walk_pdf --help", parsed.error());
    return 2;
  }

  if (parsed->text) {
    // Through the API, as other programs use walk_pdf: text to stdout, the
    // rest to stderr so that piping the text keeps only the text
    auto const extracted = walk_pdf::extract_text(std::filesystem::path{parsed->file});
    if (!extracted) {
      std::println(stderr, "Error: '{}': {}", parsed->file, extracted.error().message);
      return 1;
    }
    std::print("{}", extracted->text.to_string());
    auto const& report = extracted->report;
    if (report.pages == 0) {
      std::println(stderr, "Warning: '{}': no pages found (they may be in object streams, not read yet)",
                   parsed->file);
    }
    if (report.unmapped_glyphs > 0) {
      std::println(stderr, "Warning: '{}': {} characters could not be mapped (shown as U+FFFD)", parsed->file,
                   report.unmapped_glyphs);
    }
    return 0;
  }

  auto bytes = read_file(std::filesystem::path{parsed->file});
  if (!bytes) {
    std::println(stderr, "Error: cannot read '{}'", parsed->file);
    return 1;
  }

  walk_pdf::Document const document{std::move(*bytes)};
  if (!document.find(parsed->path)) {
    std::println(stderr, "Error: '{}' has no element {}", parsed->file, parsed->path.to_string());
    return 1;
  }
  walk(document, parsed->path, parsed->depth);
  return 0;
}
