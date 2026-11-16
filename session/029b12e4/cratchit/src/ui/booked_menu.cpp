#include "ui/booked_menu.hpp"

#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/log.hpp"
#include "pyroceph/tui/pager.hpp"
#include "ui/help_topics.hpp"
#include "ui/year_view.hpp"

#include <format>
#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

constexpr CodePoint back_key = U'<';
constexpr CodePoint forward_key = U'>';

class BookedState : public State {
public:
  BookedState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) { read(); }
  std::string name() const override { return "Bokfört"; }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    if (pager_.count() == 0) return {std::format("Bokfört {}: inget bokfört i räkenskapsåret.", label_of(year_))};
    View lines{std::format("Bokfört {}: verifikationer {}–{} av {}", label_of(year_), pager_.first() + 1,
                           pager_.first() + pager_.shown(), pager_.count())};
    auto const page = pager_.view();
    lines.insert(lines.end(), page.begin(), page.end());
    return lines;
  }
  // Always both, so that turning past either end does nothing (and starts no input)
  OptionsList options() const override { return {{back_key, "Föregående sida"}, {forward_key, "Nästa sida"}}; }
  // < and > turn the page in place (keys match exactly, beslut.md)
  bool on_key_in_place(CodePoint key) override {
    if (key == back_key) return pager_.back();
    if (key == forward_key) return pager_.forward();
    return false;
  }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  void on_return() override { read(); }
  std::string help_topic() const override { return std::string(help_topic::booked); }

private:
  void read() {
    error_.clear();
    auto const books = books::read_books(project_.folder);
    auto const start = parse_iso_date(year_.start);
    if (!books) {
      LOG_BUSINESS("Kunde inte läsa bokföringen i {}: {}", project_.folder.string(), books.error());
      error_ = books.error();
      pager_.set({});
      return;
    }
    pager_.set(voucher_rows(start ? books->year_starting(*start) : nullptr));
  }

  Project project_;
  FiscalYear year_;
  pyroceph::tui::Pager pager_{booked_page_rows};
  std::string error_;
};

} // namespace

std::unique_ptr<State> make_booked_menu(Project project, FiscalYear year) {
  return std::make_unique<BookedState>(std::move(project), std::move(year));
}

} // namespace cratchit::ui
