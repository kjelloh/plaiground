#pragma once

#include "core/data_folder.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// The workspace as found or initiated by open_workspace
struct Workspace {
  std::filesystem::path folder;
  std::filesystem::path workspace_file;
  std::vector<Initiated> initiated;
};

// The folder named by `workspace = <folder>` in runtime_file; nothing when the
// entry is missing or empty. A relative folder is taken from the folder holding
// the runtime's .cratchit. Error text when runtime_file cannot be read.
std::expected<std::optional<std::filesystem::path>, std::string> workspace_entry(std::filesystem::path const& runtime_file);

// Set `workspace = <folder>` in runtime_file, keeping its other entries and their order
std::expected<void, std::string> save_workspace_entry(std::filesystem::path const& runtime_file,
                                                      std::filesystem::path const& folder);

// Opens <folder>/.cratchit/workspace.txt; folder must exist.
//
// A missing <folder>/.cratchit is a fresh location: it is created with
// workspace.txt (`created = <today>`). An existing .cratchit without
// workspace.txt is an error, unless add_missing_file (the user has just chosen
// the folder), when workspace.txt is added. Error text otherwise.
std::expected<Workspace, std::string> open_workspace(std::filesystem::path const& folder, std::string_view today,
                                                     bool add_missing_file);

// <the user's Documents folder>/cratchit, or nothing if that folder cannot be found.
// macOS: $HOME/Documents. Linux: XDG_DOCUMENTS_DIR from user-dirs.dirs, else
// $HOME/Documents. Windows: the Documents known folder.
std::optional<std::filesystem::path> default_workspace_folder();

// The Documents folder named by XDG_DOCUMENTS_DIR in the text of a user-dirs.dirs
// file, with $HOME replaced by home; nothing if it is not given as a full path
std::optional<std::filesystem::path> xdg_documents_folder(std::string_view user_dirs, std::filesystem::path const& home);

// What Tab adds to a typed folder path
struct Completion {
  std::string added;                   // text to append to the typed path
  std::vector<std::string> candidates; // folder names to list when nothing can be added
};

// Completes the last name in typed to existing folders whose names start with
// it, like Tab in a shell. A unique folder is completed with a separator after
// it; several are completed as far as they agree, and listed when they agree on
// nothing more. Hidden folders (.name) count only when the typed name starts
// with a dot. Nothing for a path without a separator.
Completion complete_folder(std::string_view typed);

} // namespace cratchit
