#pragma once

#include "core/books/books.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>
#include <string>
#include <vector>

namespace cratchit::ui {

// A booked voucher, named by its id in the path (e.g. "A12"; see voucher_id).
// For now it only shows the voucher: its series with series_name (just the
// series when the name is empty), date, heading, amount and the day it was
// registered (when known), then its postings, one per line: account,
// account name (from accounts), amount, the posting's own text and date when
// it has them, and "struken" or "tillagd" for a struck or added posting.
// No options yet: - goes back. To grow into the voucher editor.
std::unique_ptr<pyroceph::tui::State> make_voucher_menu(books::Voucher voucher, std::vector<books::Account> accounts,
                                                        std::string series_name);

} // namespace cratchit::ui
