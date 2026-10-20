#include "core/repl.hpp"

#include "core/workspace.hpp"

#include <print>
#include <string_view>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

constexpr char ctrl_d = '\x04';
// Esc does nothing: arrow keys also start with it, so the two can't be told apart
constexpr char escape = '\x1b';

bool is_whitespace(char key) {
  return key == '\n' || key == '\r' || key == ' ' || key == '\t';
}

// How a key press is shown after the prompt, as echo is off in raw mode
std::string_view echo_of(char const& key) {
  if (key == ctrl_d) {
    return "^D";
  }
  if (is_whitespace(key)) {
    return "";
  }
  return {&key, 1};
}

} // namespace

Repl::Repl(Section const& manual, std::size_t page_lines, WorkspacePlaces places)
    : places_(std::move(places)), help_(manual, page_lines) {}

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
  mode_ = Mode::Choose;
  show_choices(out);
}

Step Repl::on_key(char key, std::ostream& out) {
  if (key == ctrl_d) {
    return Step::Quit;
  }
  switch (mode_) {
  case Mode::Help:
    if (help_.on_key(key, out) == HelpBrowser::Result::Leave) {
      mode_ = before_help_;
      if (mode_ == Mode::Choose) {
        show_choices(out);
      }
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
  if (key == 'q') {
    return Step::Quit;
  }
  if (key == 'h') {
    open_help(out);
    return Step::Continue;
  }
  switch (mode_) {
  case Mode::Main:
    if (key == 'w') {
      open_workspace(out);
    } else {
      std::println(out, "Unknown key. Press h for help.");
    }
    break;
  case Mode::Choose:
    on_choose_key(key, out);
    break;
  case Mode::Workspace:
    if (key == '-') {
      mode_ = Mode::Main;
    } else {
      std::println(out, "Unknown key. Press h for help.");
    }
    break;
  default:
    break;
  }
  return Step::Continue;
}

bool Repl::ignores(char key) const {
  // The path editor uses every key, and drops arrow keys itself
  if (mode_ == Mode::Typing) {
    return false;
  }
  if (key == escape) {
    return true;
  }
  // Space and Enter show the next page of help
  if (mode_ == Mode::Help && help_.is_paging()) {
    return key == '\t';
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
  case Mode::Choose:
    return "workspace?> ";
  case Mode::Typing:
    return editor_.prompt();
  case Mode::ConfirmCreate:
    return "Create it? (y/n) ";
  case Mode::Workspace:
    return "workspace> ";
  default:
    return "cratchit> ";
  }
}

void Repl::open_help(std::ostream& out) {
  before_help_ = mode_;
  mode_ = Mode::Help;
  help_.open(out);
}

void Repl::show_choices(std::ostream& out) {
  std::println(out, "Choose a workspace folder:");
  if (places_.default_folder) {
    std::println(out, "  1  {}", places_.default_folder->string());
  }
  std::println(out, "  2  {}", places_.start_folder.string());
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

void Repl::on_choose_key(char key, std::ostream& out) {
  if (key == '1' && places_.default_folder) {
    create_and_choose(*places_.default_folder, out);
  } else if (key == '2') {
    enter_workspace(places_.start_folder, true, out);
  } else if (key == '3') {
    editor_.start(places_.start_folder.root_path().string());
    start_typing(out);
  } else if (key == '-') {
    mode_ = Mode::Main;
  } else {
    std::println(out, "Press a number to choose, - to go back, h for help, q to quit.");
  }
}

void Repl::on_typing_key(char key, std::ostream& out) {
  auto const result = editor_.on_key(key, out);
  if (result == PathEditor::Result::Cancelled) {
    mode_ = Mode::Choose;
    show_choices(out);
    return;
  }
  if (result != PathEditor::Result::Accepted) {
    return;
  }
  auto folder = fs::path(editor_.text());
  if (folder.is_relative()) {
    folder = places_.start_folder / folder;
  }
  folder = folder.lexically_normal();
  if (!folder.has_filename() && folder.has_relative_path()) {
    folder = folder.parent_path(); // drop a final separator
  }
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

void Repl::on_confirm_key(char key, std::ostream& out) {
  if (key == 'y') {
    create_and_choose(to_create_, out);
  } else if (key == 'n' || key == '-') {
    mode_ = Mode::Typing;
    out << editor_.prompt();
  } else {
    std::println(out, "Press y to create the folder, n to change the path.");
  }
}

void Repl::create_and_choose(fs::path const& folder, std::ostream& out) {
  std::error_code ec;
  if (!fs::is_directory(folder, ec)) {
    if (!fs::create_directories(folder, ec)) {
      std::println(out, "Error: Cannot create {}: {}", folder.string(), ec.message());
      mode_ = Mode::Choose;
      show_choices(out);
      return;
    }
    std::println(out, "Created folder {}", folder.string());
  }
  enter_workspace(folder, true, out);
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
  if (chosen) {
    if (auto const saved = save_workspace_entry(places_.runtime_file, folder); !saved) {
      std::println(out, "Error: {}", saved.error());
      mode_ = Mode::Main;
      return;
    }
    auto const runtime_root = places_.runtime_file.parent_path().parent_path();
    std::println(out, "Updated .cratchit/runtime.txt in {}", runtime_root.string());
  }
  std::println(out, "Opened workspace {}", folder.string());
  mode_ = Mode::Workspace;
}

void run_repl(Repl& repl, KeySource const& next_key, std::ostream& out) {
  while (true) {
    bool const typing = repl.is_typing();
    if (!typing) {
      std::print(out, "{}", repl.prompt());
    }
    out.flush();
    std::optional<char> key;
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
