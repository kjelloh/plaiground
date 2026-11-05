#pragma once

#include <cstdint>
#include <optional>

namespace pyroceph::tui {

// Puts the console into single-key mode (no Enter needed, no echo) while alive.
// Does nothing when stdin is not a terminal, e.g. when input is piped.
// Adapted from cratchit's RawTerminal (session 60cae9fe).
class RawTerminal {
public:
  RawTerminal();
  ~RawTerminal();
  RawTerminal(RawTerminal const&) = delete;
  RawTerminal& operator=(RawTerminal const&) = delete;

  // Blocks until a byte is read; nothing at end of input. One key may come as
  // several bytes, in the terminal's input encoding.
  std::optional<std::uint8_t> read_byte();

private:
  bool is_raw_ = false;
};

} // namespace pyroceph::tui
