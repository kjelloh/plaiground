#pragma once

// IBM code page 437, the encoding SIE calls #FORMAT PC8

#include <string>
#include <string_view>

namespace cratchit::sie {

std::string cp437_to_utf8(std::string_view cp437);

} // namespace cratchit::sie
