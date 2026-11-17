#include "ui/draft_menu.hpp"

#include "charset/utf8.hpp"
#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/event_draft.hpp"
#include "core/pdf_documents.hpp"
#include "core/log.hpp"
#include "core/sie_summary.hpp"
#include "core/statement_drafts.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "pyroceph/tui/pager.hpp"
#include "pyroceph/tui/prompt_loop.hpp"
#include "ui/design_insufficiency.hpp"
#include "ui/booked_menu.hpp"
#include "ui/help_topics.hpp"
#include "ui/voucher_menu.hpp"
#include "ui/year_view.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <expected>
#include <filesystem>
#include <functional>
#include <limits>
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

constexpr CodePoint back_key = U'<';
constexpr CodePoint forward_key = U'>';

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

// Reports what was done to the state the TUI goes back to
using EventReport = std::function<void(View)>;

// Adds the event as a draft of year; the error text when it cannot
std::expected<void, std::string> make_event_draft(Project const& project, FiscalYear const& year,
                                                  books::Event const& event) {
  auto const start = parse_iso_date(year.start);
  if (!start) return std::unexpected(std::format("Felaktigt räkenskapsår {}", year.start));
  auto const added = add_event_draft(project.folder, *start, event, today_date());
  if (!added) {
    LOG_BUSINESS("Kunde inte skapa utkast av händelsen {} i {}: {}", event.heading, project.folder.string(),
                 added.error());
    return std::unexpected(std::format("cratchit kunde inte skapa utkastet: {}", added.error()));
  }
  LOG_BUSINESS("Utkast {} skapat i {} ({}): {} {} {}", draft_id(*added), label_of(year), project.folder.string(),
               format_iso_date(event.date), format_amount(event.amount), event.heading);
  return {};
}

// Tells Beskriv händelser that a draft was made of the event
using EventCreated = std::function<void(books::Event const&)>;

class AmountChoiceState : public State {
public:
  AmountChoiceState(Project project, FiscalYear year, std::vector<books::Event> readings, State const* describe,
                    EventCreated created)
      : project_(std::move(project)), year_(std::move(year)), readings_(std::move(readings)), describe_(describe),
        created_(std::move(created)) {}
  std::string name() const override { return "Belopp?"; }
  View view() const override { return {"Beloppet är otydligt: mellanslag kan skilja tusental. Vilket menar du?"}; }
  OptionsList options() const override {
    OptionsList result;
    for (std::size_t i = 0; i < readings_.size() && i < 9; ++i) {
      result.push_back({static_cast<CodePoint>(U'1' + i),
                        std::format("{:>12}  {}", format_amount(readings_[i].amount), readings_[i].heading)});
    }
    return result;
  }
  // A reading makes the draft and goes back to Beskriv händelser (back_to), for the next event
  OnKeyResult on_key(CodePoint key) override {
    if (done_ || key < U'1' || key > U'9') return nullptr;
    auto const i = static_cast<std::size_t>(key - U'1');
    if (i >= readings_.size()) return nullptr;
    if (auto made = make_event_draft(project_, year_, readings_[i]); !made) {
      return pyroceph::tui::make_error_state(made.error());
    }
    if (created_) created_(readings_[i]);
    done_ = true;
    return nullptr;
  }
  State const* back_to() const override { return done_ ? describe_ : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::amount_choice); }

private:
  Project project_;
  FiscalYear year_;
  std::vector<books::Event> readings_;
  State const* describe_;
  EventCreated created_;
  bool done_ = false;
};

// Beskriv händelser: one event after the other, each line a draft (beslut 2026-11-17); the
// user stays here, with an empty line, and sees the drafts made so far. - goes back to Utkast.
class DescribeEventState : public State {
public:
  DescribeEventState(Project project, FiscalYear year, EventReport report)
      : project_(std::move(project)), year_(std::move(year)), report_(std::move(report)) {}
  std::string name() const override { return "Beskriv händelser"; }
  View view() const override {
    View lines{"Skriv händelsen: rubrik, belopp och datum, i vilken ordning som helst, och tryck Enter.",
               "Till exempel: Telia faktura oktober 1118,00 2026-10-22",
               std::format("I dag är {}. Räkenskapsåret är {} – {}.", format_iso_date(today_date()), year_.start,
                           year_.end)};
    if (!created_.empty()) {
      lines.push_back("");
      lines.push_back("Skapade nu:");
      for (auto const& event : created_) {
        lines.push_back(std::format("  {}  {:>12}  {}", format_iso_date(event.date), format_amount(event.amount),
                                    event.heading));
      }
    }
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  // The line typed last, kept when it was refused or the amount was unclear (- from Belopp?
  // comes back to it); empty once a draft is made of it
  std::u32string start_input() const override { return line_; }
  InputResult on_input(std::u32string_view text) override {
    line_ = text;
    auto readings = parse_event(utf8_of(text));
    if (!readings) return std::unexpected(readings.error());
    auto const start = parse_iso_date(year_.start);
    auto const end = parse_iso_date(year_.end);
    auto const date = readings->front().date;
    if (start && end && (date < *start || date > *end)) {
      return std::unexpected(std::format("{} ligger utanför räkenskapsåret {} – {}.", format_iso_date(date),
                                         year_.start, year_.end));
    }
    if (readings->size() > 1) {
      return Accepted(std::make_unique<AmountChoiceState>(project_, year_, std::move(*readings), this,
                                                          [this](books::Event const& event) { created(event); }));
    }
    // A clear line makes the draft at once, without confirmation (beslut 2026-11-17); stay for the next
    if (auto made = make_event_draft(project_, year_, readings->front()); !made) {
      return std::unexpected(made.error());
    }
    created(readings->front());
    return Accepted();
  }
  std::string help_topic() const override { return std::string(help_topic::describe_event); }

private:
  // A draft made of event: listed here, the line emptied, and Utkast told how many so far
  void created(books::Event const& event) {
    created_.push_back(event);
    line_.clear();
    if (report_) {
      report_({created_.size() == 1 ? std::format("cratchit skapade 1 utkast: {}.", event.heading)
                                    : std::format("cratchit skapade {} utkast.", created_.size())});
    }
  }

  Project project_;
  FiscalYear year_;
  EventReport report_;
  std::u32string line_;
  std::vector<books::Event> created_;
};

// What Tab tells about the pdf files of a folder, e.g. "3 pdf-filer i mappen:" and their names
std::vector<std::string> pdf_lines(fs::path const& folder) {
  auto const files = pdf_files_in(folder);
  if (files.empty()) return {"Inga pdf-filer i mappen."};
  std::vector<std::string> lines{files.size() == 1 ? std::string("1 pdf-fil i mappen:")
                                                   : std::format("{} pdf-filer i mappen:", files.size())};
  for (auto const& file : files) {
    auto const name = file.filename().u8string();
    lines.push_back("  " + std::string(name.begin(), name.end()));
  }
  return lines;
}

// Code points in UTF-8 text, for lining up columns
std::size_t width_of(std::string const& text) {
  return static_cast<std::size_t>(
      std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

// The pdf files a list shows its names to at most, before the status
constexpr std::size_t pdf_name_width = 50;

// The pdf files of a folder, numbered, each with what walk_pdf finds in it
// (see status_text): "   1  faktura.pdf          text, 1 sida"
// The pdf files of a folder, each read once (read_pdf) when the folder is chosen, shared
// by Pdf-underlag, Underlag and Tolka
using Documents = std::shared_ptr<std::vector<PdfDocument> const>;

std::vector<std::string> pdf_rows(std::vector<PdfDocument> const& documents) {
  std::vector<std::string> names;
  std::size_t width = 0;
  for (auto const& document : documents) {
    auto const name = document.file.filename().u8string();
    names.emplace_back(name.begin(), name.end());
    width = std::max(width, std::min(width_of(names.back()), pdf_name_width));
  }
  std::vector<std::string> rows;
  for (std::size_t i = 0; i < documents.size(); ++i) {
    auto const shown = width_of(names[i]);
    auto const padding = shown < width ? std::string(width - shown, ' ') : std::string();
    rows.push_back(std::format("  {:>3}  {}{}  {}", i + 1, names[i], padding, status_text(documents[i].status)));
  }
  return rows;
}

// The lines a document shows at most, for now (invoices are short)
constexpr std::size_t document_lines = 20;

// The pages of a pdf file's text, each its lines; none without text
using Pages = std::vector<std::vector<std::string>>;

// The width of the lines that frame a document's text
constexpr std::size_t frame_width = 68;

// A frame line with a label in the middle, e.g. "==== sida 1 av 2 ====", or without one
std::string frame_line(char c, std::string_view label = {}) {
  if (label.empty()) return std::string(frame_width, c);
  auto const text = std::format(" {} ", label);
  auto const left = (frame_width - text.size()) / 2;
  return std::string(left, c) + text + std::string(frame_width - left - text.size(), c);
}

// The lines inside the frame: each page's lines, a "---- sida n av m ----" line between pages
std::vector<std::string> framed_body(Pages const& pages) {
  std::vector<std::string> body;
  for (std::size_t page = 0; page < pages.size(); ++page) {
    if (page > 0) body.push_back(frame_line('-', std::format("sida {} av {}", page + 1, pages.size())));
    body.insert(body.end(), pages[page].begin(), pages[page].end());
  }
  return body;
}

// A document's text in its frame, with a blank line before: "==== sida 1 av m ====", at most
// max_lines lines of the body, "====", then "… N rader till" when cut. Nothing without pages.
std::vector<std::string> framed_text(Pages const& pages, std::size_t max_lines) {
  if (pages.empty()) return {};
  auto const body = framed_body(pages);
  std::vector<std::string> lines{"", frame_line('=', std::format("sida 1 av {}", pages.size()))};
  auto const shown = std::min(body.size(), max_lines);
  lines.insert(lines.end(), body.begin(), body.begin() + static_cast<std::ptrdiff_t>(shown));
  lines.push_back(frame_line('='));
  if (shown < body.size()) lines.push_back(std::format("… {} rader till", body.size() - shown));
  return lines;
}

std::size_t line_count(Pages const& pages) {
  std::size_t count = 0;
  for (auto const& page : pages) count += page.size();
  return count;
}

// The name, cut to at most max code points with "…" at the end
std::string cut_name(std::string const& name, std::size_t max) {
  if (width_of(name) <= max) return name;
  std::string cut;
  std::size_t count = 0;
  for (std::size_t i = 0; i < name.size(); ++i) {
    bool const starts = (static_cast<unsigned char>(name[i]) & 0xC0) != 0x80;
    if (starts && ++count > max - 1) break;
    cut += name[i];
  }
  return cut + "…";
}

std::string file_name_of(fs::path const& file) {
  auto const name = file.filename().u8string();
  return {name.begin(), name.end()};
}

// Tolka: the whole text of a document, to make an event of it later. No options
// for now: - goes back to the document.
class InterpretState : public State {
public:
  InterpretState(Documents documents, std::size_t index) : documents_(std::move(documents)), index_(index) {}
  std::string name() const override { return "Tolka"; }
  View view() const override {
    auto const& document = (*documents_)[index_];
    auto const& pages = document.pages;
    View lines{std::format("Tolka {} ({}{})", file_name_of(document.file), status_text(document.status),
                           pages.empty() ? std::string() : std::format(", {} rader", line_count(pages)))};
    if (pages.empty()) {
      lines.push_back("");
      lines.push_back("Ingen text att tolka.");
    } else {
      auto const text = framed_text(pages, std::numeric_limits<std::size_t>::max());
      lines.insert(lines.end(), text.begin(), text.end());
    }
    lines.push_back("");
    lines.push_back("Att göra en händelse av underlaget finns inte än.");
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::interpret); }

private:
  Documents documents_;
  std::size_t index_;
};

// Underlag <n>: the pdf files of a folder one at a time, from the first: its
// text, page by page, at most document_lines lines. < and > go to the one
// before and after (in place; at either end they do nothing). 1 Tolka shows
// its whole text (InterpretState); 0 Glöm leads to Inte än for now.
class DocumentState : public State {
public:
  explicit DocumentState(Documents documents) : documents_(std::move(documents)) {}
  std::string name() const override { return std::format("Underlag {}", index_ + 1); }
  View view() const override {
    auto const& document = (*documents_)[index_];
    View lines{std::format("Underlag {} av {}: {} ({})", index_ + 1, documents_->size(), file_name_of(document.file),
                           status_text(document.status))};
    if (document.pages.empty()) {
      lines.push_back("");
      lines.push_back("Ingen text att visa.");
    } else {
      auto const text = framed_text(document.pages, document_lines);
      lines.insert(lines.end(), text.begin(), text.end());
    }
    lines.push_back("");
    return lines;
  }
  OptionsList options() const override {
    return {{back_key, "Föregående"}, {U'0', "Glöm"}, {U'1', "Tolka"}, {forward_key, "Nästa"}};
  }
  bool on_key_in_place(CodePoint key) override {
    if (key == back_key && index_ > 0) {
      --index_;
      return true;
    }
    if (key == forward_key && index_ + 1 < documents_->size()) {
      ++index_;
      return true;
    }
    return false;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'0') return make_design_insufficiency_state("Glöm", "att glömma ett underlag");
    if (key == U'1') return std::make_unique<InterpretState>(documents_, index_);
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::document); }

private:
  Documents documents_;
  std::size_t index_ = 0;
};

// Pdf-underlag: the pdf files of a folder, numbered, each with a status line
// telling whether walk_pdf finds text in it; a page at a time. 0 Tolka som
// händelser goes through them one at a time (DocumentState).
class PdfListState : public State {
public:
  PdfListState(fs::path folder, Documents documents) : folder_(std::move(folder)), documents_(std::move(documents)) {
    pager_.set(pdf_rows(*documents_));
  }
  std::string name() const override { return "Pdf-underlag"; }
  View view() const override {
    if (documents_->empty()) return {std::format("Inga pdf-filer i {}.", folder_.string())};
    View lines{std::format("Pdf-underlag i {}: {}", folder_.string(),
                           documents_->size() == 1 ? std::string("1 fil")
                                                   : std::format("{} filer", documents_->size()))};
    auto const page = pager_.view();
    lines.insert(lines.end(), page.begin(), page.end());
    return lines;
  }
  OptionsList options() const override {
    if (documents_->empty()) return {};
    return {{U'0', "Tolka som händelser"}, {back_key, "Föregående sida"}, {forward_key, "Nästa sida"}};
  }
  bool on_key_in_place(CodePoint key) override {
    if (key == back_key) return pager_.back();
    if (key == forward_key) return pager_.forward();
    return false;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key != U'0' || documents_->empty()) return nullptr;
    return std::make_unique<DocumentState>(documents_); // the text read with the list: nothing is read again
  }
  std::string help_topic() const override { return std::string(help_topic::pdf_list); }

private:
  fs::path folder_;
  Documents documents_;
  pyroceph::tui::Pager pager_{booked_page_rows};
};

class ReadPdfState : public State {
public:
  ReadPdfState(Project project, FiscalYear year, std::optional<fs::path> start_folder,
               std::function<void(fs::path const&)> remember)
      : project_(std::move(project)), year_(std::move(year)), chosen_folder_(std::move(start_folder)),
        remember_(std::move(remember)) {}
  std::string name() const override { return "Läs pdf-underlag"; }
  View view() const override {
    return {"Skriv sökvägen till mappen med pdf-underlagen och tryck Enter. Tab fyller i mappar och visar mappens "
            "pdf-filer."};
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::u32string start_input() const override {
    return pyroceph::tui::path_input_of(chosen_folder_.value_or(project_.folder));
  }
  // Folders only; the pdf files of the folder reached are shown as information, not to choose
  Completion complete(std::u32string_view text) const override {
    auto completion = pyroceph::tui::complete_path(text, false);
    std::u32string completed(text);
    completed += completion.added;
    auto const path = pyroceph::tui::path_of(completed);
    auto const ends_in_folder = !completed.empty() && pyroceph::tui::path_input_of(path).back() == completed.back();
    auto const folder = ends_in_folder ? path : path.parent_path();
    std::error_code ec;
    if (!fs::is_directory(folder, ec)) return completion;
    auto const lines = pdf_lines(folder);
    if (!completion.candidates.empty()) {
      completion.candidates.push_back("");
      completion.candidates.insert(completion.candidates.end(), lines.begin(), lines.end());
    } else {
      completion.notes.insert(completion.notes.end(), lines.begin(), lines.end());
    }
    return completion;
  }
  InputResult on_input(std::u32string_view text) override {
    auto folder = pyroceph::tui::path_of(text);
    if (folder.empty()) return std::unexpected(std::string("Skriv sökvägen till mappen."));
    if (!folder.has_filename() && folder.has_parent_path() && folder != folder.root_path()) {
      folder = folder.parent_path(); // without the separator at the end
    }
    std::error_code ec;
    if (fs::is_regular_file(folder, ec)) {
      return std::unexpected(std::format("{} är en fil. Välj mappen den ligger i.", folder.string()));
    }
    if (!fs::is_directory(folder, ec)) return std::unexpected(std::format("Det finns ingen mapp {}.", folder.string()));
    chosen_folder_ = folder;
    if (remember_) remember_(folder);
    // Each file read once, its status and text kept for the list, Underlag and Tolka
    std::vector<PdfDocument> documents;
    std::size_t with_text = 0;
    auto const files = pdf_files_in(folder);
    for (std::size_t i = 0; i < files.size(); ++i) {
      // The name cut, so that the line fits a narrow terminal and can be written over
      pyroceph::tui::show_progress(
          std::format("Läser pdf-filer: {} av {} ({})", i + 1, files.size(), cut_name(file_name_of(files[i]), 40)));
      documents.push_back(read_pdf(files[i]));
      with_text += documents.back().status.kind == PdfStatus::Kind::text ? 1 : 0;
    }
    if (!files.empty()) {
      pyroceph::tui::show_progress(std::format("Läste {} pdf-filer, {} med text.", files.size(), with_text));
    }
    LOG_BUSINESS("Pdf-underlag lästa för {} i {}: {} ({} pdf-filer, {} med text)", label_of(year_),
                 project_.folder.string(), folder.string(), documents.size(), with_text);
    return Accepted(std::make_unique<PdfListState>(
        folder, std::make_shared<std::vector<PdfDocument> const>(std::move(documents))));
  }
  std::string help_topic() const override { return std::string(help_topic::read_pdf); }

private:
  Project project_;
  FiscalYear year_;
  std::optional<fs::path> chosen_folder_;
  std::function<void(fs::path const&)> remember_;
};

class DraftState : public State {
public:
  DraftState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) { read(); }
  std::string name() const override { return "Utkast"; }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    View lines;
    if (pager_.count() == 0) {
      lines.push_back(std::format("Utkast {}: inga utkast.", label_of(year_)));
    } else {
      lines.push_back(std::format("Utkast {}: utkast {}–{} av {}", label_of(year_), pager_.first() + 1,
                                  pager_.first() + pager_.shown(), pager_.count()));
      auto const page = pager_.view();
      lines.insert(lines.end(), page.begin(), page.end());
    }
    lines.insert(lines.end(), report_.begin(), report_.end());
    return lines;
  }
  // < and > always, so that turning past either end does nothing (and starts no input)
  OptionsList options() const override {
    return {{U'0', "Beskriv händelser"},
            {U'1', "Läs kontoutdrag"},
            {U'2', "Läs pdf-underlag"},
            {back_key, "Föregående sida"},
            {forward_key, "Nästa sida"}};
  }
  // < and > turn the page in place (keys match exactly, beslut.md)
  bool on_key_in_place(CodePoint key) override {
    if (key == back_key) return pager_.back();
    if (key == forward_key) return pager_.forward();
    return false;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'2') {
      report_.clear();
      return std::make_unique<ReadPdfState>(project_, year_, folder_,
                                            [this](fs::path const& folder) { folder_ = folder; });
    }
    if (key == U'0') {
      report_.clear();
      return std::make_unique<DescribeEventState>(project_, year_, [this](View text) { report_ = std::move(text); });
    }
    if (key != U'1') return nullptr;
    report_.clear();
    return make_read_statement_menu(
        project_, year_, this,
        [this](View text, fs::path const& file) {
          report_ = std::move(text);
          folder_ = file.parent_path();
        },
        folder_);
  }
  // A draft's id as one word, e.g. #3, opens it (its place in the list, oldest first)
  InputResult on_input(std::u32string_view text) override {
    auto const typed = utf8_of(text);
    std::size_t number = 0;
    auto const digits = std::string_view{typed}.substr(typed.starts_with('#') ? 1 : 0);
    auto const [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), number);
    if (!typed.starts_with('#') || digits.empty() || error != std::errc{} || end != digits.data() + digits.size() ||
        number == 0) {
      return std::unexpected(std::string("Skriv utkastets id som #13."));
    }
    auto const books = books::read_books(project_.folder);
    if (!books) return std::unexpected(std::format("cratchit kunde inte läsa bokföringen: {}", books.error()));
    auto const start = parse_iso_date(year_.start);
    auto const* year = start ? books->year_starting(*start) : nullptr;
    if (year == nullptr || number > year->drafts.size()) {
      return std::unexpected(std::format("Det finns inget utkast {} i {}.", typed, label_of(year_)));
    }
    report_.clear();
    auto const index = number - 1;
    auto const& draft = year->drafts[index];
    auto const id = draft_id(index);
    // The draft as the list shows it, without the id: date, amount and text
    auto summary = draft_rows(year)[year->drafts.size() - 1 - index];
    summary = summary.substr(summary.find(format_iso_date(draft.event.date)));
    auto forget = [this, index, draft, id]() -> std::expected<void, std::string> {
      auto const year_start = parse_iso_date(year_.start);
      if (!year_start) return std::unexpected(std::format("Felaktigt räkenskapsår {}", year_.start));
      if (auto forgotten = forget_draft(project_.folder, *year_start, index, draft); !forgotten) {
        LOG_BUSINESS("Kunde inte glömma utkast {} i {} ({}): {}", id, label_of(year_), project_.folder.string(),
                     forgotten.error());
        return forgotten;
      }
      LOG_BUSINESS("Utkast {} glömt i {} ({}): {} {} \"{}\", {} konteringar", id, label_of(year_), project_.folder.string(),
                   format_iso_date(draft.event.date), format_amount(draft.event.amount), draft.event.heading,
                   draft.postings.size());
      for (auto const& posting : draft.postings) {
        LOG_BUSINESS("  glömd kontering: {} {} {} \"{}\" {}", posting.account, format_amount(posting.amount),
                     posting.date ? format_iso_date(*posting.date) : std::string(), posting.text, posting.source);
      }
      report_ = {std::format("cratchit glömde utkast {}: {}.", id, draft_text(draft))};
      return {};
    };
    return Accepted(make_draft_view(draft, id, label_of(year_), books->accounts, summary, this, std::move(forget)));
  }
  void on_return() override { read(); }
  std::string help_topic() const override { return std::string(help_topic::drafts); }

private:
  void read() {
    error_.clear();
    auto const books = books::read_books(project_.folder);
    if (!books) {
      LOG_BUSINESS("Kunde inte läsa bokföringen i {}: {}", project_.folder.string(), books.error());
      error_ = books.error();
      pager_.set({});
      return;
    }
    auto const start = parse_iso_date(year_.start);
    pager_.set(draft_rows(start ? books->year_starting(*start) : nullptr));
  }

  Project project_;
  FiscalYear year_;
  pyroceph::tui::Pager pager_{booked_page_rows};
  std::string error_;
  View report_; // what was just done, shown until the user goes on from here
  std::optional<fs::path> folder_; // of the statement or pdf files read last: they often lie together
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
