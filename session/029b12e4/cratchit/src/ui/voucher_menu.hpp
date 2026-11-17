#pragma once

#include "core/books/books.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace cratchit::ui {

// The postings shown at a time in "Konteringar", with keys 0–9
inline constexpr std::size_t postings_page_rows = 10;

// A booked voucher, named by its id in the path (e.g. "A12"; see voucher_id).
// The view has three parts (beslut.md):
// * how it is booked: its series with series_name (just the series when the
//   name is empty), the day it was registered and the signature (when known);
// * "Händelse": its date, heading and amount;
// * "Konteringar": its postings, one per line: account, account name (from
//   accounts), amount, the posting's own text and date when it has them, and
//   "struken" or "tillagd" for a struck or added posting.
// Options 0 "Händelse" and 1 "Konteringar" lead to those parts.
// * The event of a booked voucher is locked (beslut.md): its options (date,
//   heading, amount) lead to "Låst (<option>)", which says so and tells what
//   can be done instead, depending on whether the voucher is the last in its
//   series (last_in_series, the id of the last one in the fiscal year).
// * The options of a posting (account, description, amount) lead to Inte än
//   (make_design_insufficiency_state) for now.
// To grow into the voucher editor.
std::unique_ptr<pyroceph::tui::State> make_voucher_menu(books::Voucher voucher, std::vector<books::Account> accounts,
                                                        std::string series_name, std::string last_in_series);

} // namespace cratchit::ui
