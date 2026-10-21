#include "core/charset/utf8.hpp"

namespace charset::utf8 {

namespace {

constexpr unicode::CodePoint replacement_character = 0xFFFD;

// Lead byte bits ORed with the top bits, then continuation bytes 10xxxxxx
Bytes encode(unicode::CodePoint code_point, std::size_t size, CodePoint lead) {
  Bytes result;
  result.size = size;
  for (std::size_t i = size - 1; i > 0; --i) {
    result.bytes[i] = static_cast<CodePoint>(0x80 | (code_point & 0x3F));
    code_point >>= 6;
  }
  result.bytes[0] = static_cast<CodePoint>(lead | code_point);
  return result;
}

} // namespace

Bytes FromUnicodeBuffer::push(unicode::CodePoint code_point) {
  if (code_point < 0x80) {
    return encode(code_point, 1, 0x00);
  }
  if (code_point < 0x800) {
    return encode(code_point, 2, 0xC0);
  }
  if ((code_point >= 0xD800 && code_point <= 0xDFFF) || code_point > 0x10FFFF) {
    return encode(replacement_character, 3, 0xE0);
  }
  if (code_point < 0x10000) {
    return encode(code_point, 3, 0xE0);
  }
  return encode(code_point, 4, 0xF0);
}

} // namespace charset::utf8
