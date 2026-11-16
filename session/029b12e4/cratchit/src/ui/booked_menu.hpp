#pragma once

#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstddef>
#include <memory>

namespace cratchit::ui {

// The vouchers a page of "Bokfört" shows at most, for now
inline constexpr std::size_t booked_page_rows = 20;

// Bokfört: the fiscal year's vouchers, newest first, a page at a time (see
// voucher_rows and pyroceph::tui::Pager): a heading "Bokfört <year>:
// verifikationer 21–40 av 120", and "…" above and below the page when there
// are more. "<" turns a page back and ">" a page forward, in place. Both are
// always offered; at either end the key does nothing. The vouchers are read again when the user comes
// back. Opened with 1 in Årsflöde. No input.
std::unique_ptr<pyroceph::tui::State> make_booked_menu(Project project, FiscalYear year);

} // namespace cratchit::ui
