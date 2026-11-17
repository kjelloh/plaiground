#include "ui/voucher_menu.hpp"

#include "core/amount.hpp"
#include "core/date.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "ui/design_insufficiency.hpp"
#include "ui/help_topics.hpp"
#include "ui/year_view.hpp"

#include <algorithm>
#include <format>
#include <memory>
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

// The width of UTF-8 text in the terminal, counted in code points
std::size_t width_of(std::string_view text) {
  return static_cast<std::size_t>(std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

std::string padded(std::string const& text, std::size_t width) {
  auto const used = width_of(text);
  return used < width ? text + std::string(width - used, ' ') : text;
}

// A label and its value as a row of a view, the values lined up
std::string row(std::string_view label, std::string const& value) {
  return std::format("  {:<16}{}", std::string(label) + ":", value);
}

// What the voucher states share: the voucher and how to name its accounts. A
// draft is shown as a voucher without series and number, with what only a draft has.
struct Shown {
  books::Voucher voucher;
  std::vector<books::Account> accounts;
  std::string series_name;
  std::string last_in_series; // the id of the last voucher in the series, in the fiscal year

  // A draft: its id ("#3"), fiscal year, stage and the day it was noted
  bool draft = false;
  std::string draft_id{};
  std::string year_label{};
  std::string stage{};
  Date noted{};
  std::string summary{};             // one line about it, for Glöm
  State const* origin = nullptr;     // Utkast, to go back to once forgotten
  ForgetDraft forget{};

  std::string id() const { return draft ? draft_id : books::voucher_id(voucher.series, voucher.number); }

  std::string account_name(std::string const& number) const {
    auto const found = std::ranges::find(accounts, number, &books::Account::number);
    return found != accounts.end() ? found->name : std::string();
  }

  // How the voucher is booked: series, registered, signature; for a draft when it was noted
  View booking_rows() const {
    if (draft) return {row("Noterad", format_iso_date(noted))};
    View lines;
    lines.push_back(row("Serie", series_name.empty() ? voucher.series : std::format("{}  {}", voucher.series, series_name)));
    if (voucher.registered) lines.push_back(row("Registrerad", format_iso_date(*voucher.registered)));
    if (!voucher.signature.empty()) lines.push_back(row("Signatur", voucher.signature));
    return lines;
  }

  View event_rows() const {
    // A draft may lack its heading and amount: they show empty
    auto const amount = draft && voucher.event.amount == 0 ? std::string() : format_amount(voucher.event.amount);
    return {row("Datum", format_iso_date(voucher.event.date)), row("Rubrik", voucher.event.heading),
            row("Belopp", amount)};
  }

  // The postings, one line each without indent, the columns lined up over all of them
  std::vector<std::string> posting_lines() const {
    std::size_t account_width = 0;
    std::size_t name_width = 0;
    for (auto const& posting : voucher.postings) {
      account_width = std::max(account_width, width_of(posting.account));
      name_width = std::max(name_width, width_of(account_name(posting.account)));
    }
    std::vector<std::string> lines;
    for (auto const& posting : voucher.postings) {
      auto line = std::format("{}  {}  {:>12}", padded(posting.account, account_width),
                              padded(account_name(posting.account), name_width), format_amount(posting.amount));
      if (!posting.text.empty()) line += "  " + posting.text;
      if (posting.date && *posting.date != voucher.event.date) line += "  " + format_iso_date(*posting.date);
      if (posting.kind == books::Posting::Kind::removed) line += "  (struken)";
      if (posting.kind == books::Posting::Kind::added) line += "  (tillagd)";
      lines.push_back(std::move(line));
    }
    return lines;
  }
};

// One posting of the voucher, named by its number in "Konteringar"
class PostingState : public State {
public:
  PostingState(std::shared_ptr<Shown const> shown, std::size_t index) : shown_(std::move(shown)), index_(index) {}
  std::string name() const override { return std::to_string(index_); }
  View view() const override {
    auto const& posting = shown_->voucher.postings[index_];
    View lines{std::format("Kontering {} i {}", index_, shown_->id())};
    if (posting.date && *posting.date != shown_->voucher.event.date) lines.push_back(row("Datum", format_iso_date(*posting.date)));
    if (posting.kind == books::Posting::Kind::removed) lines.push_back("  Raden är struken.");
    if (posting.kind == books::Posting::Kind::added) lines.push_back("  Raden är tillagd.");
    if (!posting.source.empty()) {
      // A statement entry's file and line, e.g. "bank/nordea.csv:5"
      auto const colon = posting.source.rfind(':');
      lines.push_back(row("Från", colon == std::string::npos
                                      ? posting.source
                                      : std::format("{} rad {}", posting.source.substr(0, colon),
                                                    posting.source.substr(colon + 1))));
    }
    return lines;
  }
  OptionsList options() const override {
    auto const& posting = shown_->voucher.postings[index_];
    auto const name = shown_->account_name(posting.account);
    return {{U'0', "Konto: " + (name.empty() ? posting.account : posting.account + "  " + name)},
            {U'1', posting.text.empty() ? std::string("Beskrivning:") : "Beskrivning: " + posting.text},
            {U'2', "Belopp: " + format_amount(posting.amount)}};
  }
  // Nothing can be changed yet: each option tells so
  OnKeyResult on_key(CodePoint key) override {
    switch (key) {
    case U'0': return make_design_insufficiency_state("Konto", "att ändra kontot i en kontering");
    case U'1': return make_design_insufficiency_state("Beskrivning", "att ändra beskrivningen i en kontering");
    case U'2': return make_design_insufficiency_state("Belopp", "att ändra beloppet i en kontering");
    default: return nullptr;
    }
  }
  std::string help_topic() const override { return std::string(help_topic::posting); }

private:
  std::shared_ptr<Shown const> shown_;
  std::size_t index_;
};

// The voucher's postings, 0–9 a page at a time
class PostingsState : public State {
public:
  explicit PostingsState(std::shared_ptr<Shown const> shown) : shown_(std::move(shown)) {}
  std::string name() const override { return "Konteringar"; }
  View view() const override {
    auto const count = shown_->voucher.postings.size();
    if (count == 0) return {std::format("{} har inga konteringar.", shown_->id())};
    if (count <= postings_page_rows) return {std::format("Konteringar i {}", shown_->id())};
    return {std::format("Konteringar i {}: {}–{} av {}", shown_->id(), first() + 1, first() + shown(), count)};
  }
  OptionsList options() const override {
    auto const lines = shown_->posting_lines();
    OptionsList options;
    for (std::size_t i = 0; i < shown(); ++i) {
      options.emplace_back(static_cast<CodePoint>(U'0' + i), lines[first() + i]);
    }
    if (lines.size() > postings_page_rows) {
      options.emplace_back(back_key, "Föregående sida");
      options.emplace_back(forward_key, "Nästa sida");
    }
    return options;
  }
  // < and > turn the page in place
  bool on_key_in_place(CodePoint key) override {
    if (key == back_key && page_ > 0) {
      --page_;
      return true;
    }
    if (key == forward_key && first() + postings_page_rows < shown_->voucher.postings.size()) {
      ++page_;
      return true;
    }
    return false;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key < U'0' || key > U'9') return nullptr;
    auto const i = static_cast<std::size_t>(key - U'0');
    if (i >= shown()) return nullptr;
    return std::make_unique<PostingState>(shown_, first() + i);
  }
  std::string help_topic() const override { return std::string(help_topic::postings); }

private:
  std::size_t first() const { return page_ * postings_page_rows; }
  std::size_t shown() const { return std::min(postings_page_rows, shown_->voucher.postings.size() - first()); }

  std::shared_ptr<Shown const> shown_;
  std::size_t page_ = 0;
};

// Why an option of a booked voucher's event cannot be chosen, and what to do instead
class LockedState : public State {
public:
  LockedState(std::shared_ptr<Shown const> shown, std::string option, std::string what)
      : shown_(std::move(shown)), option_(std::move(option)), what_(std::move(what)) {}
  std::string name() const override { return std::format("Låst ({})", option_); }
  View view() const override {
    auto const& id = shown_->id();
    auto const& series = shown_->voucher.series;
    View lines{std::format("Det går inte att ändra {} i en bokförd verifikation.", what_)};
    if (id == shown_->last_in_series) {
      lines.push_back(std::format("{} är den sista verifikationen i serie {}. Den sista i en serie går att göra om till "
                                  "ett utkast, där du kan ändra händelsen fritt och sedan bokföra den igen. Det har "
                                  "utvecklaren av cratchit inte infört ännu.",
                                  id, series));
    } else {
      auto line = std::string("Bara den sista verifikationen i en serie går att göra om till ett utkast och ändra fritt.");
      if (!shown_->last_in_series.empty()) line += std::format(" I serie {} är det {}.", series, shown_->last_in_series);
      line += std::format(" Är händelsen i {} fel rättar du den med en ny verifikation.", id);
      lines.push_back(std::move(line));
    }
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::locked); }

private:
  std::shared_ptr<Shown const> shown_;
  std::string option_;
  std::string what_;
};

// The voucher's event: date, heading and amount
class EventState : public State {
public:
  explicit EventState(std::shared_ptr<Shown const> shown) : shown_(std::move(shown)) {}
  std::string name() const override { return "Händelse"; }
  View view() const override { return {std::format("Händelsen i {}", shown_->id())}; }
  OptionsList options() const override {
    auto const& event = shown_->voucher.event;
    auto const amount = shown_->draft && event.amount == 0 ? std::string() : format_amount(event.amount);
    return {{U'0', "Datum: " + format_iso_date(event.date)},
            {U'1', event.heading.empty() ? std::string("Rubrik:") : "Rubrik: " + event.heading},
            {U'2', amount.empty() ? std::string("Belopp:") : "Belopp: " + amount}};
  }
  // The event of a booked voucher is locked: each option tells why, and what to do instead.
  // A draft's can be changed, but not yet.
  OnKeyResult on_key(CodePoint key) override {
    if (shown_->draft) {
      switch (key) {
      case U'0': return make_design_insufficiency_state("Datum", "att ändra datumet i ett utkast");
      case U'1': return make_design_insufficiency_state("Rubrik", "att ändra rubriken i ett utkast");
      case U'2': return make_design_insufficiency_state("Belopp", "att ändra beloppet i ett utkast");
      default: return nullptr;
      }
    }
    switch (key) {
    case U'0': return std::make_unique<LockedState>(shown_, "Datum", "datumet");
    case U'1': return std::make_unique<LockedState>(shown_, "Rubrik", "rubriken");
    case U'2': return std::make_unique<LockedState>(shown_, "Belopp", "beloppet");
    default: return nullptr;
    }
  }
  std::string help_topic() const override {
    return std::string(shown_->draft ? help_topic::draft_event : help_topic::event);
  }

private:
  std::shared_ptr<Shown const> shown_;
};

// Glöm: the draft forgotten with 9 again (9 is the option that removes); N goes back
class ForgetState : public State {
public:
  ForgetState(std::shared_ptr<Shown const> shown, State const* draft) : shown_(std::move(shown)), draft_(draft) {}
  std::string name() const override { return "Glöm"; }
  View view() const override {
    return {std::format("Glöm utkast {}?", shown_->id()), "  " + shown_->summary,
            "Utkastet tas bort ur bokföringen och går inte att få tillbaka."};
  }
  OptionsList options() const override {
    if (done_) return {};
    return {{U'9', "Glöm"}, {U'N', "Nej"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (done_) return nullptr;
    if (key == U'N') { // keys match exactly (beslut.md)
      declined_ = true;
      return nullptr;
    }
    if (key != U'9' || !shown_->forget) return nullptr;
    if (auto forgotten = shown_->forget(); !forgotten) {
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte glömma utkastet: {}", forgotten.error()));
    }
    done_ = true;
    return nullptr; // back to Utkast (back_to)
  }
  // Forgotten: back to Utkast, as the draft is gone. N: back to the draft.
  State const* back_to() const override {
    if (done_) return shown_->origin;
    return declined_ ? draft_ : nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::forget_draft); }

private:
  std::shared_ptr<Shown const> shown_;
  State const* draft_;
  bool done_ = false;
  bool declined_ = false;
};

class VoucherState : public State {
public:
  explicit VoucherState(std::shared_ptr<Shown const> shown) : shown_(std::move(shown)) {}
  std::string name() const override { return shown_->id(); }
  View view() const override {
    View lines{shown_->draft ? std::format("Utkast {} i {} ({})", name(), shown_->year_label, shown_->stage)
                             : "Verifikation " + name()};
    auto const booking = shown_->booking_rows();
    lines.insert(lines.end(), booking.begin(), booking.end());
    lines.push_back("Händelse:");
    auto const event = shown_->event_rows();
    lines.insert(lines.end(), event.begin(), event.end());
    lines.push_back("Konteringar:");
    for (auto const& line : shown_->posting_lines()) lines.push_back("  " + line);
    return lines;
  }
  OptionsList options() const override {
    if (shown_->draft) return {{U'0', "Händelse"}, {U'1', "Konteringar"}, {U'9', "Glöm"}};
    return {{U'0', "Händelse"}, {U'1', "Konteringar"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'0') return std::make_unique<EventState>(shown_);
    if (key == U'1') return std::make_unique<PostingsState>(shown_);
    if (key == U'9' && shown_->draft) return std::make_unique<ForgetState>(shown_, this);
    return nullptr;
  }
  std::string help_topic() const override {
    return std::string(shown_->draft ? help_topic::draft : help_topic::voucher);
  }

private:
  std::shared_ptr<Shown const> shown_;
};

} // namespace

std::unique_ptr<State> make_voucher_menu(books::Voucher voucher, std::vector<books::Account> accounts,
                                         std::string series_name, std::string last_in_series) {
  return std::make_unique<VoucherState>(std::make_shared<Shown const>(
      Shown{std::move(voucher), std::move(accounts), std::move(series_name), std::move(last_in_series)}));
}

std::unique_ptr<State> make_draft_view(books::Draft const& draft, std::string id, std::string year_label,
                                       std::vector<books::Account> accounts, std::string summary, State const* origin,
                                       ForgetDraft forget) {
  Shown shown{books::Voucher{"", "", draft.event, std::nullopt, "", draft.postings}, std::move(accounts), "", ""};
  shown.draft = true;
  shown.draft_id = std::move(id);
  shown.year_label = std::move(year_label);
  shown.stage = std::string(stage_text(books::stage(draft)));
  shown.noted = draft.noted;
  shown.summary = std::move(summary);
  shown.origin = origin;
  shown.forget = std::move(forget);
  return std::make_unique<VoucherState>(std::make_shared<Shown const>(std::move(shown)));
}

} // namespace cratchit::ui
