#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// One field of a SIE record line.
struct SieField {
  std::string m_text;             // unquoted and unescaped; for an object list: the content between { and }
  bool m_is_object_list = false;  // true for a {...} field
};

// Splits one SIE record line (e.g. `#TRANS 1920 {} -200 "" "privat inköp" 0`) into fields.
// Fields are separated by spaces or tabs. "..." is a quoted string where \ escapes the next
// character. {...} is an object list. The label (e.g. "#TRANS") is the first field.
std::expected<std::vector<SieField>, std::string> split_sie_line(std::string_view line);

} // namespace cratchit
