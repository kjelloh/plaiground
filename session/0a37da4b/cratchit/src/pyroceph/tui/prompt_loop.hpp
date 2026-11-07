#pragma once

#include "pyroceph/tui/key_reader.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace pyroceph::tui {

// True for the keys that end the prompt loop: Ctrl-C (U+0003), Ctrl-D (U+0004)
// and Ctrl-Z (U+001A), since RawMode makes them come as characters
bool is_quit(CodePoint code_point);

// The key that lists the common keys. A choice with this key cannot be reached.
inline constexpr CodePoint help_key = U'?';

// True for Enter, which shows the current state's choices again. Enter comes
// as U+000A on macOS and Linux (the terminal turns CR into LF) and as U+000D
// from a Windows console. A choice with either key cannot be reached.
bool is_enter(CodePoint code_point);

// A key that works the same in every state, as listed by help_key
struct CommonKey {
  std::string key;     // as shown, e.g. "-" or "Ctrl-D"
  std::string caption; // e.g. "Tillbaka"
};

// The common keys that work in the navigator's current state. Back is only
// listed when the navigator can go back.
std::vector<CommonKey> common_keys(Navigator const& navigator);

// Writes the common keys, one per line, then the prompt with no line end
void show_common_keys(Navigator const& navigator, std::FILE* out);

// Goes between the state names in the prompt
inline constexpr std::string_view path_separator = " > ";

// The names of the states from the root to the current one, then "> ",
// e.g. "Huvudmeny > Projekt> "
std::string prompt_of(Navigator const& navigator);

// Writes the current state's choices, one per line, then the prompt with no
// line end, as UTF-8. Only the choices that lead to another state are listed,
// not the common keys.
void show(Navigator const& navigator, std::FILE* out);

// Shows the current state, then reads one code point at a time and presses it
// on the navigator. Shows the new state each time the state changes. help_key
// shows the common keys and Enter shows the choices again, without changing
// the state. Keys that are not choices
// are ignored. Ends at a quit key or when the input ends,
// and then ends the prompt line.
void run_prompt_loop(KeyReader& keys, Navigator& navigator, std::FILE* out);

} // namespace pyroceph::tui
