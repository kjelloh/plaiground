#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// One heading of the manual, with its plain text and its subheadings
struct Section {
  std::string title;
  std::string text;
  std::vector<Section> children;
};

// Build the section tree from markdown headings (#, ##, ### ...).
// A single top-level # heading becomes the root; otherwise the root has no title.
Section parse_manual(std::string_view markdown);

// Turn markdown body text into plain text for the terminal
std::string to_plain_text(std::string_view markdown);

// The contents of MANUAL.md, built into the program
std::string_view manual_text();

} // namespace cratchit
