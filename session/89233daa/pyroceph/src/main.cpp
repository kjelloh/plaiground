#include "charset/unicode.hpp"
#include "charset/utf8.hpp"
#include "terminal/raw_terminal.hpp"

#include <cstdint>
#include <format>
#include <iostream>

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

int main() {
  pyroceph::terminal::RawTerminal terminal;
  charset::utf8::Decoder decoder;

  std::cout << "Press keys to see their Unicode code points. q quits." << std::endl;

  while (auto const byte = terminal.read_byte()) {
    auto const code_point = decoder.push(*byte);
    if (!code_point) {
      continue; // more bytes of this character to come
    }
    show(*code_point);
    if (*code_point == U'q') {
      return 0;
    }
  }
  if (auto const code_point = decoder.finish()) {
    show(*code_point);
  }
  return 0;
}
