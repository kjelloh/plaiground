#pragma once

#include <string>

namespace charset::unicode {

// One Unicode code point
using CodePoint = char32_t;

// A text as Unicode code points
using CodePoints = std::u32string;

} // namespace charset::unicode
