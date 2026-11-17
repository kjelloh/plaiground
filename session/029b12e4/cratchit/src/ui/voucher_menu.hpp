#pragma once

#include "core/books/books.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstddef>
#include <expected>
#include <functional>
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

// A draft, named by its id in the path ("#3", see draft_id), shown as a voucher
// is: the heading "Utkast #3 i 2026/27 (<stage>)", the day it was noted, then
// "Händelse" (a heading or amount it lacks shows empty) and "Konteringar".
// A posting from a statement entry shows where it came from ("Från: <file> rad
// <n>"). Options as for a voucher; the event's and a posting's options lead to
// Inte än for now (in a draft everything is to be changeable).
//
// 9 depends on what the draft is made of (parts_of, beslut 2026-11-17: 9 is
// the option that removes, kept apart from the others):
// * lone: 9 Glöm leads to "Glöm": it shows summary, says the draft cannot be
//   had back, and offers 9 Glöm and N Nej. 9 again calls forget; done, the TUI
//   goes back to origin (State::back_to, Utkast); N goes back to the draft.
// * same: 9 Inte samma calls split at once (it can be undone in Samma?); done,
//   back to origin.
// * more than the same: no 9.
// A failure of forget or split leads to the error state with its text.
using ForgetDraft = std::function<std::expected<void, std::string>()>;
std::unique_ptr<pyroceph::tui::State> make_draft_view(books::Draft const& draft, std::string id,
                                                      std::string year_label, std::vector<books::Account> accounts,
                                                      std::string summary, pyroceph::tui::State const* origin,
                                                      ForgetDraft forget, ForgetDraft split);

// The lines of a draft's view (as make_draft_view shows it, without options),
// with id in its heading, e.g. "Utkast #1 av 18 i 2026/27 (kontohändelse)"
pyroceph::tui::View draft_view_lines(books::Draft const& draft, std::string const& id, std::string year_label,
                                     std::vector<books::Account> accounts);

// "Glöm" for the draft id (e.g. "#3"), as from a draft's view: it shows summary
// (one line about the draft), says the draft cannot be had back, and offers
// 9 Glöm and N Nej. 9 calls forget; done, the TUI goes back to forgotten_to;
// N goes back to declined_to. A failure of forget leads to the error state.
std::unique_ptr<pyroceph::tui::State> make_forget_draft_state(std::string id, std::string summary, ForgetDraft forget,
                                                              pyroceph::tui::State const* forgotten_to,
                                                              pyroceph::tui::State const* declined_to);

} // namespace cratchit::ui
