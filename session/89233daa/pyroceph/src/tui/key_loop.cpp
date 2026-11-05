#include "tui/key_loop.hpp"

#include "charset/unicode.hpp"
#include "charset/utf8.hpp"
#include "tui/raw_terminal.hpp"

#include <cstdint>
#include <format>
#include <iostream>

namespace pyroceph::tui {

namespace {

using charset::unicode::CodePoint;

bool is_control(CodePoint code_point) {
  return code_point < 0x20 || (code_point >= 0x7F && code_point < 0xA0);
}

// One line, e.g. "U+00E5  å". Control characters show as <control>.
void show(CodePoint code_point) {
  std::cout << std::format("U+{:04X}  ", static_cast<std::uint32_t>(code_point));
  if (is_control(code_point)) {
    std::cout << "<control>";
  } else {
    for (auto const byte : charset::utf8::Encoder{}.push(code_point)) {
      std::cout << static_cast<char>(byte);
    }
  }
  std::cout << std::endl;
}

} // namespace

KeyLoop::KeyLoop(core::Orchestrator& orchestrator) : orchestrator_(orchestrator) {}

int KeyLoop::run() {
  RawTerminal terminal;
  charset::utf8::Decoder decoder;

  std::cout << "Press keys to see their Unicode code points. q quits." << std::endl;

  while (auto const byte = terminal.read_byte()) {
    auto const code_point = decoder.push(*byte);
    if (!code_point) {
      continue; // more bytes of this character to come
    }
    show(*code_point);
    if (orchestrator_.on_key(*code_point) == core::KeyResult::Quit) {
      return 0;
    }
  }
  if (auto const code_point = decoder.finish()) {
    show(*code_point);
    orchestrator_.on_key(*code_point); // the input has ended, so the result changes nothing
  }
  return 0;
}

} // namespace pyroceph::tui
