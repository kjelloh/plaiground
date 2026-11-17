#include "ui/draft_menu.hpp"

#include "charset/utf8.hpp"
#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/log.hpp"
#include "core/sie_summary.hpp"
#include "core/statement_drafts.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/help_topics.hpp"

#include <algorithm>
#include <expected>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

namespace cratchit::ui {

namespace {

namespace fs = std::filesystem;

using pyroceph::tui::Accepted;
using pyroceph::tui::CodePoint;
using pyroceph::tui::Completion;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

// The new entries Bekräfta lists at most
constexpr std::size_t listed_entries = 20;

std::string utf8_of(std::u32string_view text) {
  std::string result;
  charset::utf8::Encoder encoder;
  for (auto const code_point : text) {
    for (auto const byte : encoder.push(code_point)) result.push_back(static_cast<char>(byte));
  }
  return result;
}

std::u32string u32_of(std::string_view text) { return std::u32string(text.begin(), text.end()); } // ASCII only

// "2026-07-03     -3.70  AVGIFTER NORDEA"
std::string entry_text(statement::Entry const& entry) {
  return std::format("    {}  {:>12}  {}", format_iso_date(entry.date), format_amount(entry.amount), entry.text);
}

// The account with its name, e.g. "1930 Företagskonto"
std::string account_text(books::Books const& books, std::string const& number) {
  auto const* account = books.account(number);
  return account != nullptr && !account->name.empty() ? std::format("{} {}", number, account->name) : number;
}

class ConfirmStatementState : public State {
public:
  ConfirmStatementState(Project project, FiscalYear year, StatementFile statement, StatementDraftPlan plan,
                        std::string account_text, State const* origin, State const* chooser, StatementReport report)
      : project_(std::move(project)), year_(std::move(year)), statement_(std::move(statement)), plan_(std::move(plan)),
        account_text_(std::move(account_text)), origin_(origin), chooser_(chooser), report_(std::move(report)) {}
  std::string name() const override { return "Bekräfta"; }
  View view() const override {
    View lines{std::format("Kontoutdrag: {}", statement_.file.string())};
    auto const row = [&lines](std::string_view label, std::string const& value) {
      lines.push_back(std::format("  {:<16}{}", std::string(label), value));
    };
    row("Konto", account_text_);
    row("Nya utkast", std::to_string(plan_.added.size()));
    row("Redan bokförda", std::to_string(plan_.booked.size()));
    row("Redan utkast", std::to_string(plan_.drafted.size()));
    if (!plan_.outside.empty()) {
      row("Utanför året", std::format("{} (läses inte in)", plan_.outside.size()));
    }
    if (plan_.added.empty()) {
      lines.push_back("Inget nytt att läsa in.");
      return lines;
    }
    lines.push_back("  Nya:");
    auto const shown = std::min(plan_.added.size(), listed_entries);
    for (std::size_t i = 0; i < shown; ++i) lines.push_back(entry_text(plan_.added[i]));
    if (shown < plan_.added.size()) lines.push_back(std::format("    … och {} till", plan_.added.size() - shown));
    lines.push_back(plan_.added.size() == 1 ? "Skapa 1 utkast?" : std::format("Skapa {} utkast?", plan_.added.size()));
    return lines;
  }
  OptionsList options() const override {
    if (done_) return {};
    if (plan_.added.empty()) return {{U'N', "Tillbaka"}};
    return {{U'J', "Ja"}, {U'N', "Nej"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (done_) return nullptr;
    if (key == U'N') { // keys match exactly (beslut.md)
      declined_ = true;
      return nullptr;
    }
    if (key != U'J' || plan_.added.empty()) return nullptr;
    auto const start = parse_iso_date(year_.start);
    if (!start) return pyroceph::tui::make_error_state(std::format("Felaktigt räkenskapsår {}", year_.start));
    auto const added = add_statement_drafts(project_.folder, statement_.file, plan_, *start, today_date());
    if (!added) {
      LOG_BUSINESS("Kunde inte läsa in {} som utkast i {}: {}", statement_.file.string(), project_.folder.string(),
                   added.error());
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte skapa utkasten: {}", added.error()));
    }
    LOG_BUSINESS("Kontoutdrag {} inläst för konto {} i {}: {} nya utkast, {} redan bokförda, {} redan utkast, {} "
                 "utanför året",
                 statement_.file.string(), plan_.account, label_of(year_), *added, plan_.booked.size(),
                 plan_.drafted.size(), plan_.outside.size());
    done_ = true;
    if (report_) {
      report_({std::format("cratchit skapade {} utkast från {} på konto {}.", *added, statement_.file.filename().string(),
                           account_text_)},
              statement_.file);
    }
    return nullptr; // back to Utkast (back_to)
  }
  // Done: back to Utkast. N: back to choose another file.
  State const* back_to() const override {
    if (done_) return origin_;
    return declined_ ? chooser_ : nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::confirm_statement); }

private:
  Project project_;
  FiscalYear year_;
  StatementFile statement_;
  StatementDraftPlan plan_;
  std::string account_text_;
  State const* origin_;
  State const* chooser_;
  StatementReport report_;
  bool done_ = false;
  bool declined_ = false;
};

class StatementAccountState : public State {
public:
  StatementAccountState(Project project, FiscalYear year, StatementFile statement, books::Books books,
                        State const* origin, State const* chooser, StatementReport report)
      : project_(std::move(project)), year_(std::move(year)), statement_(std::move(statement)), books_(std::move(books)),
        origin_(origin), chooser_(chooser), report_(std::move(report)),
        guess_(guess_statement_account(books_, statement_.document)) {}
  std::string name() const override { return "Konto"; }
  View view() const override {
    auto const& document = statement_.document;
    View lines{std::format("Kontoutdrag: {}", statement_.file.string())};
    auto const row = [&lines](std::string_view label, std::string const& value) {
      lines.push_back(std::format("  {:<10}{}", std::string(label), value));
    };
    row("Kodning", document.encoding);
    auto const order = document.order == statement::Order::newest_first   ? ", nyast först"
                       : document.order == statement::Order::oldest_first ? ", äldst först"
                                                                          : "";
    row("Rader", std::format("{}, {} – {} (rad {}–{}{})", document.entries.size(),
                             format_iso_date(document.entries.front().date),
                             format_iso_date(document.entries.back().date), document.first_line, document.last_line,
                             order));
    if (document.saldo_column && document.opening && document.closing) {
      row("Saldo", std::format("{} före, {} efter, stämmer på varje rad som har saldo", format_amount(*document.opening),
                               format_amount(*document.closing)));
    }
    for (auto const& note : document.notes) lines.push_back("  " + note);
    if (guess_) {
      lines.push_back(std::format("Förslag: {} ({} av {} rader stämmer med konteringar på {} i bokföringen).",
                                  account_text(books_, guess_->account), guess_->matches, document.entries.size(),
                                  guess_->account));
    }
    lines.push_back("Skriv BAS-kontot som kontoutdraget gäller och tryck Enter.");
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::u32string start_input() const override { return guess_ ? u32_of(guess_->account) : std::u32string(); }
  InputResult on_input(std::u32string_view text) override {
    auto const typed = utf8_of(text);
    auto const first = typed.find_first_not_of(' ');
    auto const number = first == std::string::npos ? std::string() : typed.substr(first, typed.find_last_not_of(' ') - first + 1);
    if (number.empty()) return std::unexpected(std::string("Skriv kontots nummer, till exempel 1930."));
    if (books_.account(number) == nullptr) {
      return std::unexpected(std::format("Konto {} finns inte i bokföringens kontoplan.", number));
    }
    auto const start = parse_iso_date(year_.start);
    auto const* year = start ? books_.year_starting(*start) : nullptr;
    if (year == nullptr) return std::unexpected(std::format("Räkenskapsåret {} finns inte i bokföringen.", label_of(year_)));
    auto plan = plan_statement_drafts(*year, statement_.document, number);
    LOG_BUSINESS("Kontoutdrag {}: konto {} valt", statement_.file.string(), number);
    return Accepted(std::make_unique<ConfirmStatementState>(project_, year_, statement_, std::move(plan),
                                                            account_text(books_, number), origin_, chooser_, report_));
  }
  std::string help_topic() const override { return std::string(help_topic::statement_account); }

private:
  Project project_;
  FiscalYear year_;
  StatementFile statement_;
  books::Books books_;
  State const* origin_;
  State const* chooser_;
  StatementReport report_;
  std::optional<AccountGuess> guess_;
};

class ReadStatementState : public State {
public:
  ReadStatementState(Project project, FiscalYear year, State const* origin, StatementReport report,
                     std::optional<fs::path> start_folder)
      : project_(std::move(project)), year_(std::move(year)), origin_(origin), report_(std::move(report)),
        chosen_folder_(std::move(start_folder)) {}
  std::string name() const override { return "Läs kontoutdrag"; }
  View view() const override {
    return {"Skriv sökvägen till kontoutdraget och tryck Enter. Tab fyller i mappar och filer."};
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  // The folder of the file chosen last, so that going back offers its neighbours
  std::u32string start_input() const override {
    return pyroceph::tui::path_input_of(chosen_folder_.value_or(project_.folder));
  }
  // All folders and files: cratchit does not judge what a statement may be
  Completion complete(std::u32string_view text) const override { return pyroceph::tui::complete_path(text, true); }
  InputResult on_input(std::u32string_view text) override {
    auto const file = pyroceph::tui::path_of(text);
    if (file.empty()) return std::unexpected(std::string("Skriv sökvägen till kontoutdraget."));
    std::error_code ec;
    if (fs::is_directory(file, ec)) {
      return std::unexpected(std::format("{} är en mapp. Välj en fil i den med Tab.", file.string()));
    }
    if (!fs::is_regular_file(file, ec)) return std::unexpected(std::format("Det finns ingen fil {}.", file.string()));
    chosen_folder_ = file.parent_path();
    auto statement = read_statement_file(file);
    if (!statement) {
      LOG_BUSINESS("Kan inte läsa kontoutdrag {}: {}", file.string(), statement.error());
      return std::unexpected(std::format("cratchit kan inte läsa filen som kontoutdrag. {}", statement.error()));
    }
    auto books = books::read_books(project_.folder);
    if (!books) return std::unexpected(std::format("cratchit kunde inte läsa bokföringen: {}", books.error()));
    LOG_BUSINESS("Kontoutdrag läst för {} i {}: {} ({} rader, {})", label_of(year_), project_.folder.string(),
                 file.string(), statement->document.entries.size(), statement->document.encoding);
    return Accepted(std::make_unique<StatementAccountState>(project_, year_, std::move(*statement), std::move(*books),
                                                            origin_, this, report_));
  }
  std::string help_topic() const override { return std::string(help_topic::read_statement); }

private:
  Project project_;
  FiscalYear year_;
  State const* origin_;
  StatementReport report_;
  std::optional<fs::path> chosen_folder_;
};

class DraftState : public State {
public:
  DraftState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) { read(); }
  std::string name() const override { return "Utkast"; }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    View lines{count_ == 0 ? std::format("Utkast {}: inga utkast.", label_of(year_))
                           : std::format("Utkast {}: {} utkast.", label_of(year_), count_)};
    lines.insert(lines.end(), report_.begin(), report_.end());
    return lines;
  }
  OptionsList options() const override { return {{U'0', "Läs kontoutdrag"}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key != U'0') return nullptr;
    report_.clear();
    return make_read_statement_menu(
        project_, year_, this,
        [this](View text, fs::path const& file) {
          report_ = std::move(text);
          statement_folder_ = file.parent_path();
        },
        statement_folder_);
  }
  void on_return() override { read(); }
  std::string help_topic() const override { return std::string(help_topic::drafts); }

private:
  void read() {
    error_.clear();
    count_ = 0;
    auto const books = books::read_books(project_.folder);
    if (!books) {
      LOG_BUSINESS("Kunde inte läsa bokföringen i {}: {}", project_.folder.string(), books.error());
      error_ = books.error();
      return;
    }
    auto const start = parse_iso_date(year_.start);
    auto const* year = start ? books->year_starting(*start) : nullptr;
    if (year != nullptr) count_ = year->drafts.size();
  }

  Project project_;
  FiscalYear year_;
  std::size_t count_ = 0;
  std::string error_;
  View report_; // what was just done, shown until the user goes on from here
  std::optional<fs::path> statement_folder_; // of the statement read last
};

} // namespace

std::unique_ptr<State> make_draft_menu(Project project, FiscalYear year) {
  return std::make_unique<DraftState>(std::move(project), std::move(year));
}

std::unique_ptr<State> make_read_statement_menu(Project project, FiscalYear year, State const* origin,
                                                StatementReport report, std::optional<fs::path> start_folder) {
  return std::make_unique<ReadStatementState>(std::move(project), std::move(year), origin, std::move(report),
                                              std::move(start_folder));
}

} // namespace cratchit::ui
