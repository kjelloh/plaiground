#include "core/repl.hpp"

#include "core/books/report.hpp"
#include "core/charset/utf8.hpp"
#include "core/log.hpp"
#include "core/sie/file.hpp"

#include <algorithm>
#include <format>
#include <print>
#include <string>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

constexpr charset::unicode::CodePoint ctrl_d = U'\x04';
// Esc does nothing: arrow keys also start with it, so the two can't be told apart
constexpr charset::unicode::CodePoint escape = U'\x1b';

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
    } else if (key == U'i') {
      start_import(out);
    } else if (key == U'r') {
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
  // Enter lists the keys at a prompt
  if (is_prompt() && (key == U'\n' || key == U'\r')) {
    return false;
  }
  // Space and Enter show the next page of help or the log
  if ((mode_ == Mode::Help && help_.is_paging()) || mode_ == Mode::Log || mode_ == Mode::Paging) {
    return key == U'\t';
  }
  return is_whitespace(key);
}

bool Repl::is_prompt() const {
  return mode_ == Mode::Main || mode_ == Mode::Workspace || mode_ == Mode::Project;
}

void Repl::show_keys(std::ostream& out) const {
  std::println(out, "Keys at {}:", prompt().substr(0, prompt().size() - 1));
  if (mode_ == Mode::Main) {
    std::println(out, "  w  Workspace menu");
  } else if (mode_ == Mode::Workspace) {
    std::println(out, "  p  Projects");
  } else {
    std::println(out, "  i  Import an SIE file");
    std::println(out, "  r  Reports");
  }
  std::println(out, "  l  Show the log");
  if (mode_ == Mode::Workspace) {
    std::println(out, "  -  Back to cratchit>");
  } else if (mode_ == Mode::Project) {
    std::println(out, "  -  Back to workspace>");
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
    std::println(out, "{} {} in {}", written.created ? "Initiated" : "Updated", written.file.generic_string(),
                 project_->folder.string());
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
