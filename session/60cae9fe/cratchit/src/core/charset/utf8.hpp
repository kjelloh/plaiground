#pragma once

#include "core/charset/unicode.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace charset::utf8 {

// One UTF-8 byte
using CodePoint = std::uint8_t;

// The one to four UTF-8 bytes of one Unicode code point
struct Bytes {
  std::array<CodePoint, 4> bytes{};
  std::size_t size = 0;

  CodePoint const* begin() const { return bytes.data(); }
  CodePoint const* end() const { return bytes.data() + size; }
};

// Takes Unicode code points one at a time and gives back their UTF-8 bytes.
// A surrogate (U+D800-U+DFFF) or a value above U+10FFFF is not a character
// and gives the bytes of U+FFFD (replacement character).
class FromUnicodeBuffer {
public:
  Bytes push(unicode::CodePoint code_point);
};

} // namespace charset::utf8
