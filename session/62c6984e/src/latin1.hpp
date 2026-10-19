#pragma once

#include <expected>
#include <string>
#include <string_view>

namespace cratchit {

// Converts UTF-8 to ISO-8859-1. Fails on characters outside ISO-8859-1 or invalid UTF-8.
std::expected<std::string, std::string> utf8_to_latin1(std::string_view utf8);

// Converts ISO-8859-1 to UTF-8.
std::string latin1_to_utf8(std::string_view latin1);

} // namespace cratchit
