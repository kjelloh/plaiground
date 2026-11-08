#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace cratchit {

// A project: the books of one juridical person, in a folder of its own directly
// in the workspace. Marked by <folder>/.cratchit/project.txt, in the format of
// take 3 (`created`, `uuid`, `name`, `orgnr`, ...). Only `name` is read for
// now; the other entries are left as they are.
struct Project {
  std::filesystem::path folder; // the project folder
  std::filesystem::path file;   // <folder>/.cratchit/project.txt
  std::string name;             // the official name, e.g. "The ITfied AB"
};

// A project as listed in the workspace, before it is opened
struct ListedProject {
  std::filesystem::path folder;
  std::string caption; // `name` from project.txt, or the folder name when it cannot be read
};

// The folders directly in workspace_folder that hold .cratchit/project.txt,
// ordered by caption (then folder). Other folders and files are skipped. A
// project.txt that cannot be read is still listed (by folder name), so that
// opening it shows why. Error text when the folder cannot be read.
std::expected<std::vector<ListedProject>, std::string> list_projects(std::filesystem::path const& workspace_folder);

// Opens the project in folder: reads .cratchit/project.txt, which must hold a
// `name`. Never creates or changes anything. Error text otherwise.
std::expected<Project, std::string> open_project(std::filesystem::path const& folder);

} // namespace cratchit
