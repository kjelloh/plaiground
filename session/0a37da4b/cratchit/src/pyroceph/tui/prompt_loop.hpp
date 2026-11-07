#pragma once

#include "pyroceph/tui/key_reader.hpp"

#include <cstdio>
#include <functional>
#include <optional>

namespace pyroceph::tui {

// A choice a key can be mapped to. Empty for now: what a choice holds is
// decided in a later step.
struct Choice {};

// The choice a code point is mapped to, if any
using Lookup = std::function<std::optional<Choice>(CodePoint)>;

// True for the keys that end the prompt loop: Ctrl-C (U+0003), Ctrl-D (U+0004)
// and Ctrl-Z (U+001A), since RawMode makes them come as characters
bool is_quit(CodePoint code_point);

// Reads one code point at a time, looks it up and writes one ASCII line per
// key to `out`, as "U+00E5 (inget val)". Ends at a quit key or when the
// input ends.
void run_prompt_loop(KeyReader& keys, Lookup const& lookup, std::FILE* out);

} // namespace pyroceph::tui
