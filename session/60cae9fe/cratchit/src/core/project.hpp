#pragma once

#include "core/data_folder.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// The bookkeeping of one juridical person (private person, company or organisation),
// kept in a folder whose .cratchit holds project.txt
struct Project {
  std::string uuid;
  std::string name; // the official name of the person, company or organisation
  std::filesystem::path folder;
  std::vector<Initiated> initiated;
  std::string orgnr; // the personal or organisation number; empty until set (by the first import)
  std::string year;  // the start (YYYY-MM-DD) of the fiscal year last opened, from `year`; empty if none
};

// A project listed in workspace.txt as `project.<uuid> = <folder>`
struct ListedProject {
  std::string uuid;
  std::filesystem::path folder;
};

// A version 4 (random) uuid from 128 bits, in lowercase 8-4-4-4-12 form.
// The version and variant bits are set over the given ones.
std::string format_uuid(std::uint64_t high, std::uint64_t low);

// A new random version 4 uuid
std::string new_uuid();

// True for lowercase 8-4-4-4-12 hex digits
bool is_uuid(std::string_view text);

// The projects listed in workspace_file, in file order. A relative folder is
// taken from the folder holding the workspace's .cratchit. Error text when the
// file cannot be read or a `project.<x>` path does not name a uuid.
std::expected<std::vector<ListedProject>, std::string> listed_projects(std::filesystem::path const& workspace_file);

// The uuid named by `project = <uuid>` in workspace_file (the last opened project);
// nothing when the entry is missing or empty
std::expected<std::optional<std::string>, std::string> last_project(std::filesystem::path const& workspace_file);

// Lists the project in workspace_file when not yet listed (after the last listed
// one, else last), and sets it as the last opened, keeping the other entries and
// their order. The file is written only when it changes; true if it did.
std::expected<bool, std::string> remember_project(std::filesystem::path const& workspace_file,
                                                  ListedProject const& project);

// Reads <folder>/.cratchit/project.txt, which must hold a uuid and a name
std::expected<Project, std::string> read_project(std::filesystem::path const& folder);

// Sets the name and organisation number in <folder>/.cratchit/project.txt,
// keeping the other entries. The file is written only when it changes; true if it did.
std::expected<bool, std::string> save_project_identity(std::filesystem::path const& folder, std::string_view name,
                                                       std::string_view orgnr);

// Sets `year = <start>` (the fiscal year last opened) in <folder>/.cratchit/project.txt,
// keeping the other entries. The file is written only when it changes; true if it did.
std::expected<bool, std::string> save_project_year(std::filesystem::path const& folder, std::string_view start);

// Opens a listed project: its folder must exist and its project.txt hold the listed uuid
std::expected<Project, std::string> open_listed_project(ListedProject const& listed);

// Creates <folder>/.cratchit/project.txt (`created`, `uuid`, `name`) in an existing
// folder, creating .cratchit when missing. Error text when the folder already
// holds a project.txt.
std::expected<Project, std::string> create_project(std::filesystem::path const& folder, std::string_view name,
                                                   std::string_view uuid, std::string_view today);

// <workspace folder>/<name>, or nothing when name cannot be a folder name as it is
// (empty, `.` or `..`, or holding a character some platform does not allow)
std::optional<std::filesystem::path> default_project_folder(std::filesystem::path const& workspace_folder,
                                                            std::string_view name);

} // namespace cratchit
