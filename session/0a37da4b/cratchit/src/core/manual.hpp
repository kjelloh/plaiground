#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// The name of the user manual in the framework folder (<program folder>/.cratchit/)
inline constexpr std::string_view manual_file_name = "anvandarmanual.md";

// The user manual (docs/anvandarmanual.md, Markdown in Swedish), read for its
// help paragraphs. A help paragraph is marked by a line of its own
// `<!-- hjälp: <topic> -->` (invisible when the Markdown is shown) and is the
// paragraph that follows it: its lines up to the first blank line, a heading
// or another comment.
class Manual {
public:
  explicit Manual(std::string text) : text_(std::move(text)) {}

  // The help paragraph of topic as plain text lines for the terminal: no
  // `**`, `*` or backticks, and a link shows only its text. Empty when the
  // manual has no help paragraph for topic.
  std::vector<std::string> help(std::string_view topic) const;

  // The topics the manual has help paragraphs for, in order
  std::vector<std::string> topics() const;

private:
  std::string text_;
};

// Reads the manual from file. Error text when it cannot be read.
std::expected<Manual, std::string> read_manual(std::filesystem::path const& file);

// The Markdown of one line as plain text for the terminal (see Manual::help).
// A list item `* x` becomes `- x`.
std::string plain_text_of(std::string_view markdown);

} // namespace cratchit
