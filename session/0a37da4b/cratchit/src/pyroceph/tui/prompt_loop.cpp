#include "pyroceph/tui/prompt_loop.hpp"

#include "charset/utf8.hpp"
#include "pyroceph/tui/input_buffer.hpp"
#include "pyroceph/tui/key_parser.hpp"

#include <print>
#include <string>
#include <vector>

namespace pyroceph::tui {

namespace {

std::string utf8_of(CodePoint code_point) {
  auto const bytes = charset::utf8::Encoder().push(code_point);
  return std::string(bytes.begin(), bytes.end());
}

std::string utf8_of(std::u32string const& text) {
  std::string result;
  for (auto const code_point : text) result += utf8_of(code_point);
  return result;
}

std::string path_of(Navigator const& navigator) {
  std::string path;
  for (auto const& state : navigator.path()) {
    if (!path.empty()) path += path_separator;
    path += state->name();
  }
  return path;
}

// The prompt ends in "> " while navigating and ": " while typing input. Both
// are two ASCII characters, so the one can be turned into the other in place.
void to_edit_prompt(std::FILE* out) { std::print(out, "\b\b: "); }
void to_navigation_prompt(std::FILE* out) { std::print(out, "\b\b> "); }

// Moves back over one code point on the terminal and blanks it
void unecho(std::FILE* out) { std::print(out, "\b \b"); }

// Erases the last code point of the input. Navigating starts again when it becomes empty.
void erase_one(InputBuffer& input, std::FILE* out) {
  input.erase();
  unecho(out);
  if (input.empty()) to_navigation_prompt(out);
}

// Empties the input, and navigating starts again
void erase_all(InputBuffer& input, std::FILE* out) {
  for (std::size_t i = 0; i < input.size(); ++i) unecho(out);
  input.clear();
  to_navigation_prompt(out);
}

// Handles a key while navigating: the current state first, then the common
// keys. Starts the input with the key when it is no action. False when the
// key ends the loop.
bool navigate(CodePoint key, Navigator& navigator, InputBuffer& input, std::FILE* out) {
  if (navigator.press(key)) {
    show(navigator, out);
  } else if (key == back_key) {
    if (navigator.back()) show(navigator, out);
  } else if (key == help_key) {
    show_common_keys(navigator, out);
  } else if (is_enter(key)) {
    show(navigator, out);
  } else if (key == quit_key) {
    return false;
  } else if (input.push(key)) {
    to_edit_prompt(out);
    std::print(out, "{}", utf8_of(key));
  }
  return true;
}

// Handles a key while typing input
void edit(CodePoint key, Navigator& navigator, InputBuffer& input, std::FILE* out) {
  if (is_backspace(key)) {
    erase_one(input, out);
  } else if (key == clear_key) {
    erase_all(input, out);
  } else if (is_enter(key)) {
    auto const result = navigator.current().on_input(input.text());
    if (result) {
      input.clear();
      if (!result->empty()) std::print(out, "\n{}", *result);
      show(navigator, out);
    } else {
      std::print(out, "\n{}\n{}{}", result.error(), edit_prompt_of(navigator), utf8_of(input.text()));
    }
  } else if (input.push(key)) {
    std::print(out, "{}", utf8_of(key));
  }
}

// Handles a function key while typing input. Function keys do nothing while navigating.
void edit(FunctionKey key, InputBuffer& input, std::FILE* out) {
  if (key == FunctionKey::escape) {
    erase_all(input, out);
  } else if (key == FunctionKey::del) {
    erase_one(input, out); // the cursor is always at the end
  }
}

} // namespace

bool is_quit(CodePoint code_point) {
  return code_point == 0x03 || code_point == 0x04 || code_point == 0x1A;
}

bool is_enter(CodePoint code_point) { return code_point == U'\n' || code_point == U'\r'; }

bool is_backspace(CodePoint code_point) { return code_point == 0x7F || code_point == 0x08; }

std::vector<CommonKey> common_keys(Navigator const& navigator) {
  std::vector<CommonKey> keys;
  if (navigator.can_go_back()) keys.push_back({utf8_of(back_key), "Tillbaka"});
  keys.push_back({utf8_of(help_key), "Visa gemensamma val"});
  keys.push_back({"Enter", "Visa valen igen"});
  keys.push_back({"Esc", "Töm indata (även Ctrl-U)"});
  keys.push_back({utf8_of(quit_key), "Avsluta"});
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

std::string prompt_of(Navigator const& navigator) { return path_of(navigator) + "> "; }

std::string edit_prompt_of(Navigator const& navigator) { return path_of(navigator) + ": "; }

void show(Navigator const& navigator, std::FILE* out) {
  std::print(out, "\n");
  for (auto const& [key, caption] : navigator.current().options()) {
    std::print(out, "  {} {}\n", utf8_of(key), caption);
  }
  std::print(out, "{}", prompt_of(navigator));
  std::fflush(out);
}

void run_prompt_loop(KeyReader& keys, Navigator& navigator, std::FILE* out) {
  KeyParser parser(keys);
  InputBuffer input;
  show(navigator, out);
  while (auto const key = parser.next()) {
    if (auto const* code_point = std::get_if<CodePoint>(&*key)) {
      if (is_quit(*code_point)) break;
      if (input.empty()) {
        if (!navigate(*code_point, navigator, input, out)) break;
      } else {
        edit(*code_point, navigator, input, out);
      }
    } else if (!input.empty()) {
      edit(std::get<FunctionKey>(*key), input, out);
    }
    std::fflush(out);
  }
  std::print(out, "\n");
  std::fflush(out);
}

} // namespace pyroceph::tui
