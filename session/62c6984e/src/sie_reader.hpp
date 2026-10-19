#pragma once

#include "sie_document.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

namespace cratchit {

// Parses SIE text that is already converted to UTF-8.
// Reads #ORGNR, #RAR, #KONTO and #VER/#TRANS. Other records are ignored, and so are
// #BTRANS/#RTRANS (correction history; an #RTRANS is always followed by the same #TRANS).
std::expected<SieDocument, std::string> parse_sie(std::string_view text);

// Reads a SIE file in code page 437 (SIE "#FORMAT PC8") and parses it.
std::expected<SieDocument, std::string> read_sie_file(std::filesystem::path const& path);

} // namespace cratchit
