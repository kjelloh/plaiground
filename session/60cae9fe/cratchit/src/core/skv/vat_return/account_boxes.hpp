#pragma once

#include "core/skv/vat_return/box.hpp"

#include <expected>
#include <map>
#include <string>
#include <string_view>

// The boxes accounts report in, as built into the program from
// resources/momsrapport/bas_to_momsrapport.csv. Behind box_of; here for the tests.
namespace cratchit::skv::vat_return {

// The built-in text: one line per account, `account;box;code`, e.g. `2611;10;U1`
std::string_view account_boxes_text();

// The box of each account in such text. A UTF-8 byte order mark, CR before LF
// and blank lines are skipped; the code is not read. Error text names the line
// whose account is not four digits, whose box is not two digits, or whose
// account is listed before.
std::expected<std::map<std::string, Box>, std::string> account_boxes(std::string_view text);

} // namespace cratchit::skv::vat_return
