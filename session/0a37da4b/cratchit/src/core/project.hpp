#pragma once

#include "core/dot_folder.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace cratchit {

// A project: the books of one juridical person, in a folder of its own that
// may be anywhere. Marked by <folder>/.cratchit/project.txt, in the format of
// take 3 (`created`, `uuid`, `name`, `orgnr`, ...). Only `uuid` and `name` are
// read for now; the other entries are left as they are.
struct Project {
  std::filesystem::path folder; // the project folder
  std::filesystem::path file;   // <folder>/.cratchit/project.txt
  std::string uuid;             // as in project.txt
  std::string name;             // the official name, e.g. "The ITfied AB"
};

// A project as the workspace remembers it, before it is opened
struct ListedProject {
  std::string uuid;
  std::filesystem::path folder; // full path, resolved from the workspace folder when relative
  std::string caption;          // `name` from project.txt, or the folder name when the project cannot be opened
};

// The projects the workspace remembers: the `project.<uuid> = <folder>` entries
// of its workspace.txt (as in take 3). A relative folder is read from the
// workspace folder. Ordered by caption (then folder). The folders are not
// searched. Error text when workspace.txt cannot be read, or an entry has no
// uuid or no folder.
std::expected<std::vector<ListedProject>, std::string> list_projects(Workspace const& workspace);

// Opens the project in folder: reads .cratchit/project.txt, which must hold a
// `name` and the `uuid` the workspace lists it with. Never creates or changes
// anything. Error text otherwise.
std::expected<Project, std::string> open_project(std::filesystem::path const& folder, std::string_view uuid);

// Opens the project in folder, whatever its uuid: as open_project, for a
// project the workspace does not list yet. folder must be a full path; it is
// normalised (no `.`, `..` or separator at the end). Error text otherwise.
std::expected<Project, std::string> open_project(std::filesystem::path const& folder);

// Makes the workspace remember the project: sets `project.<uuid> = <folder>`
// in workspace.txt (added last, or replacing the folder it had), keeping the
// other entries as they are. Error text when workspace.txt cannot be read or written.
std::expected<void, std::string> remember_project(Workspace const& workspace, Project const& project);

} // namespace cratchit
