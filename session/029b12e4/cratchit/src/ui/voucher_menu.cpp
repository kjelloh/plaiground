#include "ui/voucher_menu.hpp"

#include "core/amount.hpp"
#include "core/date.hpp"
#include "ui/help_topics.hpp"

#include <algorithm>
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

// The width of UTF-8 text in the terminal, counted in code points
std::size_t width_of(std::string_view text) {
  return static_cast<std::size_t>(std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

std::string padded(std::string const& text, std::size_t width) {
  auto const used = width_of(text);
  return used < width ? text + std::string(width - used, ' ') : text;
}

class VoucherState : public State {
public:
  VoucherState(books::Voucher voucher, std::vector<books::Account> accounts)
      : voucher_(std::move(voucher)), accounts_(std::move(accounts)) {}
  std::string name() const override { return books::voucher_id(voucher_.series, voucher_.number); }
  View view() const override {
    View lines{"Verifikation " + name()};
    auto const row = [&lines](std::string_view label, std::string const& value) {
      lines.push_back(std::format("  {:<16}{}", std::string(label) + ":", value));
    };
    row("Datum", format_iso_date(voucher_.event.date));
    row("Rubrik", voucher_.event.heading);
    row("Belopp", format_amount(voucher_.event.amount));
    if (voucher_.registered) row("Registrerad", format_iso_date(*voucher_.registered));
    lines.push_back("Konteringar:");
    std::size_t account_width = 0;
    std::size_t name_width = 0;
    for (auto const& posting : voucher_.postings) {
      account_width = std::max(account_width, width_of(posting.account));
      name_width = std::max(name_width, width_of(account_name(posting.account)));
    }
    for (auto const& posting : voucher_.postings) {
      auto line = std::format("  {}  {}  {:>12}", padded(posting.account, account_width),
                              padded(account_name(posting.account), name_width), format_amount(posting.amount));
      if (!posting.text.empty()) line += "  " + posting.text;
      if (posting.date && *posting.date != voucher_.event.date) line += "  " + format_iso_date(*posting.date);
      if (posting.kind == books::Posting::Kind::removed) line += "  (struken)";
      if (posting.kind == books::Posting::Kind::added) line += "  (tillagd)";
      lines.push_back(line);
    }
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::voucher); }

private:
  std::string account_name(std::string const& number) const {
    auto const found = std::ranges::find(accounts_, number, &books::Account::number);
    return found != accounts_.end() ? found->name : std::string();
  }

  books::Voucher voucher_;
  std::vector<books::Account> accounts_;
};

} // namespace

std::unique_ptr<State> make_voucher_menu(books::Voucher voucher, std::vector<books::Account> accounts) {
  return std::make_unique<VoucherState>(std::move(voucher), std::move(accounts));
}

} // namespace cratchit::ui
