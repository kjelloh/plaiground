#pragma once

#include "vat_return.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

namespace cratchit {

// Parses eskd file content (eSKDUpload 6.0, ISO-8859-1 bytes) into a VAT return.
// Used to compare cratchit's output with VAT returns that were filed earlier.
std::expected<VatReturn, std::string> parse_eskd(std::string_view latin1_text);

// Reads and parses an eskd file.
std::expected<VatReturn, std::string> read_eskd_file(std::filesystem::path const& path);

} // namespace cratchit
