#include "pyroceph/tui/prompt_loop.hpp"

#include <cstdint>
#include <print>

namespace pyroceph::tui {

bool is_quit(CodePoint code_point) {
  return code_point == 0x03 || code_point == 0x04 || code_point == 0x1A;
}

void run_prompt_loop(KeyReader& keys, Lookup const& lookup, std::FILE* out) {
  while (auto const code_point = keys.next()) {
    if (is_quit(*code_point)) return;
    auto const choice = lookup(*code_point);
    std::print(out, "U+{:04X} ({})\n", static_cast<std::uint32_t>(*code_point), choice ? "val" : "inget val");
    std::fflush(out);
  }
}

} // namespace pyroceph::tui
