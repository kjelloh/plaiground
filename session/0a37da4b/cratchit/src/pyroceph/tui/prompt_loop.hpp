#pragma once

#include "pyroceph/tui/key_reader.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstdio>
#include <string>
#include <string_view>

namespace pyroceph::tui {

// True for the keys that end the prompt loop: Ctrl-C (U+0003), Ctrl-D (U+0004)
// and Ctrl-Z (U+001A), since RawMode makes them come as characters
bool is_quit(CodePoint code_point);

// Goes between the state names in the prompt
inline constexpr std::string_view path_separator = " > ";

// The names of the states from the root to the current one, then "> ",
// e.g. "Huvudmeny > Projekt> "
std::string prompt_of(Navigator const& navigator);

// Writes the current state's choices, one per line, then the prompt with no
// line end, as UTF-8. "- Tillbaka" is listed when the navigator can go back.
void show(Navigator const& navigator, std::FILE* out);

// Shows the current state, then reads one code point at a time and presses it
// on the navigator. Shows the new state each time the state changes. Keys
// that are not choices are ignored. Ends at a quit key or when the input ends,
// and then ends the prompt line.
void run_prompt_loop(KeyReader& keys, Navigator& navigator, std::FILE* out);

} // namespace pyroceph::tui
