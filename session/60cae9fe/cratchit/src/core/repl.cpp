#include "core/repl.hpp"

#include "core/charset/utf8.hpp"
#include "core/log.hpp"

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
    : places_(std::move(places)), log_file_(std::move(log_file)), help_(manual, page_lines), log_view_(page_lines) {}

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
  switch (mode_) {
  case Mode::Main:
    if (key == U'w') {
      open_menu(out);
    } else if (key == U'l') {
      open_log(out);
    } else {
      std::println(out, "Unknown key. Press h for help.");
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
      std::println(out, "Unknown key. Press h for help.");
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
    } else {
      std::println(out, "Unknown key. Press h for help.");
    }
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
  // Space and Enter show the next page of help or the log
  if ((mode_ == Mode::Help && help_.is_paging()) || mode_ == Mode::Log) {
    return key == U'\t';
  }
  return is_whitespace(key);
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
  mode_ = Mode::Project;
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
