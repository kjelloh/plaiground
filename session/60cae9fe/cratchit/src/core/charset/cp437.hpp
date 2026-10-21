#pragma once

#include "core/charset/unicode.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace charset::cp437 {

// One CP437 code point, which is one byte
using CodePoint = std::uint8_t;

// Takes CP437 bytes one at a time and gives back Unicode code points as they
// become available. 0x00-0x1F and 0x7F are the ASCII control characters.
class ToUnicodeBuffer {
public:
  // A code point once the bytes pushed so far make one, else nothing.
  // CP437 is one byte per character, so a byte always makes one.
  std::optional<unicode::CodePoint> push(CodePoint byte);
};

// All bytes of a CP437 text as Unicode code points
unicode::CodePoints to_unicode(std::string_view bytes);

} // namespace charset::cp437
