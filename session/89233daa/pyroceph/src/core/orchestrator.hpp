#pragma once

#include "charset/unicode.hpp"

namespace pyroceph::core {

// What the front end shall do after a key
enum class KeyResult { Continue, Quit };

// Decides what each key means. Knows nothing about the terminal or the screen.
class Orchestrator {
public:
  KeyResult on_key(charset::unicode::CodePoint key);
};

} // namespace pyroceph::core
