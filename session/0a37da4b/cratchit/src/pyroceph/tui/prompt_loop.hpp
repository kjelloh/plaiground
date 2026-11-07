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

// True for Backspace, which erases the last code point of the input. It comes
// as U+007F on macOS and Linux and as U+0008 from a Windows console.
bool is_backspace(CodePoint code_point);

// Ctrl-U (U+0015) empties the input and goes back to navigating
inline constexpr CodePoint clear_key = 0x15;

// The error text when a state without on_input gets text
inline constexpr std::string_view no_input_error = "Här finns inget att göra med indata.";

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

// As prompt_of but ending in ": ", shown while the user types input
std::string edit_prompt_of(Navigator const& navigator);

// Writes the current state's choices, one per line, then the prompt with no
// line end, as UTF-8. Only the choices that lead to another state are listed,
// not the common keys.
void show(Navigator const& navigator, std::FILE* out);

// Shows the current state, then reads one code point at a time.
//
// Navigating: a choice moves to its state and shows it, back_key goes back
// (nothing at the root), help_key shows the common keys and Enter shows the
// choices again. Any other printable key starts the input with that key.
//
// Typing input: printable keys go into the input and are echoed. Backspace
// erases one; when the input becomes empty, navigating starts again. Ctrl-U
// empties the input and navigating starts again. Enter gives the input to
// the state's on_input: when accepted, the reply is shown, the input is
// emptied and navigating starts again; when not, the error text is shown and
// the input is kept, so the user can correct it.
//
// Ends at a quit key or when the input ends, and then ends the prompt line.
void run_prompt_loop(KeyReader& keys, Navigator& navigator, std::FILE* out);

} // namespace pyroceph::tui
