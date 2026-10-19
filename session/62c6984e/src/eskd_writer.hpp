#pragma once

#include "vat_return.hpp"

#include <expected>
#include <filesystem>
#include <string>

namespace cratchit {

// Produces the eskd file content (eSKDUpload 6.0, ISO-8859-1 bytes) for a VAT return.
// Layout matches files accepted by SKV: XML declaration, DOCTYPE, then the body on one line.
// Fails if the org number is not NNNNNN-NNNN, box 49 is missing, a box is unknown,
// an amount has öre, or the note is longer than 300 characters.
std::expected<std::string, std::string> to_eskd(VatReturn const& vat_return);

// Writes to_eskd() output to a file.
std::expected<void, std::string> write_eskd_file(VatReturn const& vat_return, std::filesystem::path const& path);

} // namespace cratchit
