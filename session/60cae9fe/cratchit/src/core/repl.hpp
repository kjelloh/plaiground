#pragma once

#include "core/help.hpp"
#include "core/manual.hpp"
#include "core/path_editor.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <ostream>
#include <string>

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
// choosing a workspace, or in the workspace
class Repl {
public:
  // page_lines is the number of help lines per page; 0 shows everything at once
  Repl(Section const& manual, std::size_t page_lines, WorkspacePlaces places);

  // Open the workspace named in runtime.txt, or offer folders to choose from
  // when there is none or its folder is missing (key w, and on start)
  void open_workspace(std::ostream& out);

  // Handle one key press, writing any response to out
  Step on_key(char key, std::ostream& out);

  // True for keys that do nothing in the current mode, so are not even echoed
  bool ignores(char key) const;

  // True while a folder path is typed; on_key then writes the prompt and echo itself
  bool is_typing() const;

  std::string prompt() const;

private:
  enum class Mode { Main, Help, Choose, Typing, ConfirmCreate, Workspace };

  WorkspacePlaces places_;
  HelpBrowser help_;
  PathEditor editor_;
  Mode mode_ = Mode::Main;
  Mode before_help_ = Mode::Main;
  std::filesystem::path to_create_;

  void open_help(std::ostream& out);
  void show_choices(std::ostream& out);
  void start_typing(std::ostream& out);
  void on_choose_key(char key, std::ostream& out);
  void on_typing_key(char key, std::ostream& out);
  void on_confirm_key(char key, std::ostream& out);
  void create_and_choose(std::filesystem::path const& folder, std::ostream& out);
  void enter_workspace(std::filesystem::path const& folder, bool chosen, std::ostream& out);
};

// Supplies the next key press, or nothing at end of input
using KeySource = std::function<std::optional<char>()>;

// Prompt, read a key, handle it, until quit or end of input
void run_repl(Repl& repl, KeySource const& next_key, std::ostream& out);

} // namespace cratchit
