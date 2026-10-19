#pragma once

#include <string>
#include <string_view>

namespace cratchit {

// Converts text in code page 437 (SIE "#FORMAT PC8") to UTF-8.
std::string cp437_to_utf8(std::string_view cp437);

} // namespace cratchit
