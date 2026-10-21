#pragma once

#include "core/charset/unicode.hpp"
#include "core/help.hpp"
#include "core/log_view.hpp"
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
// viewing the log, choosing a workspace, or in the workspace
class Repl {
public:
  // page_lines is the number of help and log lines per page; 0 shows everything at once.
  // log_file is the log the l key shows.
  Repl(Section const& manual, std::size_t page_lines, WorkspacePlaces places, std::filesystem::path log_file = {});

  // Open the workspace named in runtime.txt, or offer folders to choose from
  // when there is none or its folder is missing (key w, and on start)
  void open_workspace(std::ostream& out);

  // Handle one key press, writing any response to out
  Step on_key(charset::unicode::CodePoint key, std::ostream& out);

  // True for keys that do nothing in the current mode, so are not even echoed
  bool ignores(charset::unicode::CodePoint key) const;

  // True while a folder path is typed; on_key then writes the prompt and echo itself
  bool is_typing() const;

  std::string prompt() const;

private:
  enum class Mode { Main, Help, Log, Choose, Typing, ConfirmCreate, Workspace };

  WorkspacePlaces places_;
  std::filesystem::path log_file_;
  HelpBrowser help_;
  LogView log_view_;
  PathEditor editor_;
  Mode mode_ = Mode::Main;
  Mode before_help_ = Mode::Main;
  Mode before_log_ = Mode::Main;
  std::filesystem::path to_create_;

  void open_help(std::ostream& out);
  void open_log(std::ostream& out);
  void show_choices(std::ostream& out);
  void start_typing(std::ostream& out);
  void on_choose_key(charset::unicode::CodePoint key, std::ostream& out);
  void on_typing_key(charset::unicode::CodePoint key, std::ostream& out);
  void on_confirm_key(charset::unicode::CodePoint key, std::ostream& out);
  void create_and_choose(std::filesystem::path const& folder, std::ostream& out);
  void enter_workspace(std::filesystem::path const& folder, bool chosen, std::ostream& out);
};

// Supplies the next key press as a Unicode code point, or nothing at end of input
using KeySource = std::function<std::optional<charset::unicode::CodePoint>()>;

// Prompt, read a key, handle it, until quit or end of input
void run_repl(Repl& repl, KeySource const& next_key, std::ostream& out);

} // namespace cratchit
