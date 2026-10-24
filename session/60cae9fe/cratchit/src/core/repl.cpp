#include "core/repl.hpp"

#include "core/books/report.hpp"
#include "core/charset/utf8.hpp"
#include "core/log.hpp"
#include "core/sie/file.hpp"

#include <algorithm>
#include <format>
#include <print>
#include <string>
#include <utility>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

constexpr charset::unicode::CodePoint ctrl_d = U'\x04';
// Esc does nothing: arrow keys also start with it, so the two can't be told apart
constexpr charset::unicode::CodePoint escape = U'\x1b';

// Voucher numbers in order: by value when both are digits, else as text
bool number_less(std::string const& a, std::string const& b) {
  auto const digits = [](std::string const& text) {
    return !text.empty() && std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; });
  };
  if (digits(a) && digits(b) && a.size() != b.size()) {
    return a.size() < b.size();
  }
  return a < b;
}

bool is_whitespace(charset::unicode::CodePoint key) {
  return key == U'\n' || key == U'\r' || key == U' ' || key == U'\t';
}

// With more projects than keys 1-9, keys 1-8 show a range, 0 scrolls up and 9 down, as in help
constexpr std::size_t max_keys = 9;
constexpr std::size_t range_size = 8;

std::string trimmed(std::string const& text) {
  auto const first = text.find_first_not_of(' ');
  if (first == std::string::npos) {
    return {};
  }
  return text.substr(first, text.find_last_not_of(' ') - first + 1);
}

// How a key press is shown after the prompt, as echo is off in raw mode.
// UTF-8, as that is the only terminal output encoding cratchit writes so far.
std::string echo_of(charset::unicode::CodePoint key) {
  if (key == ctrl_d) {
    return "^D";
  }
  if (is_whitespace(key)) {
    return "";
  }
  std::string result;
  for (auto const byte : charset::utf8::FromUnicodeBuffer{}.push(key)) {
    result += static_cast<char>(byte);
  }
  return result;
}

// How a file written for the books is reported
std::string_view change_word(books::Written const& written) {
  return written.removed ? "Removed" : written.created ? "Initiated" : "Updated";
}

} // namespace

Repl::Repl(Section const& manual, std::size_t page_lines, WorkspacePlaces places, fs::path log_file)
    : places_(std::move(places)), log_file_(std::move(log_file)), help_(manual, page_lines), log_view_(page_lines),
      pager_(page_lines) {}

void Repl::open_workspace(std::ostream& out) {
  auto const entry = workspace_entry(places_.runtime_file);
  if (!entry) {
    std::println(out, "Error: {}", entry.error());
    mode_ = Mode::Main;
    return;
  }
  std::error_code ec;
  if (!*entry) {
    std::println(out, "No workspace is set in {}.", places_.runtime_file.string());
  } else if (!fs::is_directory(**entry, ec)) {
    std::println(out, "Workspace folder {} does not exist.", (*entry)->string());
  } else {
    enter_workspace(**entry, false, out);
    return;
  }
  before_choose_ = Mode::Main;
  mode_ = Mode::Choose;
  show_choices(out);
}

Step Repl::on_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == ctrl_d) {
    return Step::Quit;
  }
  switch (mode_) {
  case Mode::Help:
    if (help_.on_key(key, out) == HelpBrowser::Result::Leave) {
      mode_ = before_help_;
      if (mode_ == Mode::Menu) {
        show_menu(out);
      } else if (mode_ == Mode::Choose) {
        show_choices(out);
      } else if (mode_ == Mode::Projects) {
        show_projects(out);
      } else if (mode_ == Mode::ProjectFolder) {
        show_project_folders(out);
      } else if (mode_ == Mode::Years) {
        show_years(out);
      } else if (mode_ == Mode::Reports) {
        show_reports(out);
      } else if (mode_ == Mode::ChooseAmount) {
        show_readings(out);
      } else if (mode_ == Mode::YearMenu) {
        show_year_menu(out);
      } else if (mode_ == Mode::Waiting) {
        show_waiting(out);
      } else if (mode_ == Mode::Template) {
        show_templates(out);
      } else if (mode_ == Mode::VoucherList) {
        show_vouchers(out);
      } else if (mode_ == Mode::VoucherView) {
        show_voucher(out);
      }
    }
    return Step::Continue;
  case Mode::Log:
    if (log_view_.on_key(key, out) == LogView::Result::Leave) {
      mode_ = before_log_;
    }
    return Step::Continue;
  case Mode::Typing:
    on_typing_key(key, out);
    return Step::Continue;
  case Mode::ConfirmCreate:
    on_confirm_key(key, out);
    return Step::Continue;
  case Mode::ConfirmImport:
    on_confirm_import_key(key, out);
    return Step::Continue;
  case Mode::Paging:
    if (pager_.on_key(key, out) == Pager::Result::Leave) {
      mode_ = Mode::Reports;
      show_reports(out);
    }
    return Step::Continue;
  case Mode::ConfirmEvent:
    on_confirm_event_key(key, out);
    return Step::Continue;
  case Mode::Series:
    // h is series H here, so help is not offered
    if (key == U'q') {
      return Step::Quit;
    }
    on_series_key(key, out);
    return Step::Continue;
  case Mode::ConfirmSave:
    on_confirm_save_key(key, out);
    return Step::Continue;
  case Mode::ConfirmDelete:
    on_confirm_delete_key(key, out);
    return Step::Continue;
  case Mode::VoucherSeries:
    // h is series H here, so help is not offered
    if (key == U'q') {
      return Step::Quit;
    }
    on_voucher_series_key(key, out);
    return Step::Continue;
  case Mode::ConfirmRight:
    on_confirm_right_key(key, out);
    return Step::Continue;
  default:
    break;
  }
  if (key == U'q') {
    return Step::Quit;
  }
  if (key == U'h') {
    open_help(out);
    return Step::Continue;
  }
  if (is_prompt() && (key == U'\n' || key == U'\r')) {
    show_keys(out);
    return Step::Continue;
  }
  switch (mode_) {
  case Mode::Main:
    if (key == U'w') {
      open_menu(out);
    } else if (key == U'l') {
      open_log(out);
    } else {
      std::println(out, "Unknown key.");
      show_keys(out);
    }
    break;
  case Mode::Menu:
    on_menu_key(key, out);
    break;
  case Mode::Choose:
    on_choose_key(key, out);
    break;
  case Mode::Workspace:
    if (key == U'-') {
      mode_ = Mode::Main;
    } else if (key == U'p') {
      open_projects(out);
    } else if (key == U'l') {
      open_log(out);
    } else {
      std::println(out, "Unknown key.");
      show_keys(out);
    }
    break;
  case Mode::Projects:
    on_projects_key(key, out);
    break;
  case Mode::ProjectFolder:
    on_project_folder_key(key, out);
    break;
  case Mode::Project:
    if (key == U'-') {
      mode_ = Mode::Workspace;
    } else if (key == U'l') {
      open_log(out);
    } else if (key == U'y') {
      open_year_menu(out);
    } else if (key == U'i') {
      start_import(out);
    } else if (key == U'r') {
      before_reports_ = Mode::Years;
      open_years(out);
    } else {
      std::println(out, "Unknown key.");
      show_keys(out);
    }
    break;
  case Mode::Years:
    on_years_key(key, out);
    break;
  case Mode::Reports:
    on_reports_key(key, out);
    break;
  case Mode::ChooseAmount:
    on_choose_amount_key(key, out);
    break;
  case Mode::YearMenu:
    on_year_menu_key(key, out);
    break;
  case Mode::Year:
    on_year_key(key, out);
    break;
  case Mode::Waiting:
    on_waiting_key(key, out);
    break;
  case Mode::Template:
    on_template_key(key, out);
    break;
  case Mode::VoucherList:
    on_voucher_list_key(key, out);
    break;
  case Mode::VoucherView:
    on_voucher_view_key(key, out);
    break;
  default:
    break;
  }
  return Step::Continue;
}

bool Repl::ignores(charset::unicode::CodePoint key) const {
  // The path editor uses every key, and drops arrow keys itself
  if (mode_ == Mode::Typing) {
    return false;
  }
  if (key == escape) {
    return true;
  }
  // Enter lists the keys at a prompt, and takes the first template or the default series
  if ((is_prompt() || mode_ == Mode::Template || mode_ == Mode::Series) && (key == U'\n' || key == U'\r')) {
    return false;
  }
  // Space and Enter show the next page of help or the log
  if ((mode_ == Mode::Help && help_.is_paging()) || mode_ == Mode::Log || mode_ == Mode::Paging) {
    return key == U'\t';
  }
  return is_whitespace(key);
}

bool Repl::is_prompt() const {
  return mode_ == Mode::Main || mode_ == Mode::Workspace || mode_ == Mode::Project || mode_ == Mode::Year;
}

void Repl::show_keys(std::ostream& out) const {
  std::println(out, "Keys at {}:", prompt().substr(0, prompt().size() - 1));
  if (mode_ == Mode::Main) {
    std::println(out, "  w  Workspace menu");
  } else if (mode_ == Mode::Workspace) {
    std::println(out, "  p  Projects");
  } else if (mode_ == Mode::Project) {
    std::println(out, "  y  Fiscal years");
    std::println(out, "  i  Import an SIE file");
    std::println(out, "  r  Reports");
  } else {
    auto const* year = books_.year_starting(open_year_);
    std::println(out, "  e  Enter a voucher");
    std::println(out, "  n  Note an event to post later");
    std::println(out, "  w  Waiting events ({})", year ? year->waiting.size() : 0);
    std::println(out, "  v  View vouchers (correct one, or use it as a template)");
    std::println(out, "  r  Reports of this year");
    if (year && year->closed) {
      std::println(out, "  o  Open the year for new vouchers");
    } else {
      std::println(out, "  c  Close the year for new vouchers");
    }
  }
  std::println(out, "  l  Show the log");
  if (mode_ == Mode::Workspace) {
    std::println(out, "  -  Back to cratchit>");
  } else if (mode_ == Mode::Project) {
    std::println(out, "  -  Back to workspace>");
  } else if (mode_ == Mode::Year) {
    std::println(out, "  -  Back to project>");
  }
  std::println(out, "  h  Help (the manual)");
  std::println(out, "  q  Quit");
}

bool Repl::is_typing() const {
  return mode_ == Mode::Typing;
}

std::string Repl::prompt() const {
  switch (mode_) {
  case Mode::Help:
    return help_.prompt();
  case Mode::Log:
    return log_view_.prompt();
  case Mode::Menu:
  case Mode::Choose:
    return "workspace?> ";
  case Mode::Typing:
    return editor_.prompt();
  case Mode::ConfirmCreate:
    return "Create it? (y/n) ";
  case Mode::Workspace:
    return "workspace> ";
  case Mode::Projects:
  case Mode::ProjectFolder:
    return "project?> ";
  case Mode::Project:
    return "project> ";
  case Mode::ConfirmImport:
    return "Import it? (y/n) ";
  case Mode::Years:
  case Mode::Reports:
    return "report?> ";
  case Mode::Paging:
    return pager_.prompt();
  case Mode::YearMenu:
    return "year?> ";
  case Mode::Year: {
    // The fiscal year, e.g. 2026/27, or 2025 for a calendar year
    auto const* year = books_.year_starting(open_year_);
    if (!year) {
      return "year> ";
    }
    auto const start = static_cast<int>(year->start.year());
    auto const end = static_cast<int>(year->end.year());
    return start == end ? std::format("{}> ", start) : std::format("{}/{:02}> ", start, end % 100);
  }
  case Mode::ChooseAmount:
    return "amount?> ";
  case Mode::ConfirmEvent:
    return "OK? (y/n) ";
  case Mode::Template:
    return "template?> ";
  case Mode::Series:
    return "series> ";
  case Mode::ConfirmSave:
    return std::format("Save as {} {}? (y/n) ", draft_->series, number_);
  case Mode::Waiting:
    return deleting_ ? "delete?> " : "waiting?> ";
  case Mode::ConfirmDelete:
    return "Delete it? (y/n) ";
  case Mode::VoucherSeries:
    return "series?> ";
  case Mode::VoucherList:
    return "voucher?> ";
  case Mode::VoucherView: {
    auto const* year = books_.year_starting(open_year_);
    return year && viewed_ < year->vouchers.size()
               ? std::format("{} {}> ", year->vouchers[viewed_].series, year->vouchers[viewed_].number)
               : "voucher> ";
  }
  case Mode::ConfirmRight:
    return "Enter the right voucher now? (y/n) ";
  default:
    return "cratchit> ";
  }
}

void Repl::open_help(std::ostream& out) {
  before_help_ = mode_;
  mode_ = Mode::Help;
  help_.open(out);
}

void Repl::open_log(std::ostream& out) {
  if (log_view_.open(log_file_, out) == LogView::Result::Stay) {
    before_log_ = mode_;
    mode_ = Mode::Log;
  }
}

void Repl::open_menu(std::ostream& out) {
  auto const entry = workspace_entry(places_.runtime_file);
  if (!entry) {
    std::println(out, "Error: {}", entry.error());
    return;
  }
  std::error_code ec;
  current_.reset();
  if (*entry && fs::is_directory(**entry, ec)) {
    current_ = **entry;
  }
  mode_ = Mode::Menu;
  show_menu(out);
}

void Repl::show_menu(std::ostream& out) {
  std::println(out, "Workspace:");
  if (current_) {
    std::println(out, "  w  Open {}", current_->string());
  }
  std::println(out, "  b  Browse for a new workspace");
  std::println(out, "");
  std::println(out, "Press {}, - to go back, h for help, q to quit.", current_ ? "w or b" : "b");
}

void Repl::on_menu_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'w' && current_) {
    enter_workspace(*current_, false, out);
  } else if (key == U'b') {
    before_choose_ = Mode::Menu;
    mode_ = Mode::Choose;
    show_choices(out);
  } else if (key == U'-') {
    mode_ = Mode::Main;
  } else {
    std::println(out, "Press {}, - to go back, h for help, q to quit.", current_ ? "w or b" : "b");
  }
}

void Repl::show_choices(std::ostream& out) {
  std::println(out, "Choose a workspace folder:");
  std::println(out, "  1  {}", places_.start_folder.string());
  if (places_.default_folder) {
    std::println(out, "  2  {}", places_.default_folder->string());
  }
  std::println(out, "  3  Type a folder path");
  std::println(out, "");
  std::println(out, "Press a number to choose, - to go back, h for help, q to quit.");
}

void Repl::start_typing(std::ostream& out) {
  mode_ = Mode::Typing;
  std::println(out, "Type the folder path. Tab completes a folder name, Backspace erases, Enter accepts.");
  std::println(out, "Enter on an empty path goes back.");
  out << editor_.prompt();
}

void Repl::on_choose_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'1') {
    enter_workspace(places_.start_folder, true, out);
  } else if (key == U'2' && places_.default_folder) {
    create_and_choose(*places_.default_folder, out);
  } else if (key == U'3') {
    editor_.start(places_.start_folder.root_path().string());
    purpose_ = Purpose::Workspace;
    start_typing(out);
  } else if (key == U'-') {
    mode_ = before_choose_;
    if (mode_ == Mode::Menu) {
      show_menu(out);
    }
  } else {
    std::println(out, "Press a number to choose, - to go back, h for help, q to quit.");
  }
}

void Repl::on_typing_key(charset::unicode::CodePoint key, std::ostream& out) {
  auto const result = editor_.on_key(key, out);
  if (result == PathEditor::Result::Typing) {
    return;
  }
  if (purpose_ == Purpose::Voucher) {
    if (result == PathEditor::Result::Cancelled) {
      mode_ = Mode::Year;
      return;
    }
    on_voucher_line(out);
    return;
  }
  if (purpose_ == Purpose::Row) {
    on_row(result, out);
    return;
  }
  if (purpose_ == Purpose::SieFile) {
    if (result == PathEditor::Result::Cancelled) {
      mode_ = Mode::Project;
      return;
    }
    auto const file = typed_folder(places_.start_folder);
    std::error_code ec;
    if (!fs::is_regular_file(file, ec)) {
      std::println(out, "No file {}.", file.string());
      out << editor_.prompt();
      return;
    }
    check_import(file, out);
    return;
  }
  if (purpose_ == Purpose::Account) {
    mode_ = Mode::Reports;
    auto const account = trimmed(editor_.text());
    if (result == PathEditor::Result::Cancelled || account.empty()) {
      show_reports(out);
      return;
    }
    auto lines = books::account_ledger(books_, year_, company(), account);
    if (!lines) {
      std::println(out, "No account {} in the books.", account);
      show_reports(out);
      return;
    }
    show_report(std::move(*lines), out);
    return;
  }
  if (purpose_ == Purpose::ProjectName) {
    new_name_ = trimmed(editor_.text());
    if (result == PathEditor::Result::Cancelled || new_name_.empty()) {
      mode_ = Mode::Projects;
      show_projects(out);
    } else {
      mode_ = Mode::ProjectFolder;
      show_project_folders(out);
    }
    return;
  }
  if (result == PathEditor::Result::Cancelled) {
    if (purpose_ == Purpose::Workspace) {
      mode_ = Mode::Choose;
      show_choices(out);
    } else if (purpose_ == Purpose::ProjectFolder) {
      mode_ = Mode::ProjectFolder;
      show_project_folders(out);
    } else {
      mode_ = Mode::Projects;
      show_projects(out);
    }
    return;
  }
  if (purpose_ != Purpose::Workspace) {
    on_project_path(typed_folder(workspace_->folder), out);
    return;
  }
  auto const folder = typed_folder(places_.start_folder);
  std::error_code ec;
  if (fs::is_directory(folder, ec)) {
    enter_workspace(folder, true, out);
  } else if (fs::exists(folder, ec)) {
    std::println(out, "{} is not a folder.", folder.string());
    out << editor_.prompt();
  } else {
    std::println(out, "Folder {} does not exist.", folder.string());
    to_create_ = folder;
    mode_ = Mode::ConfirmCreate;
  }
}

void Repl::on_confirm_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'y' && purpose_ == Purpose::ProjectFolder) {
    if (make_folder(to_create_, out)) {
      create_new_project(to_create_, out);
    } else {
      mode_ = Mode::Typing;
      out << editor_.prompt();
    }
  } else if (key == U'y') {
    create_and_choose(to_create_, out);
  } else if (key == U'n' || key == U'-') {
    mode_ = Mode::Typing;
    out << editor_.prompt();
  } else {
    std::println(out, "Press y to create the folder, n to change the path.");
  }
}

void Repl::create_and_choose(fs::path const& folder, std::ostream& out) {
  if (!make_folder(folder, out)) {
    mode_ = Mode::Choose;
    show_choices(out);
    return;
  }
  enter_workspace(folder, true, out);
}

// The folder typed in the editor; a relative path is taken from base
fs::path Repl::typed_folder(fs::path const& base) const {
  auto folder = fs::path(editor_.text());
  if (folder.is_relative()) {
    folder = base / folder;
  }
  folder = folder.lexically_normal();
  if (!folder.has_filename() && folder.has_relative_path()) {
    folder = folder.parent_path(); // drop a final separator
  }
  return folder;
}

// Creates folder when missing, reporting it; false (with the error shown) if it cannot be created
bool Repl::make_folder(fs::path const& folder, std::ostream& out) {
  std::error_code ec;
  if (fs::is_directory(folder, ec)) {
    return true;
  }
  if (!fs::create_directories(folder, ec)) {
    std::println(out, "Error: Cannot create {}: {}", folder.string(), ec.message());
    return false;
  }
  std::println(out, "Created folder {}", folder.string());
  return true;
}

void Repl::enter_workspace(fs::path const& folder, bool chosen, std::ostream& out) {
  auto const workspace = cratchit::open_workspace(folder, places_.today, chosen);
  if (!workspace) {
    std::println(out, "Error: {}", workspace.error());
    mode_ = Mode::Main;
    return;
  }
  for (auto const& entry : workspace->initiated) {
    std::println(out, "Initiated {} in {}", entry.file.generic_string(), entry.folder.string());
  }
  // Choosing the saved workspace again leaves runtime.txt as it is
  auto const previous = workspace_entry(places_.runtime_file);
  auto const unchanged = previous && *previous && **previous == folder;
  if (chosen && !unchanged) {
    if (auto const saved = save_workspace_entry(places_.runtime_file, folder); !saved) {
      std::println(out, "Error: {}", saved.error());
      mode_ = Mode::Main;
      return;
    }
    auto const runtime_root = places_.runtime_file.parent_path().parent_path();
    std::println(out, "Updated .cratchit/runtime.txt in {}", runtime_root.string());
    if (previous && *previous) {
      LOG_BUSINESS("Switched workspace from {} to {}, saved in {}", (*previous)->string(), folder.string(),
                   places_.runtime_file.string());
    } else {
      LOG_BUSINESS("Chose workspace {}, saved in {}", folder.string(), places_.runtime_file.string());
    }
  }
  for (auto const& entry : workspace->initiated) {
    LOG_BUSINESS("Initiated {} in {}", entry.file.generic_string(), entry.folder.string());
  }
  std::println(out, "Opened workspace {}", folder.string());
  LOG_BUSINESS("Opened workspace {}", folder.string());
  workspace_ = *workspace;
  mode_ = Mode::Workspace;
  open_last_project(out);
}

// Opens the project named by `project = <uuid>` in workspace.txt, if any
void Repl::open_last_project(std::ostream& out) {
  auto const last = last_project(workspace_->workspace_file);
  if (!last) {
    std::println(out, "Error: {}", last.error());
    return;
  }
  if (!*last) {
    return;
  }
  auto const listed = listed_projects(workspace_->workspace_file);
  if (!listed) {
    std::println(out, "Error: {}", listed.error());
    return;
  }
  auto const it = std::ranges::find(*listed, **last, &ListedProject::uuid);
  if (it == listed->end()) {
    std::println(out, "Error: The last project {} is not listed in {}", **last, workspace_->workspace_file.string());
    return;
  }
  auto const project = open_listed_project(*it);
  if (!project) {
    std::println(out, "Error: {}", project.error());
    return;
  }
  enter_project(*project, out);
}

void Repl::open_projects(std::ostream& out) {
  auto listed = listed_projects(workspace_->workspace_file);
  if (!listed) {
    std::println(out, "Error: {}", listed.error());
    return;
  }
  listed_ = std::move(*listed);
  first_listed_ = 0;
  mode_ = Mode::Projects;
  show_projects(out);
}

void Repl::show_projects(std::ostream& out) {
  std::println(out, "Projects in {}:", workspace_->folder.string());
  auto const n = listed_.size();
  auto const first = n <= max_keys ? 0 : first_listed_;
  auto const last = n <= max_keys ? n : std::min(first + range_size, n);
  if (first > 0) {
    std::println(out, "  0  ...up");
  }
  for (auto i = first; i < last; ++i) {
    auto const& listed = listed_[i];
    std::string name = "(error)";
    std::error_code ec;
    if (auto const project = open_listed_project(listed)) {
      name = project->name;
    } else if (!fs::is_directory(listed.folder, ec)) {
      name = "(not found)";
    }
    std::println(out, "  {}  {}  {}", i - first + 1, name, listed.folder.string());
  }
  if (last < n) {
    std::println(out, "  9  ...down");
  }
  std::println(out, "  n  New project");
  std::println(out, "  a  Add an existing project folder");
  std::println(out, "");
  projects_hint(out);
}

void Repl::projects_hint(std::ostream& out) {
  std::println(out, "Press {}n or a, - to go back, h for help, q to quit.", listed_.empty() ? "" : "a number, ");
}

void Repl::on_projects_key(charset::unicode::CodePoint key, std::ostream& out) {
  auto const n = listed_.size();
  if (key == U'n') {
    start_project_typing(Purpose::ProjectName, out);
  } else if (key == U'a') {
    start_project_typing(Purpose::ExistingProject, out);
  } else if (key == U'-') {
    mode_ = Mode::Workspace;
  } else if (n > max_keys && key == U'0' && first_listed_ > 0) {
    first_listed_ -= range_size;
    show_projects(out);
  } else if (n > max_keys && key == U'9' && first_listed_ + range_size < n) {
    first_listed_ += range_size;
    show_projects(out);
  } else if (key >= U'1' && key <= (n > max_keys ? U'8' : U'9')) {
    auto const index = (n > max_keys ? first_listed_ : 0) + static_cast<std::size_t>(key - U'1');
    if (index >= n) {
      projects_hint(out);
      return;
    }
    auto const project = open_listed_project(listed_[index]);
    if (!project) {
      std::println(out, "Error: {}", project.error());
      return;
    }
    enter_project(*project, out);
  } else {
    projects_hint(out);
  }
}

void Repl::start_project_typing(Purpose purpose, std::ostream& out) {
  purpose_ = purpose;
  if (purpose == Purpose::ProjectName) {
    editor_.start("", PathEditor::Kind::Name);
    mode_ = Mode::Typing;
    std::println(out, "Type the official name of the person, company or organisation. Backspace erases, Enter accepts.");
    std::println(out, "Enter on an empty name goes back.");
    out << editor_.prompt();
    return;
  }
  if (purpose == Purpose::ExistingProject) {
    std::println(out, "The folder must hold .cratchit/project.txt.");
  }
  editor_.start((workspace_->folder / "").string());
  start_typing(out);
}

void Repl::show_project_folders(std::ostream& out) {
  std::println(out, "Choose a folder for {}:", new_name_);
  if (auto const folder = default_project_folder(workspace_->folder, new_name_)) {
    std::println(out, "  1  {}", folder->string());
  }
  std::println(out, "  2  Type a folder path");
  std::println(out, "");
  std::println(out, "Press a number to choose, - to go back, h for help, q to quit.");
}

void Repl::on_project_folder_key(charset::unicode::CodePoint key, std::ostream& out) {
  auto const folder = default_project_folder(workspace_->folder, new_name_);
  if (key == U'1' && folder) {
    if (make_folder(*folder, out)) {
      create_new_project(*folder, out);
    }
  } else if (key == U'2') {
    start_project_typing(Purpose::ProjectFolder, out);
  } else if (key == U'-') {
    mode_ = Mode::Projects;
    show_projects(out);
  } else {
    std::println(out, "Press a number to choose, - to go back, h for help, q to quit.");
  }
}

// A folder path accepted in the editor, for a new or an existing project
void Repl::on_project_path(fs::path const& folder, std::ostream& out) {
  std::error_code ec;
  if (fs::is_directory(folder, ec)) {
    if (purpose_ == Purpose::ProjectFolder) {
      create_new_project(folder, out);
    } else {
      add_existing_project(folder, out);
    }
  } else if (fs::exists(folder, ec)) {
    std::println(out, "{} is not a folder.", folder.string());
    out << editor_.prompt();
  } else if (purpose_ == Purpose::ProjectFolder) {
    std::println(out, "Folder {} does not exist.", folder.string());
    to_create_ = folder;
    mode_ = Mode::ConfirmCreate;
  } else {
    std::println(out, "Folder {} does not exist.", folder.string());
    out << editor_.prompt();
  }
}

void Repl::create_new_project(fs::path const& folder, std::ostream& out) {
  auto const project = create_project(folder, new_name_, new_uuid(), places_.today);
  if (!project) {
    std::println(out, "Error: {}", project.error());
    mode_ = Mode::ProjectFolder;
    show_project_folders(out);
    return;
  }
  for (auto const& entry : project->initiated) {
    std::println(out, "Initiated {} in {}", entry.file.generic_string(), entry.folder.string());
    LOG_BUSINESS("Initiated {} in {}", entry.file.generic_string(), entry.folder.string());
  }
  LOG_BUSINESS("Created project {} ({}) in {}", project->name, project->uuid, folder.string());
  enter_project(*project, out);
}

void Repl::add_existing_project(fs::path const& folder, std::ostream& out) {
  auto const project = read_project(folder);
  if (!project) {
    std::println(out, "Error: {}", project.error());
    out << editor_.prompt();
    return;
  }
  auto const listed = listed_projects(workspace_->workspace_file);
  if (!listed) {
    std::println(out, "Error: {}", listed.error());
    out << editor_.prompt();
    return;
  }
  auto const it = std::ranges::find(*listed, project->uuid, &ListedProject::uuid);
  std::error_code ec;
  if (it != listed->end() && !fs::equivalent(it->folder, folder, ec)) {
    // The same project in two folders, e.g. a copied project folder
    std::println(out, "Error: Project {} ({}) is already listed with folder {}", project->name, project->uuid,
                 it->folder.string());
    LOG_DESIGN_INSUFFICIENCY("Project {} ({}) in {} is already listed with folder {}", project->name, project->uuid,
                             folder.string(), it->folder.string());
    out << editor_.prompt();
    return;
  }
  if (it == listed->end()) {
    LOG_BUSINESS("Added project {} ({}) in {}", project->name, project->uuid, folder.string());
  }
  enter_project(*project, out);
}

// Lists the project in workspace.txt if needed, saves it as the last opened, and opens it
void Repl::enter_project(Project const& project, std::ostream& out) {
  auto const changed = remember_project(workspace_->workspace_file, {project.uuid, project.folder});
  if (!changed) {
    std::println(out, "Error: {}", changed.error());
    mode_ = Mode::Workspace;
    return;
  }
  if (*changed) {
    std::println(out, "Updated .cratchit/workspace.txt in {}", workspace_->folder.string());
  }
  std::println(out, "Opened project {} in {}", project.name, project.folder.string());
  LOG_BUSINESS("Opened project {} ({}) in {}", project.name, project.uuid, project.folder.string());
  project_ = project;
  mode_ = Mode::Project;
  open_last_year(out);
}

std::string Repl::company() const {
  return project_->orgnr.empty() ? project_->name : std::format("{}, {}", project_->name, project_->orgnr);
}

void Repl::start_import(std::ostream& out) {
  purpose_ = Purpose::SieFile;
  editor_.start((places_.start_folder / "").string(), PathEditor::Kind::File);
  mode_ = Mode::Typing;
  std::println(out, "Type the path of the SIE file to import. Tab completes a folder or file name, Backspace erases,");
  std::println(out, "Enter accepts. Enter on an empty path goes back.");
  out << editor_.prompt();
}

// Reads, checks and plans the import of an SIE file, shows what it would do, and asks
void Repl::check_import(fs::path const& file, std::ostream& out) {
  mode_ = Mode::Project;
  auto const lexed = sie::lex_file(file);
  if (!lexed) {
    std::println(out, "Error: {}", lexed.error());
    return;
  }
  auto const parsed = sie::parse_document(lexed->lines);
  auto const books = books::read_books(project_->folder);
  if (!books) {
    std::println(out, "Error: {}", books.error());
    return;
  }
  auto plan = books::plan_import(*books, {project_->name, project_->orgnr}, parsed.document);
  plan.errors.insert(plan.errors.begin(), parsed.errors.begin(), parsed.errors.end());
  if (!lexed->alert.empty()) {
    plan.notes.insert(plan.notes.begin(), lexed->alert);
  }
  auto const& document = parsed.document;
  std::println(out, "SIE file {}", file.string());
  std::println(out, "  Read as      {}", charset_name(lexed->charset));
  std::println(out, "  Program      {} {}{}", document.program, document.program_version,
               document.generated ? ", generated " + format_iso_date(*document.generated) : std::string{});
  std::println(out, "  Company      {}, {}", document.company_name, document.orgnr);
  if (document.year(0)) {
    std::println(out, "  Fiscal year  {} - {}", format_iso_date(plan.year_start), format_iso_date(plan.year_end));
  }
  std::println(out, "  Accounts     {}: {} new, {} changed", document.accounts.size(), plan.accounts_added,
               plan.accounts_changed);
  std::println(out, "  Vouchers     {}: {} new, {} already in the books", document.vouchers.size(), plan.vouchers_added,
               plan.vouchers_known);
  if (!plan.notes.empty()) {
    std::println(out, "Notes:");
    for (auto const& note : plan.notes) {
      std::println(out, "  - {}", note);
    }
  }
  LOG_BUSINESS("Checked SIE file {} for project {} ({}): {} errors, {} notes", file.string(), project_->name,
               project_->uuid, plan.errors.size(), plan.notes.size());
  if (!plan.errors.empty()) {
    constexpr std::size_t max_errors = 20;
    std::println(out, "Errors:");
    for (std::size_t i = 0; i < std::min(plan.errors.size(), max_errors); ++i) {
      std::println(out, "  - {}", plan.errors[i]);
    }
    if (plan.errors.size() > max_errors) {
      std::println(out, "  - and {} more", plan.errors.size() - max_errors);
    }
    std::println(out, "Nothing is imported.");
    return;
  }
  if (plan.vouchers_added == 0 && plan.accounts_added == 0 && plan.accounts_changed == 0 && !plan.identity_changed &&
      books->year_starting(plan.year_start)) {
    std::println(out, "Everything in the file is already in the books. Nothing to import.");
    return;
  }
  import_source_ = {file, std::string(charset_name(lexed->charset)),
                    std::format("{} {}", document.program, document.program_version), document.generated,
                    places_.today};
  plan_ = std::move(plan);
  mode_ = Mode::ConfirmImport;
}

void Repl::on_confirm_import_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'y') {
    import(out);
  } else if (key == U'n' || key == U'-') {
    std::println(out, "Nothing is imported.");
    plan_.reset();
    mode_ = Mode::Project;
  } else {
    std::println(out, "Press y to import the file, n not to.");
  }
}

void Repl::import(std::ostream& out) {
  mode_ = Mode::Project;
  auto const plan = std::move(*plan_);
  plan_.reset();
  auto const saved = books::save_import(project_->folder, plan, import_source_);
  if (!saved) {
    std::println(out, "Error: {}", saved.error());
    return;
  }
  for (auto const& written : saved->written) {
    std::println(out, "{} {} in {}", change_word(written), written.file.generic_string(), project_->folder.string());
    if (written.created) {
      LOG_BUSINESS("Initiated {} in {}", written.file.generic_string(), project_->folder.string());
    }
  }
  if (plan.identity_changed) {
    auto const changed = save_project_identity(project_->folder, plan.identity.name, plan.identity.orgnr);
    if (!changed) {
      std::println(out, "Error: {}", changed.error());
    } else if (*changed) {
      std::println(out, "Updated .cratchit/project.txt in {}", project_->folder.string());
      LOG_BUSINESS("Project {} ({}) is now {}, organisation number {}", project_->name, project_->uuid,
                   plan.identity.name, plan.identity.orgnr);
      project_->name = plan.identity.name;
      project_->orgnr = plan.identity.orgnr;
    }
  }
  std::println(out, "Stashed the file as {} in {}", saved->stashed.generic_string(), project_->folder.string());
  std::println(out, "Imported {} voucher{} into fiscal year {} - {} ({} already in the books).", plan.vouchers_added,
               plan.vouchers_added == 1 ? "" : "s", format_iso_date(plan.year_start), format_iso_date(plan.year_end),
               plan.vouchers_known);
  LOG_BUSINESS("Imported SIE file {} into project {} ({}): fiscal year {} - {}, {} vouchers added, {} already in the "
               "books, stashed as {}",
               import_source_.file.string(), project_->name, project_->uuid, format_iso_date(plan.year_start),
               format_iso_date(plan.year_end), plan.vouchers_added, plan.vouchers_known,
               saved->stashed.generic_string());
  for (auto const& note : plan.notes) {
    LOG_BUSINESS("Import note: {}", note);
  }
}

void Repl::open_years(std::ostream& out) {
  auto books = books::read_books(project_->folder);
  if (!books) {
    std::println(out, "Error: {}", books.error());
    return;
  }
  if (books->years.empty()) {
    std::println(out, "No books yet. Press i to import an SIE file.");
    return;
  }
  books_ = std::move(*books);
  first_year_ = 0;
  mode_ = Mode::Years;
  show_years(out);
}

// Newest year first
void Repl::show_years(std::ostream& out) {
  std::println(out, "Fiscal years of {}:", company());
  auto const n = books_.years.size();
  auto const first = n <= max_keys ? 0 : first_year_;
  auto const last = n <= max_keys ? n : std::min(first + range_size, n);
  if (first > 0) {
    std::println(out, "  0  ...up");
  }
  for (auto i = first; i < last; ++i) {
    auto const& year = books_.years[n - 1 - i];
    std::println(out, "  {}  {} - {}", i - first + 1, format_iso_date(year.start), format_iso_date(year.end));
  }
  if (last < n) {
    std::println(out, "  9  ...down");
  }
  std::println(out, "");
  std::println(out, "Press a number to choose, - to go back, h for help, q to quit.");
}

void Repl::on_years_key(charset::unicode::CodePoint key, std::ostream& out) {
  auto const n = books_.years.size();
  if (key == U'-') {
    mode_ = Mode::Project;
  } else if (n > max_keys && key == U'0' && first_year_ > 0) {
    first_year_ -= range_size;
    show_years(out);
  } else if (n > max_keys && key == U'9' && first_year_ + range_size < n) {
    first_year_ += range_size;
    show_years(out);
  } else if (key >= U'1' && key <= (n > max_keys ? U'8' : U'9') &&
             (n > max_keys ? first_year_ : 0) + static_cast<std::size_t>(key - U'1') < n) {
    year_ = n - 1 - ((n > max_keys ? first_year_ : 0) + static_cast<std::size_t>(key - U'1'));
    mode_ = Mode::Reports;
    show_reports(out);
  } else {
    std::println(out, "Press a number to choose, - to go back, h for help, q to quit.");
  }
}

void Repl::show_reports(std::ostream& out) {
  auto const& year = books_.years[year_];
  std::println(out, "Reports for {} - {}:", format_iso_date(year.start), format_iso_date(year.end));
  std::println(out, "  1  Trial balance (saldobalans)");
  std::println(out, "  2  Vouchers");
  std::println(out, "  3  Account ledger");
  std::println(out, "");
  std::println(out, "Press a number to choose, - to go back, h for help, q to quit.");
}

void Repl::on_reports_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'1') {
    show_report(books::trial_balance(books_, year_, company()), out);
  } else if (key == U'2') {
    show_report(books::voucher_list(books_, year_, company()), out);
  } else if (key == U'3') {
    purpose_ = Purpose::Account;
    editor_.start("", PathEditor::Kind::Account);
    mode_ = Mode::Typing;
    std::println(out, "Type the account number. Backspace erases, Enter accepts. Enter on an empty number goes back.");
    out << editor_.prompt();
  } else if (key == U'-' && before_reports_ == Mode::Year) {
    mode_ = Mode::Year;
  } else if (key == U'-') {
    mode_ = Mode::Years;
    show_years(out);
  } else {
    std::println(out, "Press a number to choose, - to go back, h for help, q to quit.");
  }
}

void Repl::show_report(std::vector<std::string> lines, std::ostream& out) {
  mode_ = Mode::Paging;
  if (pager_.open(std::move(lines), out) == Pager::Result::Leave) {
    mode_ = Mode::Reports;
    show_reports(out);
  }
}

void Repl::open_year_menu(std::ostream& out) {
  auto books = books::read_books(project_->folder);
  if (!books) {
    std::println(out, "Error: {}", books.error());
    return;
  }
  if (books->years.empty()) {
    std::println(out, "No books yet. Press i to import an SIE file.");
    return;
  }
  books_ = std::move(*books);
  first_open_ = 0;
  mode_ = Mode::YearMenu;
  show_year_menu(out);
}

// Numbered from 0, the newest, as SIE numbers years; ten at a time
void Repl::show_year_menu(std::ostream& out) {
  auto const n = books_.years.size();
  auto const last = std::min(first_open_ + 10, n);
  if (first_open_ == 0) {
    std::println(out, "Fiscal years of {} (0 is the newest):", company());
  } else {
    std::println(out, "Fiscal years {} to {} of {} (key 0 is year {}):", first_open_, last - 1, company(), first_open_);
  }
  if (first_open_ > 0) {
    std::println(out, "  <  ...newer");
  }
  for (auto i = first_open_; i < last; ++i) {
    auto const& year = books_.years[n - 1 - i];
    std::println(out, "  {}  {} - {}  {}", i - first_open_, format_iso_date(year.start), format_iso_date(year.end),
                 year.closed ? "closed" : "open");
  }
  if (last < n) {
    std::println(out, "  >  ...older");
  }
  std::println(out, "");
  std::println(out, "Press a number to open a year, - to go back, h for help, q to quit.");
}

void Repl::on_year_menu_key(charset::unicode::CodePoint key, std::ostream& out) {
  auto const n = books_.years.size();
  if (key == U'-') {
    mode_ = Mode::Project;
  } else if (key == U'<' && first_open_ > 0) {
    first_open_ -= 10;
    show_year_menu(out);
  } else if (key == U'>' && first_open_ + 10 < n) {
    first_open_ += 10;
    show_year_menu(out);
  } else if (key >= U'0' && key <= U'9' && first_open_ + static_cast<std::size_t>(key - U'0') < n) {
    auto const index = n - 1 - (first_open_ + static_cast<std::size_t>(key - U'0'));
    auto const saved = save_project_year(project_->folder, format_iso_date(books_.years[index].start));
    if (!saved) {
      std::println(out, "Error: {}", saved.error());
      return;
    }
    if (*saved) {
      std::println(out, "Updated .cratchit/project.txt in {}", project_->folder.string());
    }
    project_->year = format_iso_date(books_.years[index].start);
    enter_year(index, out);
  } else {
    std::println(out, "Press a number to open a year, - to go back, h for help, q to quit.");
  }
}

void Repl::enter_year(std::size_t index, std::ostream& out) {
  auto const& year = books_.years[index];
  open_year_ = year.start;
  mode_ = Mode::Year;
  std::println(out, "Opened fiscal year {} - {}, {}.", format_iso_date(year.start), format_iso_date(year.end),
               year.closed ? "closed for new vouchers" : "open for new vouchers");
  LOG_BUSINESS("Opened fiscal year {} - {} of project {} ({})", format_iso_date(year.start), format_iso_date(year.end),
               project_->name, project_->uuid);
}

// Opens the fiscal year named by `year` in project.txt, if any
void Repl::open_last_year(std::ostream& out) {
  if (project_->year.empty()) {
    return;
  }
  auto books = books::read_books(project_->folder);
  if (!books) {
    std::println(out, "Error: {}", books.error());
    return;
  }
  auto const start = parse_iso_date(project_->year);
  auto const* year = start ? books->year_starting(*start) : nullptr;
  if (!year) {
    std::println(out, "Error: The last fiscal year {} in .cratchit/project.txt is not in the books", project_->year);
    return;
  }
  auto const index = static_cast<std::size_t>(year - books->years.data());
  books_ = std::move(*books);
  enter_year(index, out);
}

// Reads the books again and finds the open year in them; nothing (with the error shown) if it is gone
std::optional<std::size_t> Repl::read_open_year(std::ostream& out) {
  auto books = books::read_books(project_->folder);
  if (!books) {
    std::println(out, "Error: {}", books.error());
    return std::nullopt;
  }
  books_ = std::move(*books);
  auto const* year = books_.year_starting(open_year_);
  if (!year) {
    std::println(out, "Error: Fiscal year {} is not in the books", format_iso_date(open_year_));
    return std::nullopt;
  }
  return static_cast<std::size_t>(year - books_.years.data());
}

void Repl::on_year_key(charset::unicode::CodePoint key, std::ostream& out) {
  auto const* year = books_.year_starting(open_year_);
  if (key == U'-') {
    mode_ = Mode::Project;
  } else if (key == U'l') {
    open_log(out);
  } else if (key == U'e') {
    start_entry(false, out);
  } else if (key == U'n') {
    start_entry(true, out);
  } else if (key == U'w') {
    open_waiting(out);
  } else if (key == U'v') {
    open_vouchers(out);
  } else if (key == U'r') {
    if (auto const index = read_open_year(out)) {
      year_ = *index;
      before_reports_ = Mode::Year;
      mode_ = Mode::Reports;
      show_reports(out);
    }
  } else if (key == U'c' && year && !year->closed) {
    set_closed(true, out);
  } else if (key == U'o' && year && year->closed) {
    set_closed(false, out);
  } else {
    std::println(out, "Unknown key.");
    show_keys(out);
  }
}

void Repl::set_closed(bool closed, std::ostream& out) {
  auto const index = read_open_year(out);
  if (!index) {
    return;
  }
  auto& year = books_.years[*index];
  year.closed = closed;
  if (!write_books(out)) {
    year.closed = !closed;
    return;
  }
  std::println(out, "Fiscal year {} - {} is now {} for new vouchers.", format_iso_date(year.start),
               format_iso_date(year.end), closed ? "closed" : "open");
  LOG_BUSINESS("{} fiscal year {} - {} of project {} ({}) for new vouchers", closed ? "Closed" : "Opened",
               format_iso_date(year.start), format_iso_date(year.end), project_->name, project_->uuid);
}

// The voucher line of a new voucher, or of an event to note when noting, starting with line; false when refused
bool Repl::start_entry(bool noting, std::ostream& out, std::string const& line) {
  auto const index = read_open_year(out);
  if (!index) {
    return false;
  }
  auto const& year = books_.years[*index];
  if (year.closed) {
    std::println(out, "Fiscal year {} - {} is closed for new vouchers. Press o to open it.", format_iso_date(year.start),
                 format_iso_date(year.end));
    return false;
  }
  draft_.reset();
  noting_ = noting;
  from_waiting_.reset();
  template_key_.reset();
  correcting_.reset();
  purpose_ = Purpose::Voucher;
  editor_.start(line, noting ? PathEditor::Kind::Event : PathEditor::Kind::Voucher);
  mode_ = Mode::Typing;
  std::println(out, "Type the heading, amount and date of the {}, e.g. Telia faktura oktober 1118,00 2026-10-22.",
               noting ? "event to post later" : "voucher");
  std::println(out, "No date means today. Backspace erases, Enter accepts. Enter on an empty line goes back.");
  out << editor_.prompt();
  return true;
}

// The heading, amount and date typed: shown for y/n, offered as readings when the
// amount is unclear, or refused with the line kept for editing
void Repl::on_voucher_line(std::ostream& out) {
  auto const year = static_cast<std::size_t>(books_.year_starting(open_year_) - books_.years.data());
  auto const& fiscal = books_.years[year];
  auto today = parse_iso_date(places_.today);
  if (today && !fiscal.holds(*today)) {
    today.reset();
  }
  auto readings = books::parse_event(editor_.text(), today);
  if (!readings) {
    std::println(out, "{}", readings.error());
    out << editor_.prompt();
    return;
  }
  if (!fiscal.holds(readings->front().date)) {
    std::println(out, "Refused: {} is outside fiscal year {} - {}.", format_iso_date(readings->front().date),
                 format_iso_date(fiscal.start), format_iso_date(fiscal.end));
    out << editor_.prompt();
    return;
  }
  if (readings->size() == 1) {
    show_event(readings->front(), year, out);
    return;
  }
  readings_ = std::move(*readings);
  mode_ = Mode::ChooseAmount;
  show_readings(out);
}

void Repl::show_readings(std::ostream& out) {
  std::println(out, "The amount is unclear: spaces may separate thousands. Which is it?");
  for (std::size_t i = 0; i < readings_.size(); ++i) {
    std::println(out, "  {}  {:>15}  {}", i + 1, format_amount(readings_[i].amount), readings_[i].heading);
  }
  std::println(out, "");
  std::println(out, "Press a number to choose, - to change the line, h for help, q to quit.");
}

void Repl::on_choose_amount_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key >= U'1' && key <= U'9' && static_cast<std::size_t>(key - U'1') < readings_.size()) {
    auto const event = readings_[static_cast<std::size_t>(key - U'1')];
    show_event(event, static_cast<std::size_t>(books_.year_starting(open_year_) - books_.years.data()), out);
  } else if (key == U'-') {
    mode_ = Mode::Typing;
    out << editor_.prompt();
  } else {
    std::println(out, "Press a number to choose, - to change the line, h for help, q to quit.");
  }
}

void Repl::show_event(books::Event const& event, std::size_t year, std::ostream& out) {
  auto const& fiscal = books_.years[year];
  std::println(out, "  Heading  {}", event.heading);
  std::println(out, "  Amount   {}", format_amount(event.amount));
  std::println(out, "  Date     {}   (fiscal year {} - {})", format_iso_date(event.date), format_iso_date(fiscal.start),
               format_iso_date(fiscal.end));
  draft_ = books::Draft{event, year, {}, {}};
  mode_ = Mode::ConfirmEvent;
}

void Repl::on_confirm_event_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'y' && noting_) {
    note_event(out);
  } else if (key == U'y' && (correcting_ || template_key_)) {
    templates_.clear();
    use_template(voucher_of(correcting_ ? *correcting_ : *template_key_), out, correcting_.has_value());
  } else if (key == U'y') {
    offer_templates(out);
  } else if (key == U'n' || key == U'-') {
    mode_ = Mode::Typing;
    out << editor_.prompt();
  } else {
    std::println(out, "Press y to go on, n to change the line.");
  }
}

// Earlier vouchers like the draft's event to post it after, or straight to the series when there are none
void Repl::offer_templates(std::ostream& out) {
  templates_ = books::find_templates(books_, draft_->event);
  if (templates_.empty()) {
    use_template(nullptr, out);
    return;
  }
  mode_ = Mode::Template;
  show_templates(out);
}

void Repl::show_templates(std::ostream& out) const {
  std::println(out, "Earlier vouchers like this one:");
  for (std::size_t i = 0; i < templates_.size(); ++i) {
    auto const& like = *templates_[i];
    std::println(out, "  {}  {} {}  {}  {:>12}  {}", i + 1, like.series, like.number, format_iso_date(like.event.date),
                 format_amount(like.event.amount), like.event.heading);
    std::string rows;
    for (auto const& posting : like.postings) {
      if (posting.kind != books::Posting::Kind::removed) {
        rows += std::format("  {} {}", posting.account, format_amount(posting.amount));
      }
    }
    std::println(out, "    {}", rows);
  }
  std::println(out, "");
  std::println(out, "Press a number or Enter (= 1) to use it as a template, n for none, - to go back, h for help, q to quit.");
}

void Repl::on_template_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'\n' || key == U'\r') {
    use_template(templates_.front(), out);
  } else if (key >= U'1' && key <= U'9' && static_cast<std::size_t>(key - U'1') < templates_.size()) {
    use_template(templates_[static_cast<std::size_t>(key - U'1')], out);
  } else if (key == U'n') {
    use_template(nullptr, out);
  } else if (key == U'-' && from_waiting_) {
    draft_.reset();
    from_waiting_.reset();
    mode_ = Mode::Waiting;
    show_waiting(out);
  } else if (key == U'-') {
    purpose_ = Purpose::Voucher;
    mode_ = Mode::Typing;
    out << editor_.prompt();
  } else {
    std::println(out, "Press a number or Enter (= 1) to use it as a template, n for none, - to go back, h for help, q to quit.");
  }
}

// The template's rows guide the postings and its series is the default; without one, the guessed series
// reverse (to correct like) flips the rows
void Repl::use_template(books::Voucher const* like, std::ostream& out, bool reverse) {
  draft_->guide.clear();
  draft_->from_template.clear();
  draft_->corrects.clear();
  if (like && reverse) {
    draft_->corrects = std::format("{} {}", like->series, like->number);
  }
  if (like) {
    auto guide = books::guide_of(books_, *like, reverse ? -draft_->event.amount : draft_->event.amount);
    for (auto const& account : guide.missing) {
      std::println(out, "Account {} of {} {} is not in the books, so it is not suggested.", account, like->series,
                   like->number);
    }
    draft_->guide = std::move(guide.postings);
    draft_->from_template = std::format("{} {}", like->series, like->number);
    guess_ = {like->series, like};
  } else {
    guess_ = books::guess_series(books_, draft_->event.heading);
  }
  mode_ = Mode::Series;
  show_series(out);
}

void Repl::show_series(std::ostream& out) {
  if (guess_.like) {
    std::println(out, "Series (Enter = {}, as {} {} {} \"{}\"):", guess_.series, guess_.like->series, guess_.like->number,
                 format_iso_date(guess_.like->event.date), guess_.like->event.heading);
  } else {
    std::println(out, "Series (Enter = {}):", guess_.series);
  }
  for (auto const& series : books::series_names()) {
    std::println(out, "  {}  {}", series.letter, series.name);
  }
  std::println(out, "");
  std::println(out, "Press a letter or Enter to choose, - to go back, q to quit.");
}

void Repl::on_series_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'-' && !templates_.empty()) {
    mode_ = Mode::Template;
    show_templates(out);
    return;
  }
  if (key == U'-' && from_waiting_) {
    draft_.reset();
    from_waiting_.reset();
    mode_ = Mode::Waiting;
    show_waiting(out);
    return;
  }
  if (key == U'-') {
    purpose_ = Purpose::Voucher;
    mode_ = Mode::Typing;
    out << editor_.prompt();
    return;
  }
  std::string letter;
  if (key == U'\n' || key == U'\r') {
    letter = guess_.series;
  } else if (key >= U'a' && key <= U'z') {
    letter = static_cast<char>(key - U'a' + U'A');
  } else if (key >= U'A' && key <= U'Z') {
    letter = static_cast<char>(key);
  }
  if (letter.empty() || !books::series_name(letter)) {
    std::println(out, "Press a letter or Enter to choose, - to go back, q to quit.");
    return;
  }
  draft_->series = letter;
  number_ = books::next_number(books_.years[draft_->year], letter);
  std::println(out, "Voucher {} {} ({}) {} \"{}\" {}", letter, number_, books::series_name(letter)->name,
               format_iso_date(draft_->event.date), draft_->event.heading, format_amount(draft_->event.amount));
  std::println(out, "Type one row per line: an account and an amount, debit positive, credit negative, e.g. 1930 -1118.");
  std::println(out, "An account alone gets what remains (on the first row, the amount). A number prefix or a word of");
  std::println(out, "the name lists accounts. - removes the last row. Enter saves when debit and credit each add up");
  std::println(out, "to the amount.");
  if (!draft_->guide.empty() && !draft_->corrects.empty()) {
    std::println(out, "The rows of {}, reversed, are suggested one at a time: Enter takes the suggestion,",
                 draft_->corrects);
    std::println(out, "or type your own row instead.");
  } else if (!draft_->guide.empty()) {
    std::println(out, "Rows from {} are suggested one at a time, scaled to the amount: Enter takes the suggestion,",
                 draft_->from_template);
    std::println(out, "or type your own row instead.");
  }
  start_rows(out);
}

void Repl::start_rows(std::ostream& out) {
  if (auto const suggested = draft_->suggestion()) {
    auto const* account = books_.account(suggested->account);
    std::println(out, "  Suggested {:<6} {:>12}  {}", suggested->account, format_amount(suggested->amount),
                 account ? account->name : "");
  }
  purpose_ = Purpose::Row;
  editor_.start("", PathEditor::Kind::Row);
  mode_ = Mode::Typing;
  out << editor_.prompt();
}

void Repl::on_row(PathEditor::Result result, std::ostream& out) {
  auto& rows = draft_->postings;
  auto const text = trimmed(editor_.text());
  if (result == PathEditor::Result::Cancelled || text.empty()) {
    if (draft_->is_balanced()) {
      std::println(out, "Voucher {} {} {} \"{}\"", draft_->series, number_, format_iso_date(draft_->event.date),
                   draft_->event.heading);
      for (auto const& row : rows) {
        show_row(row, out);
      }
      mode_ = Mode::ConfirmSave;
      return;
    }
    if (auto const suggested = draft_->suggestion()) {
      rows.push_back(*suggested);
      show_row(rows.back(), out);
      show_remaining(out);
      start_rows(out);
      return;
    }
    if (rows.empty()) {
      std::println(out, "No rows yet. Type an account and an amount, or - to drop the voucher.");
    } else {
      show_remaining(out);
    }
    start_rows(out);
    return;
  }
  if (text == "-") {
    if (rows.empty()) {
      std::println(out, "Dropped the voucher. Nothing is saved.{}", from_waiting_ ? " The event is still waiting." : "");
      draft_.reset();
      from_waiting_.reset();
      mode_ = Mode::Year;
      return;
    }
    std::print(out, "Removed");
    show_row(rows.back(), out);
    rows.pop_back();
    show_remaining(out);
    start_rows(out);
    return;
  }
  auto const row = books::parse_row(text);
  if (!row) {
    std::println(out, "{}", row.error());
    start_rows(out);
    return;
  }
  if (!books_.account(row->account)) {
    show_accounts(row->amount ? row->account : text, out);
    start_rows(out);
    return;
  }
  auto const amount = row->amount ? *row->amount : rows.empty() ? draft_->event.amount : draft_->remaining();
  if (amount == 0) {
    std::println(out, "{}", row->amount         ? "The amount is zero."
                            : draft_->is_balanced() ? "Nothing remains. Enter saves, - removes the last row."
                                                    : "Type an amount: the rows add up to zero, but not to the amount.");
    start_rows(out);
    return;
  }
  books::Posting posting;
  posting.account = row->account;
  posting.amount = amount;
  rows.push_back(std::move(posting));
  show_row(rows.back(), out);
  show_remaining(out);
  start_rows(out);
}

void Repl::show_row(books::Posting const& row, std::ostream& out) const {
  auto const* account = books_.account(row.account);
  std::println(out, "  {:<6} {:>12}  {}", row.account, format_amount(row.amount), account ? account->name : "");
}

void Repl::show_remaining(std::ostream& out) const {
  std::println(out, "  {}", draft_->status());
}

// The accounts matching a query that is not an account in the books
void Repl::show_accounts(std::string const& query, std::ostream& out) const {
  constexpr std::size_t max_shown = 20;
  auto const found = books::find_accounts(books_, query);
  if (found.empty()) {
    std::println(out, "No account matches {}.", query);
    return;
  }
  std::println(out, "Accounts matching {}:", query);
  for (std::size_t i = 0; i < std::min(found.size(), max_shown); ++i) {
    std::println(out, "  {}  {}", found[i]->number, found[i]->name);
  }
  if (found.size() > max_shown) {
    std::println(out, "  and {} more. Type more of the number or name.", found.size() - max_shown);
  }
}

void Repl::on_confirm_save_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'y') {
    save_voucher(out);
  } else if (key == U'n' || key == U'-') {
    start_rows(out);
  } else {
    std::println(out, "Press y to save the voucher, n to change the rows.");
  }
}

void Repl::save_voucher(std::ostream& out) {
  mode_ = Mode::Year;
  auto const draft = std::move(*draft_);
  draft_.reset();
  auto const from_waiting = std::exchange(from_waiting_, std::nullopt);
  auto& year = books_.years[draft.year];
  std::optional<books::WaitingEvent> waited;
  if (from_waiting) {
    waited = year.waiting[*from_waiting];
    year.waiting.erase(year.waiting.begin() + static_cast<std::ptrdiff_t>(*from_waiting));
  }
  year.vouchers.push_back(draft.voucher(number_, parse_iso_date(places_.today).value_or(draft.event.date)));
  if (!write_books(out)) {
    year.vouchers.pop_back();
    if (waited) {
      year.waiting.insert(year.waiting.begin() + static_cast<std::ptrdiff_t>(*from_waiting), *waited);
    }
    return;
  }
  std::println(out, "Saved voucher {} {} into fiscal year {} - {}.", draft.series, number_, format_iso_date(year.start),
               format_iso_date(year.end));
  if (waited) {
    std::println(out, "The event is no longer waiting ({} still waiting).", year.waiting.size());
  }
  LOG_BUSINESS("Saved voucher {} {} {} \"{}\" ({} rows, debit {}){}{} into project {} ({}), fiscal year {} - {}",
               draft.series, number_, format_iso_date(draft.event.date), draft.event.heading, draft.postings.size(),
               format_amount(books::debit(draft.postings)), waited ? " from a waiting event" : "",
               !draft.corrects.empty()        ? " correcting " + draft.corrects
               : draft.from_template.empty() ? ""
                                             : " with template " + draft.from_template,
               project_->name, project_->uuid, format_iso_date(year.start), format_iso_date(year.end));
  if (correcting_) {
    corrected_ = std::exchange(correcting_, std::nullopt);
    std::println(out, "{} {} reverses {}. The right voucher can follow, with {} as the template.", draft.series, number_,
                 draft.corrects, draft.corrects);
    mode_ = Mode::ConfirmRight;
  }
}

bool Repl::write_books(std::ostream& out) {
  auto const written = books::write_books(project_->folder, books_);
  if (!written) {
    std::println(out, "Error: {}", written.error());
    return false;
  }
  for (auto const& file : *written) {
    std::println(out, "{} {} in {}", change_word(file), file.file.generic_string(), project_->folder.string());
  }
  return true;
}

// The event typed, noted in its year's waiting events
void Repl::note_event(std::ostream& out) {
  mode_ = Mode::Year;
  auto const draft = std::move(*draft_);
  draft_.reset();
  auto& year = books_.years[draft.year];
  books::Event const event{draft.event.heading, draft.target(), draft.event.date};
  year.waiting.push_back({event, parse_iso_date(places_.today).value_or(event.date)});
  if (!write_books(out)) {
    year.waiting.pop_back();
    return;
  }
  std::println(out, "Noted: {} {} {} ({} waiting). Press w to post it.", format_iso_date(event.date), event.heading,
               format_amount(event.amount), year.waiting.size());
  LOG_BUSINESS("Noted waiting event {} \"{}\" {} in project {} ({}), fiscal year {} - {}", format_iso_date(event.date),
               event.heading, format_amount(event.amount), project_->name, project_->uuid,
               format_iso_date(year.start), format_iso_date(year.end));
}

void Repl::open_waiting(std::ostream& out) {
  auto const index = read_open_year(out);
  if (!index) {
    return;
  }
  auto const& year = books_.years[*index];
  if (year.waiting.empty()) {
    std::println(out, "No events are waiting in fiscal year {} - {}. Press n to note one.", format_iso_date(year.start),
                 format_iso_date(year.end));
    return;
  }
  first_waiting_ = 0;
  deleting_ = false;
  mode_ = Mode::Waiting;
  show_waiting(out);
}

// Numbered from 1 in the order noted; over 9, keys 1-8 with 0 up and 9 down
void Repl::show_waiting(std::ostream& out) {
  auto const& year = *books_.year_starting(open_year_);
  auto const n = year.waiting.size();
  auto const first = n <= max_keys ? 0 : first_waiting_;
  auto const last = n <= max_keys ? n : std::min(first + range_size, n);
  std::println(out, "Waiting events in fiscal year {} - {}:", format_iso_date(year.start), format_iso_date(year.end));
  if (first > 0) {
    std::println(out, "  0  ...up");
  }
  for (auto i = first; i < last; ++i) {
    auto const& event = year.waiting[i].event;
    std::println(out, "  {}  {}  {:>12}  {}", i - first + 1, format_iso_date(event.date), format_amount(event.amount),
                 event.heading);
  }
  if (last < n) {
    std::println(out, "  9  ...down");
  }
  std::println(out, "");
  waiting_hint(out);
}

void Repl::waiting_hint(std::ostream& out) const {
  if (deleting_) {
    std::println(out, "Press the number of the event to delete, - not to delete.");
  } else {
    std::println(out, "Press a number to post it, d to delete one, - to go back, h for help, q to quit.");
  }
}

void Repl::on_waiting_key(charset::unicode::CodePoint key, std::ostream& out) {
  auto const& year = *books_.year_starting(open_year_);
  auto const n = year.waiting.size();
  if (key == U'-' && deleting_) {
    deleting_ = false;
    waiting_hint(out);
  } else if (key == U'-') {
    mode_ = Mode::Year;
  } else if (key == U'd' && !deleting_) {
    deleting_ = true;
    waiting_hint(out);
  } else if (n > max_keys && key == U'0' && first_waiting_ > 0) {
    first_waiting_ -= range_size;
    show_waiting(out);
  } else if (n > max_keys && key == U'9' && first_waiting_ + range_size < n) {
    first_waiting_ += range_size;
    show_waiting(out);
  } else if (key >= U'1' && key <= (n > max_keys ? U'8' : U'9') &&
             (n > max_keys ? first_waiting_ : 0) + static_cast<std::size_t>(key - U'1') < n) {
    auto const index = (n > max_keys ? first_waiting_ : 0) + static_cast<std::size_t>(key - U'1');
    if (deleting_) {
      auto const& event = year.waiting[index].event;
      std::println(out, "  {}  {:>12}  {}", format_iso_date(event.date), format_amount(event.amount), event.heading);
      to_delete_ = index;
      mode_ = Mode::ConfirmDelete;
    } else {
      post_waiting(index, out);
    }
  } else {
    waiting_hint(out);
  }
}

// Enters a voucher for a waiting event: its series, then its rows
void Repl::post_waiting(std::size_t index, std::ostream& out) {
  auto const year_index = static_cast<std::size_t>(books_.year_starting(open_year_) - books_.years.data());
  auto const& year = books_.years[year_index];
  if (year.closed) {
    std::println(out, "Fiscal year {} - {} is closed for new vouchers. Press - and then o to open it.",
                 format_iso_date(year.start), format_iso_date(year.end));
    return;
  }
  noting_ = false;
  from_waiting_ = index;
  template_key_.reset();
  correcting_.reset();
  draft_ = books::Draft{year.waiting[index].event, year_index, {}, {}};
  offer_templates(out);
}

void Repl::on_confirm_delete_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'y') {
    auto& year = books_.years[static_cast<std::size_t>(books_.year_starting(open_year_) - books_.years.data())];
    auto const deleted = year.waiting[to_delete_];
    year.waiting.erase(year.waiting.begin() + static_cast<std::ptrdiff_t>(to_delete_));
    if (!write_books(out)) {
      year.waiting.insert(year.waiting.begin() + static_cast<std::ptrdiff_t>(to_delete_), deleted);
      mode_ = Mode::Waiting;
      return;
    }
    std::println(out, "Deleted the waiting event {} \"{}\" {}.", format_iso_date(deleted.event.date),
                 deleted.event.heading, format_amount(deleted.event.amount));
    LOG_BUSINESS("Deleted waiting event {} \"{}\" {} from project {} ({}), fiscal year {} - {}",
                 format_iso_date(deleted.event.date), deleted.event.heading, format_amount(deleted.event.amount),
                 project_->name, project_->uuid, format_iso_date(year.start), format_iso_date(year.end));
    deleting_ = false;
    if (year.waiting.empty()) {
      std::println(out, "No events are waiting now.");
      mode_ = Mode::Year;
      return;
    }
    first_waiting_ = std::min(first_waiting_, (year.waiting.size() - 1) / range_size * range_size);
    mode_ = Mode::Waiting;
    show_waiting(out);
  } else if (key == U'n' || key == U'-') {
    deleting_ = false;
    mode_ = Mode::Waiting;
    show_waiting(out);
  } else {
    std::println(out, "Press y to delete the event, n not to.");
  }
}

books::Voucher const* Repl::voucher_of(books::VoucherKey const& key) const {
  auto const* year = books_.year_starting(key.year);
  if (!year) {
    return nullptr;
  }
  auto const it = std::ranges::find_if(year->vouchers, [&](books::Voucher const& voucher) {
    return voucher.series == key.series && voucher.number == key.number;
  });
  return it == year->vouchers.end() ? nullptr : &*it;
}

books::VoucherKey Repl::key_of(books::Voucher const& voucher) const {
  return {open_year_, voucher.series, voucher.number};
}

// The series of the open year's vouchers, to list one of them
void Repl::open_vouchers(std::ostream& out) {
  auto const index = read_open_year(out);
  if (!index) {
    return;
  }
  auto const& year = books_.years[*index];
  if (year.vouchers.empty()) {
    std::println(out, "No vouchers in fiscal year {} - {}.", format_iso_date(year.start), format_iso_date(year.end));
    return;
  }
  auto imported = books::imported_vouchers(project_->folder);
  if (!imported) {
    std::println(out, "Error: {}", imported.error());
    return;
  }
  imported_ = std::move(*imported);
  mode_ = Mode::VoucherSeries;
  show_voucher_series(out);
}

void Repl::show_voucher_series(std::ostream& out) const {
  auto const& year = *books_.year_starting(open_year_);
  std::set<std::string> series;
  for (auto const& voucher : year.vouchers) {
    series.insert(voucher.series);
  }
  std::println(out, "Series in fiscal year {} - {}:", format_iso_date(year.start), format_iso_date(year.end));
  for (auto const& letter : series) {
    std::size_t count = 0;
    books::Voucher const* last = nullptr;
    for (auto const& voucher : year.vouchers) {
      if (voucher.series == letter) {
        ++count;
        if (!last || number_less(last->number, voucher.number)) {
          last = &voucher;
        }
      }
    }
    auto const name = books::series_name(letter);
    std::println(out, "  {}  {:<32} {:>4} voucher{}, last {} {} {}", letter, name ? name->name : "", count,
                 count == 1 ? "" : "s", last->series, last->number, format_iso_date(last->event.date));
  }
  std::println(out, "");
  std::println(out, "Press a letter to list that series, - to go back, q to quit.");
}

void Repl::on_voucher_series_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'-') {
    mode_ = Mode::Year;
    return;
  }
  std::string letter;
  if (key >= U'a' && key <= U'z') {
    letter = static_cast<char>(key - U'a' + U'A');
  } else if (key >= U'A' && key <= U'Z') {
    letter = static_cast<char>(key);
  }
  auto const& year = *books_.year_starting(open_year_);
  if (letter.empty() ||
      std::ranges::none_of(year.vouchers, [&](books::Voucher const& voucher) { return voucher.series == letter; })) {
    std::println(out, "Press a letter to list that series, - to go back, q to quit.");
    return;
  }
  view_series_ = letter;
  list_vouchers(out);
}

// The vouchers of the series, newest (highest number) first
void Repl::list_vouchers(std::ostream& out) {
  auto const& year = *books_.year_starting(open_year_);
  listed_vouchers_.clear();
  for (std::size_t i = 0; i < year.vouchers.size(); ++i) {
    if (year.vouchers[i].series == view_series_) {
      listed_vouchers_.push_back(i);
    }
  }
  std::ranges::sort(listed_vouchers_, [&](std::size_t a, std::size_t b) {
    return number_less(year.vouchers[b].number, year.vouchers[a].number);
  });
  first_voucher_ = 0;
  mode_ = Mode::VoucherList;
  show_vouchers(out);
}

// Numbered from 1, newest first; over 9, keys 1-8 with 0 up and 9 down
void Repl::show_vouchers(std::ostream& out) const {
  auto const& year = *books_.year_starting(open_year_);
  auto const n = listed_vouchers_.size();
  auto const first = n <= max_keys ? 0 : first_voucher_;
  auto const last = n <= max_keys ? n : std::min(first + range_size, n);
  std::println(out, "Vouchers {} in fiscal year {} - {}, newest first:", view_series_, format_iso_date(year.start),
               format_iso_date(year.end));
  if (first > 0) {
    std::println(out, "  0  ...up");
  }
  for (auto i = first; i < last; ++i) {
    auto const& voucher = year.vouchers[listed_vouchers_[i]];
    std::println(out, "  {}  {} {:<4}  {}  {:>12}  {}  ({})", i - first + 1, voucher.series, voucher.number,
                 format_iso_date(voucher.event.date), format_amount(voucher.event.amount), voucher.event.heading,
                 imported_.contains(key_of(voucher)) ? "imported" : "entered");
  }
  if (last < n) {
    std::println(out, "  9  ...down");
  }
  std::println(out, "");
  std::println(out, "Press a number to view it, - to go back, h for help, q to quit.");
}

void Repl::on_voucher_list_key(charset::unicode::CodePoint key, std::ostream& out) {
  auto const n = listed_vouchers_.size();
  if (key == U'-') {
    mode_ = Mode::VoucherSeries;
    show_voucher_series(out);
  } else if (n > max_keys && key == U'0' && first_voucher_ > 0) {
    first_voucher_ -= range_size;
    show_vouchers(out);
  } else if (n > max_keys && key == U'9' && first_voucher_ + range_size < n) {
    first_voucher_ += range_size;
    show_vouchers(out);
  } else if (key >= U'1' && key <= (n > max_keys ? U'8' : U'9') &&
             (n > max_keys ? first_voucher_ : 0) + static_cast<std::size_t>(key - U'1') < n) {
    viewed_ = listed_vouchers_[(n > max_keys ? first_voucher_ : 0) + static_cast<std::size_t>(key - U'1')];
    mode_ = Mode::VoucherView;
    show_voucher(out);
  } else {
    std::println(out, "Press a number to view it, - to go back, h for help, q to quit.");
  }
}

void Repl::show_voucher(std::ostream& out) const {
  auto const& voucher = books_.year_starting(open_year_)->vouchers[viewed_];
  std::println(out, "Voucher {} {} {} \"{}\" {}", voucher.series, voucher.number, format_iso_date(voucher.event.date),
               voucher.event.heading, format_amount(voucher.event.amount));
  for (auto const& posting : voucher.postings) {
    auto const* account = books_.account(posting.account);
    auto const mark = posting.kind == books::Posting::Kind::added     ? "  (added)"
                      : posting.kind == books::Posting::Kind::removed ? "  (removed)"
                                                                      : "";
    std::println(out, "  {:<6} {:>12}  {}{}", posting.account, format_amount(posting.amount),
                 account ? account->name : "", mark);
  }
  auto const imported = imported_.contains(key_of(voucher));
  if (voucher.registered) {
    std::println(out, "  Registered {}, {}", format_iso_date(*voucher.registered),
                 imported ? "imported from an SIE file" : "entered in cratchit");
  } else {
    std::println(out, "  {}", imported ? "Imported from an SIE file" : "Entered in cratchit");
  }
  std::println(out, "");
  std::println(out, "Press c to correct it, t to use it as a template, - to go back, h for help, q to quit.");
}

void Repl::on_voucher_view_key(charset::unicode::CodePoint key, std::ostream& out) {
  auto const voucher = books_.year_starting(open_year_)->vouchers[viewed_];
  auto const viewed = key_of(voucher);
  if (key == U'-') {
    mode_ = Mode::VoucherList;
    show_vouchers(out);
  } else if (key == U'c' && imported_.contains(viewed)) {
    std::println(out, "{} {} came with an SIE import, so cratchit does not correct it. Correct it in the program it",
                 voucher.series, voucher.number);
    std::println(out, "came from and import the file again.");
  } else if (key == U'c') {
    // The line is the correction's heading and amount; no date means today
    auto const line = std::format("Rättelse av {} {}: {} {}", voucher.series, voucher.number, voucher.event.heading,
                                  format_amount(voucher.event.amount));
    if (start_entry(false, out, line)) {
      correcting_ = viewed;
    }
  } else if (key == U't') {
    if (start_entry(false, out)) {
      template_key_ = viewed;
    }
  } else {
    std::println(out, "Press c to correct it, t to use it as a template, - to go back, h for help, q to quit.");
  }
}

// After a correction is saved: the right voucher, with the corrected one as its template
void Repl::on_confirm_right_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'y') {
    auto const corrected = *std::exchange(corrected_, std::nullopt);
    auto const* voucher = voucher_of(corrected);
    auto const line = voucher ? std::format("{} {}", voucher->event.heading, format_amount(voucher->event.amount)) : "";
    if (start_entry(false, out, line)) {
      template_key_ = corrected;
    } else {
      mode_ = Mode::Year;
    }
  } else if (key == U'n' || key == U'-') {
    corrected_.reset();
    mode_ = Mode::Year;
  } else {
    std::println(out, "Press y to enter the right voucher, n not to.");
  }
}

void run_repl(Repl& repl, KeySource const& next_key, std::ostream& out) {
  while (true) {
    bool const typing = repl.is_typing();
    if (!typing) {
      std::print(out, "{}", repl.prompt());
    }
    out.flush();
    std::optional<charset::unicode::CodePoint> key;
    do {
      key = next_key();
    } while (key && repl.ignores(*key));
    if (!key) {
      std::println(out, "");
      return;
    }
    if (!typing) {
      std::println(out, "{}", echo_of(*key));
    }
    if (repl.on_key(*key, out) == Step::Quit) {
      if (typing) {
        std::println(out, "");
      }
      return;
    }
  }
}

} // namespace cratchit
