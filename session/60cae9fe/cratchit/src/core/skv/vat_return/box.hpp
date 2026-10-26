#pragma once

#include <optional>
#include <string>
#include <string_view>

// SKV's VAT return (momsdeklaration): the boxes (rutor) of the form and the
// amounts the books report in them for a period
namespace cratchit::skv::vat_return {

// A box of the form by its two-digit number, e.g. "05" or "48"
using Box = std::string;

// The box a BAS account reports in, else nothing
std::optional<Box> box_of(std::string_view account);

} // namespace cratchit::skv::vat_return
