#pragma once

#include <optional>

namespace cratchit {

// Puts the console into single-key mode (no Enter needed, no echo) while alive.
// Does nothing when stdin is not a terminal, e.g. when input is piped.
class RawTerminal {
public:
  RawTerminal();
  ~RawTerminal();
  RawTerminal(RawTerminal const&) = delete;
  RawTerminal& operator=(RawTerminal const&) = delete;

  // Block until a key is pressed; nothing at end of input
  std::optional<char> read_key();

private:
  bool is_raw_ = false;
};

} // namespace cratchit
