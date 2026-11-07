#include "pyroceph/tui/prompt_loop.hpp"

#include "charset/utf8.hpp"

#include <print>
#include <string>

namespace pyroceph::tui {

namespace {

std::string utf8_of(CodePoint code_point) {
  auto const bytes = charset::utf8::Encoder().push(code_point);
  return std::string(bytes.begin(), bytes.end());
}

} // namespace

bool is_quit(CodePoint code_point) {
  return code_point == 0x03 || code_point == 0x04 || code_point == 0x1A;
}

void show(Navigator const& navigator, std::FILE* out) {
  auto const& state = navigator.current();
  std::print(out, "\n{}\n", state.name);
  for (auto const& choice : state.choices) {
    std::print(out, "  {} {}\n", utf8_of(choice.key), choice.caption);
  }
  if (navigator.can_go_back()) std::print(out, "  {} Tillbaka\n", utf8_of(back_key));
  std::fflush(out);
}

void run_prompt_loop(KeyReader& keys, Navigator& navigator, std::FILE* out) {
  show(navigator, out);
  while (auto const code_point = keys.next()) {
    if (is_quit(*code_point)) return;
    if (navigator.press(*code_point)) show(navigator, out);
  }
}

} // namespace pyroceph::tui
