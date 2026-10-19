#include "latin1.hpp"

#include <cstddef>
#include <cstdint>

namespace cratchit {

std::expected<std::string, std::string> utf8_to_latin1(std::string_view utf8) {
  std::string out;
  out.reserve(utf8.size());
  for (std::size_t i = 0; i < utf8.size(); ++i) {
    auto const byte = static_cast<std::uint8_t>(utf8[i]);
    if (byte < 0x80) {
      out += static_cast<char>(byte);
      continue;
    }
    // Two-byte sequences C2 80 .. C3 BF cover U+0080 .. U+00FF, which is exactly ISO-8859-1.
    if ((byte == 0xC2 || byte == 0xC3) && i + 1 < utf8.size()) {
      auto const next = static_cast<std::uint8_t>(utf8[i + 1]);
      if ((next & 0xC0) == 0x80) {
        out += static_cast<char>(((byte & 0x03) << 6) | (next & 0x3F));
        ++i;
        continue;
      }
    }
    return std::unexpected("Text contains a character that ISO-8859-1 cannot represent: \"" + std::string{utf8} + "\"");
  }
  return out;
}

std::string latin1_to_utf8(std::string_view latin1) {
  std::string out;
  out.reserve(latin1.size());
  for (char c : latin1) {
    auto const byte = static_cast<std::uint8_t>(c);
    if (byte < 0x80) {
      out += c;
    } else {
      out += static_cast<char>(0xC0 | (byte >> 6));
      out += static_cast<char>(0x80 | (byte & 0x3F));
    }
  }
  return out;
}

} // namespace cratchit
