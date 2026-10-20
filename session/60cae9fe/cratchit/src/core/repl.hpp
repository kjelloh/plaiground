#pragma once

#include <functional>
#include <optional>
#include <ostream>

namespace cratchit {

// What the REPL loop should do after a key has been handled
enum class Step { Continue, Quit };

// Handle one key press, writing any response to out
Step on_key(char key, std::ostream& out);

// Supplies the next key press, or nothing at end of input
using KeySource = std::function<std::optional<char>()>;

// Prompt, read a key, handle it, until quit or end of input
void run_repl(KeySource const& next_key, std::ostream& out);

} // namespace cratchit
