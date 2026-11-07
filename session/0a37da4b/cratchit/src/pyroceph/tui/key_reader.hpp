#pragma once

#include "charset/decoder.hpp"
#include "charset/encoding.hpp"
#include "charset/unicode.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>

namespace pyroceph::tui {

using CodePoint = charset::unicode::CodePoint;

// Gives the next input byte, or nothing when the input has ended
using ByteSource = std::function<std::optional<std::uint8_t>()>;

// Reads bytes from a source and gives back one Unicode code point at a time.
// Bytes that cannot be decoded give U+FFFD and are counted in failure_count().
class KeyReader {
public:
  KeyReader(ByteSource bytes, charset::Encoding encoding)
      : bytes_(std::move(bytes)), decoder_(encoding) {}

  // The next code point, or nothing when the input has ended
  std::optional<CodePoint> next();

  // Number of failed input sequences so far
  std::size_t failure_count() const { return decoder_.failure_count(); }

private:
  ByteSource bytes_;
  charset::Decoder decoder_;
  bool ended_ = false;
};

// A KeyReader for stdin. Bytes from stdin are read as `encoding`. On Windows a
// console is read as UTF-16 (ReadConsoleW) instead, whatever its code page.
KeyReader terminal_key_reader(charset::Encoding encoding);

} // namespace pyroceph::tui
