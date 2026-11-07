#include "pyroceph/tui/prompt_loop.hpp"

#include "charset/utf8.hpp"

#include <print>
#include <string>
#include <vector>

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

std::vector<CommonKey> common_keys(Navigator const& navigator) {
  std::vector<CommonKey> keys;
  if (navigator.can_go_back()) keys.push_back({utf8_of(back_key), "Tillbaka"});
  keys.push_back({utf8_of(help_key), "Visa gemensamma val"});
  keys.push_back({"Ctrl-D", "Avsluta (även Ctrl-C och Ctrl-Z)"});
  return keys;
}

void show_common_keys(Navigator const& navigator, std::FILE* out) {
  std::print(out, "\n");
  for (auto const& common : common_keys(navigator)) {
    std::print(out, "  {:<7} {}\n", common.key, common.caption);
  }
  std::print(out, "{}", prompt_of(navigator));
  std::fflush(out);
}

std::string prompt_of(Navigator const& navigator) {
  std::string prompt;
  for (auto const* state : navigator.path()) {
    if (!prompt.empty()) prompt += path_separator;
    prompt += state->name;
  }
  return prompt + "> ";
}

void show(Navigator const& navigator, std::FILE* out) {
  std::print(out, "\n");
  for (auto const& choice : navigator.current().choices) {
    std::print(out, "  {} {}\n", utf8_of(choice.key), choice.caption);
  }
  std::print(out, "{}", prompt_of(navigator));
  std::fflush(out);
}

void run_prompt_loop(KeyReader& keys, Navigator& navigator, std::FILE* out) {
  show(navigator, out);
  while (auto const code_point = keys.next()) {
    if (is_quit(*code_point)) break;
    if (*code_point == help_key) {
      show_common_keys(navigator, out);
      continue;
    }
    if (navigator.press(*code_point)) show(navigator, out);
  }
  std::print(out, "\n");
  std::fflush(out);
}

} // namespace pyroceph::tui
