#pragma once

#include "core/charset/cp437.hpp"
#include "core/charset/unicode.hpp"
#include "core/charset/utf8.hpp"
#include "core/terminal_encoding.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <variant>

namespace cratchit {

// Supplies the next byte read from the terminal, or nothing at end of input
using ByteSource = std::function<std::optional<char>()>;

// Reads key presses as Unicode code points, decoding the terminal's bytes from
// its input charset. A byte sequence that does not decode gives U+FFFD
// (replacement character) and is logged as a design insufficiency.
class KeyReader {
public:
  // A charset cratchit cannot read is read as UTF-8
  KeyReader(Charset charset, ByteSource next_byte);

  // The next key, or nothing at end of input
  std::optional<charset::unicode::CodePoint> next();

private:
  ByteSource next_byte_;
  std::variant<charset::utf8::ToUnicodeBuffer, charset::cp437::ToUnicodeBuffer> buffer_;
  bool finished_ = false;

  std::optional<charset::unicode::CodePoint> push(std::uint8_t byte);
  std::optional<charset::unicode::CodePoint> finish();
};

} // namespace cratchit
