#pragma once

#include "core/books/books.hpp"
#include "core/books/entry.hpp"
#include "core/books/hint.hpp"
#include "core/books/import.hpp"
#include "core/books/match.hpp"
#include "core/books/statements.hpp"
#include "core/charset/unicode.hpp"
#include "core/help.hpp"
#include "core/item_list.hpp"
#include "core/log_view.hpp"
#include "core/manual.hpp"
#include "core/operations.hpp"
#include "core/pager.hpp"
#include "core/path_editor.hpp"
#include "core/project.hpp"
#include "core/skv/vat_return/document.hpp"
#include "core/workspace.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// What the REPL loop should do after a key has been handled
enum class Step { Continue, Quit };

// Where the workspace is looked up, offered and saved
struct WorkspacePlaces {
  std::filesystem::path runtime_file;                  // holds `workspace = <folder>`
  std::filesystem::path start_folder;                  // the folder cratchit started in
  std::optional<std::filesystem::path> default_folder; // the platform default, if found
  std::string today;                                   // YYYY-MM-DD, for files created
};

// The state of the app between key presses: the main prompt, browsing help,
// viewing the log, the workspace menu, choosing a workspace, in the workspace,
// the project menu, creating or adding a project, in a project, importing an
// SIE file, choosing and reading reports, choosing a fiscal year, in a fiscal
// year, entering a voucher (its template, series and rows), noting an event, or the waiting events and their matches
class Repl {
public:
  // page_lines is the number of help and log lines per page; 0 shows everything at once.
  // log_file is the log the l key shows.
  Repl(Section const& manual, std::size_t page_lines, WorkspacePlaces places, std::filesystem::path log_file = {});

  // Open the workspace named in runtime.txt, or offer folders to choose from
  // when there is none or its folder is missing (on start). The workspace's
  // last opened project is opened too.
  void open_workspace(std::ostream& out);

  // Handle one key press, writing any response to out
  Step on_key(charset::unicode::CodePoint key, std::ostream& out);

  // True for keys that do nothing in the current mode, so are not even echoed
  bool ignores(charset::unicode::CodePoint key) const;

  // True while a path, name, account or voucher line is typed; on_key then writes the prompt and echo itself
  bool is_typing() const;

  std::string prompt() const;

private:
  enum class Mode {
    Main,
    Help,
    Log,
    Menu,
    Choose,
    Typing,
    ConfirmCreate,
    Workspace,
    Projects,
    ProjectList,
    ProjectFolder,
    Project,
    ConfirmImport,
    Years,
    Reports,
    VatPeriods,
    Paging,
    YearMenu,
    Year,
    ChooseAmount,
    ConfirmEvent,
    Template,
    Series,
    Postings,
    Waiting,
    WaitingEvent,
    ConfirmDelete,
    VoucherSeries,
    VoucherList,
    VoucherView,
    ConfirmRight,
    StatementFiles,
    ConfirmStatement,
    EntryList,
    EntryChoice,
    AddToEvent,
    Matches,
    MatchChoice,
    ConfirmBooked,
  };
  // What a number picks at cratchit>, workspace>, project>, the year prompt, and in the menus and views of one item
  enum class Op {
    WorkspaceMenu,
    Projects,
    FiscalYears,
    ImportSie,
    ImportStatement,
    StatementEntries,
    Reports,
    EnterVoucher,
    NoteEvent,
    WaitingEvents,
    ViewVouchers,
    CloseYear,
    OpenYear,
    FindMatches,
    OpenSaved,
    Browse,
    OpenProject,
    NewProject,
    AddProject,
    PostEvent,
    MakeOneEvent,
    CorrectVoucher,
    VoucherAsTemplate,
    NewEventFromEntry,
    AddEntryToEvent,
    SaveVoucher,
    FlipTemplate,
  };
  // What the text typed in the editor is for
  enum class Purpose {
    Workspace,
    ProjectName,
    ProjectFolder,
    ExistingProject,
    SieFile,
    Account,
    Voucher,
    Row,
    StatementFile,
    StatementAccount,
    EventHeading,
  };

  WorkspacePlaces places_;
  std::filesystem::path log_file_;
  HelpBrowser help_;
  LogView log_view_;
  PathEditor editor_;
  Mode mode_ = Mode::Main;
  Mode before_help_ = Mode::Main;
  Mode before_log_ = Mode::Main;
  Mode before_choose_ = Mode::Main;
  std::optional<std::filesystem::path> current_; // the saved workspace offered by the menu, if its folder exists
  std::filesystem::path to_create_;
  Purpose purpose_ = Purpose::Workspace;
  std::optional<Workspace> workspace_;  // the open workspace
  std::vector<ListedProject> listed_;   // the projects in the project menu
  ItemList project_page_;               // the page of the project menu
  std::string new_name_;                // the name of the project being created
  std::optional<Project> project_;      // the open project
  std::optional<books::ImportPlan> plan_; // the import waiting for y/n
  books::ImportSource import_source_;
  books::Books books_;                  // the books the reports are about
  ItemList year_page_;                  // the page of the report years
  std::size_t year_ = 0;                // the year of the reports, an index into books_.years
  Pager pager_;
  Mode before_paging_ = Mode::Reports;  // the menu a paged report goes back to
  std::vector<skv::vat_return::Period> vat_periods_; // the VAT return periods of the report year, oldest first
  ItemList vat_period_page_;            // the page of the VAT return periods
  ItemList year_menu_page_{ItemList::Start::Zero}; // the page of the year menu, keys from 0 as SIE numbers years
  Date open_year_;                      // the start of the fiscal year at the year prompt
  Mode before_reports_ = Mode::Years;   // where - leaves the report menu for
  std::vector<books::Event> readings_;  // the readings of an unclear amount
  std::optional<books::Draft> draft_;   // the voucher being entered, in a year of books_
  std::vector<books::Voucher const*> templates_; // the earlier vouchers offered as templates for the draft
  books::Guess guess_;                  // the series offered by default
  std::string number_;                  // the number the voucher is saved as
  bool whole_suggested_ = false;        // the guide's whole voucher is shown to save as it is
  bool noting_ = false;                 // the event line is noted to be posted later, not posted now
  std::optional<std::size_t> from_waiting_; // the waiting event being posted, an index into its year's waiting
  ItemList waiting_page_;               // the page of the waiting events
  std::size_t chosen_waiting_ = 0;      // the waiting event shown, to post or delete
  std::set<books::VoucherKey> imported_; // the vouchers that came with an SIE import, when viewing vouchers
  std::string view_series_;             // the series whose vouchers are listed
  std::vector<std::size_t> listed_vouchers_; // the vouchers listed, newest first, indexes into the open year's vouchers
  ItemList voucher_page_;               // the page of the voucher list
  std::size_t viewed_ = 0;              // the voucher shown, an index into the open year's vouchers
  std::optional<books::VoucherKey> template_key_; // the template chosen before the voucher line, if any
  std::optional<books::VoucherKey> correcting_;   // the voucher being corrected (its rows reversed), if any
  std::optional<books::VoucherKey> corrected_;    // the voucher just corrected, for entering the right one
  Mode before_statement_ = Mode::Project;           // where a statement import started (project or year prompt)
  std::vector<books::StatementImport> statement_imports_; // the statements already imported
  std::vector<std::filesystem::path> statement_files_;    // the files listed in a folder
  ItemList statement_page_;                         // the page of the statement files
  std::filesystem::path statement_file_;            // the file being imported
  std::optional<statement::Document> statement_;    // its postings
  std::optional<books::AccountGuess> statement_guess_;
  std::optional<books::StatementPlan> statement_plan_; // the import waiting for y/n
  Mode before_entries_ = Mode::Project;             // where the list of statement entries was opened
  std::vector<books::OpenEntry> open_entries_;      // the statement entries not yet booked
  std::optional<std::size_t> unbooked_count_;       // how many there are, counted for the key list
  ItemList entry_page_;                             // the page of the statement entries
  std::size_t chosen_entry_ = 0;                    // the entry chosen, an index into open_entries_
  std::size_t entry_year_ = 0;                      // its fiscal year, an index into books_.years
  ItemList target_page_;                            // the page of the waiting events to add an entry to
  std::vector<books::Match> matches_;               // the possible matches among the open year's waiting events
  ItemList match_page_;                             // the page of the matches
  std::size_t chosen_match_ = 0;                    // the match chosen, an index into matches_
  bool merging_ = false;                            // the event line is the one event the chosen match makes

  bool is_prompt() const; // at cratchit>, workspace> or project>
  void show_keys(std::ostream& out);
  Operations<Op> operations() const; // of the current prompt, menu or view
  void show_operations(std::ostream& out) const; // the numbered operations
  bool shows_on_enter() const;
  void show_screen(std::ostream& out);
  void unknown_key(std::ostream& out) const;
  void on_prompt_key(charset::unicode::CodePoint key, std::ostream& out);
  void on_operation(Op op, std::ostream& out);
  void open_help(std::ostream& out);
  void open_log(std::ostream& out);
  void open_menu(std::ostream& out);
  void show_menu(std::ostream& out);
  void on_menu_key(charset::unicode::CodePoint key, std::ostream& out);
  void show_choices(std::ostream& out);
  void start_typing(std::ostream& out);
  void on_choose_key(charset::unicode::CodePoint key, std::ostream& out);
  void on_typing_key(charset::unicode::CodePoint key, std::ostream& out);
  void on_confirm_key(charset::unicode::CodePoint key, std::ostream& out);
  void create_and_choose(std::filesystem::path const& folder, std::ostream& out);
  void enter_workspace(std::filesystem::path const& folder, bool chosen, std::ostream& out);
  std::filesystem::path typed_folder(std::filesystem::path const& base) const;
  bool make_folder(std::filesystem::path const& folder, std::ostream& out);
  void open_last_project(std::ostream& out);
  void open_projects(std::ostream& out);
  void show_projects(std::ostream& out);
  void on_projects_key(charset::unicode::CodePoint key, std::ostream& out);
  void show_project_list(std::ostream& out);
  void on_project_list_key(charset::unicode::CodePoint key, std::ostream& out);
  void start_project_typing(Purpose purpose, std::ostream& out);
  void show_project_folders(std::ostream& out);
  void on_project_folder_key(charset::unicode::CodePoint key, std::ostream& out);
  void on_project_path(std::filesystem::path const& folder, std::ostream& out);
  void create_new_project(std::filesystem::path const& folder, std::ostream& out);
  void add_existing_project(std::filesystem::path const& folder, std::ostream& out);
  void enter_project(Project const& project, std::ostream& out);
  void start_import(std::ostream& out);
  void check_import(std::filesystem::path const& file, std::ostream& out);
  void on_confirm_import_key(charset::unicode::CodePoint key, std::ostream& out);
  void import(std::ostream& out);
  void open_years(std::ostream& out);
  void show_years(std::ostream& out);
  void on_years_key(charset::unicode::CodePoint key, std::ostream& out);
  void show_reports(std::ostream& out);
  void on_reports_key(charset::unicode::CodePoint key, std::ostream& out);
  void show_report(std::vector<std::string> lines, std::ostream& out);
  void open_vat_periods(std::ostream& out);
  void show_vat_periods(std::ostream& out) const;
  void on_vat_periods_key(charset::unicode::CodePoint key, std::ostream& out);
  std::string company() const;
  void open_year_menu(std::ostream& out);
  void show_year_menu(std::ostream& out);
  void on_year_menu_key(charset::unicode::CodePoint key, std::ostream& out);
  void enter_year(std::size_t index, std::ostream& out);
  void open_last_year(std::ostream& out);
  std::optional<std::size_t> read_open_year(std::ostream& out);
  void set_closed(bool closed, std::ostream& out);
  bool start_entry(bool noting, std::ostream& out, std::string const& line = "");
  books::Voucher const* voucher_of(books::VoucherKey const& key) const;
  books::VoucherKey key_of(books::Voucher const& voucher) const; // in the open year
  void open_vouchers(std::ostream& out);
  void show_voucher_series(std::ostream& out) const;
  void on_voucher_series_key(charset::unicode::CodePoint key, std::ostream& out);
  void list_vouchers(std::ostream& out);
  void show_vouchers(std::ostream& out) const;
  void on_voucher_list_key(charset::unicode::CodePoint key, std::ostream& out);
  void show_voucher(std::ostream& out) const;
  void on_voucher_view_key(charset::unicode::CodePoint key, std::ostream& out);
  void on_confirm_right_key(charset::unicode::CodePoint key, std::ostream& out);
  void start_statement(std::ostream& out);
  void on_statement_path(PathEditor::Result result, std::ostream& out);
  void show_statement_files(std::ostream& out) const;
  void on_statement_files_key(charset::unicode::CodePoint key, std::ostream& out);
  void check_statement(std::filesystem::path const& file, std::ostream& out);
  void start_statement_account(std::string const& account, std::ostream& out);
  void on_statement_account(PathEditor::Result result, std::ostream& out);
  void on_confirm_statement_key(charset::unicode::CodePoint key, std::ostream& out);
  void import_statement(std::ostream& out);
  void open_entries(std::ostream& out);
  bool read_entries(std::ostream& out); // reads the books and the open entries; false (error shown) on failure
  std::optional<std::size_t> count_unbooked() const; // the open entries at project> or the year; none on a read error
  void show_entries(std::ostream& out) const;
  void on_entry_list_key(charset::unicode::CodePoint key, std::ostream& out);
  void show_entry_choice(std::ostream& out) const;
  void on_entry_choice_key(charset::unicode::CodePoint key, std::ostream& out);
  void on_event_heading(PathEditor::Result result, std::ostream& out);
  void show_targets(std::ostream& out) const;
  void on_add_to_event_key(charset::unicode::CodePoint key, std::ostream& out);
  void back_to_entries(std::ostream& out);
  books::Posting posting_of(books::OpenEntry const& entry) const;
  void on_voucher_line(std::ostream& out);
  void show_readings(std::ostream& out);
  void on_choose_amount_key(charset::unicode::CodePoint key, std::ostream& out);
  void show_event(books::Event const& event, std::size_t year, std::ostream& out);
  void on_confirm_event_key(charset::unicode::CodePoint key, std::ostream& out);
  void offer_templates(std::ostream& out);
  void show_templates(std::ostream& out) const;
  void on_template_key(charset::unicode::CodePoint key, std::ostream& out);
  void use_template(books::Voucher const* like, std::ostream& out, bool reverse = false);
  void show_series(std::ostream& out);
  void on_series_key(charset::unicode::CodePoint key, std::ostream& out);
  void apply_guide(std::ostream& out);
  void show_row_help(std::ostream& out) const;
  void show_draft(std::ostream& out) const;
  void start_rows(std::ostream& out, std::string text = "");
  void on_row(PathEditor::Result result, std::ostream& out);
  void show_row(books::Posting const& row, std::ostream& out) const;
  void show_remaining(std::ostream& out) const;
  void show_accounts(std::string const& query, std::ostream& out) const;
  void on_hint_line(std::string_view query, std::ostream& out) const;
  void show_hints(std::vector<books::AccountSide> const& sides, std::ostream& out) const;
  void open_postings(std::ostream& out);
  void on_postings_key(charset::unicode::CodePoint key, std::ostream& out);
  void save_voucher(std::ostream& out);
  void note_event(std::ostream& out);
  void open_waiting(std::ostream& out);
  void show_waiting(std::ostream& out);
  void show_waiting_event(std::ostream& out) const;
  void on_waiting_event_key(charset::unicode::CodePoint key, std::ostream& out);
  void show_counted_rows(books::Postings const& postings, std::string_view indent, std::ostream& out) const;
  void show_event_postings(books::WaitingEvent const& waiting, std::ostream& out) const;
  void on_waiting_key(charset::unicode::CodePoint key, std::ostream& out);
  void post_waiting(std::size_t index, std::ostream& out);
  void on_confirm_delete_key(charset::unicode::CodePoint key, std::ostream& out);
  void open_matches(std::ostream& out);
  void back_to_matches(std::ostream& out);
  void show_matches(std::ostream& out) const;
  void on_matches_key(charset::unicode::CodePoint key, std::ostream& out);
  void show_match(std::ostream& out) const;
  void on_match_choice_key(charset::unicode::CodePoint key, std::ostream& out);
  void merge_events(std::ostream& out);
  void on_confirm_booked_key(charset::unicode::CodePoint key, std::ostream& out);
  bool write_books(std::ostream& out); // writes books_ and shows the files written; false (error shown) on failure
};

// Supplies the next key press as a Unicode code point, or nothing at end of input
using KeySource = std::function<std::optional<charset::unicode::CodePoint>()>;

// Prompt, read a key, handle it, until quit or end of input
void run_repl(Repl& repl, KeySource const& next_key, std::ostream& out);

} // namespace cratchit
