#pragma once

#include <array>
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

// The boxes that hold VAT: output VAT (10-12, 30-32, 60-62) and input VAT (48)
inline constexpr std::array<std::string_view, 10> vat_boxes{"10", "11", "12", "30", "31", "32", "60", "61", "62", "48"};

// True for an account that reports in one of vat_boxes
bool is_vat_account(std::string_view account);

// The BAS accounts the VAT of a period is cleared to, from the VAT accounts; SKV
// then charges or credits the tax account against them. Fixed for now; later
// perhaps from a chart of accounts or the box table.
inline constexpr std::string_view vat_liability_account = "2650";  // Redovisningskonto för moms
inline constexpr std::string_view vat_receivable_account = "1650"; // Momsfordran

// True for vat_liability_account and vat_receivable_account
bool is_vat_clearing_account(std::string_view account);

} // namespace cratchit::skv::vat_return
