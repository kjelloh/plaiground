#pragma once

#include "pyroceph/tui/key_reader.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstdio>

namespace pyroceph::tui {

// True for the keys that end the prompt loop: Ctrl-C (U+0003), Ctrl-D (U+0004)
// and Ctrl-Z (U+001A), since RawMode makes them come as characters
bool is_quit(CodePoint code_point);

// Writes the state's name and its choices, one per line, as UTF-8.
// "- Tillbaka" is listed when the navigator can go back.
void show(Navigator const& navigator, std::FILE* out);

// Shows the current state, then reads one code point at a time and presses it
// on the navigator. Shows the new state each time the state changes. Keys
// that are not choices are ignored. Ends at a quit key or when the input ends.
void run_prompt_loop(KeyReader& keys, Navigator& navigator, std::FILE* out);

} // namespace pyroceph::tui
