#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// A file open_runtime created
struct Initiated {
  std::filesystem::path folder; // the folder holding .cratchit
  std::filesystem::path file;   // relative to folder, e.g. .cratchit/runtime.txt
};

// The framework that comes with the installation. Read only: cratchit never
// writes it. Later it also holds the rules for Swedish bookkeeping.
struct Framework {
  std::filesystem::path file; // <program folder>/.cratchit/framework.txt
  std::string version;        // the framework's `version` entry
};

// The user's own cratchit files, in the user's home folder
struct Runtime {
  std::filesystem::path file; // <home>/.cratchit/runtime.txt
  std::vector<Initiated> initiated;
};

// The folder that holds the running program, with links and `..` resolved.
// Asks the platform (macOS _NSGetExecutablePath, Linux /proc/self/exe, Windows
// GetModuleFileNameW), since argv[0] is only what the starter passed, e.g. the
// bare name when started from PATH. Error text when it cannot be found.
std::expected<std::filesystem::path, std::string> program_folder();

// Opens <program_folder>/.cratchit/framework.txt and reads its version. Never
// creates or changes anything. Error text when the file is missing (the
// installation is not whole), malformed or has no `version`.
std::expected<Framework, std::string> open_framework(std::filesystem::path const& program_folder);

// Opens <home>/.cratchit/runtime.txt. A missing .cratchit or runtime.txt is a
// fresh start: created, with runtime.txt holding `created = <today>`. Existing
// files are never changed. Error text otherwise.
std::expected<Runtime, std::string> open_runtime(std::filesystem::path const& home, std::string_view today);

// The user's home folder. It must be a full path to an existing folder. Error
// text otherwise, or when it is not given or empty.
std::expected<std::filesystem::path, std::string> choose_user_home(std::optional<std::filesystem::path> const& user_home);

// choose_user_home from the environment: USERPROFILE on Windows, HOME elsewhere
std::expected<std::filesystem::path, std::string> user_home();

// Today's local date as YYYY-MM-DD
std::string today_iso();

} // namespace cratchit
