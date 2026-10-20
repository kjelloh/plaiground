#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// A file open_data_folder created
struct Initiated {
  std::filesystem::path folder; // the folder holding .cratchit
  std::filesystem::path file;   // relative to folder, e.g. .cratchit/framework.txt
};

// The cratchit files as found or initiated by open_data_folder
struct DataFolder {
  std::filesystem::path framework_file;
  std::filesystem::path runtime_file;
  std::vector<Initiated> initiated;
};

// Opens <root>/.cratchit/framework.txt and the runtime it refers to.
//
// framework.txt entry `runtime = <folder>` names a folder whose .cratchit holds
// runtime.txt. A relative folder is taken from root; `.` (the default when the
// entry is missing) is root itself.
//
// A missing <root>/.cratchit is a fresh start: it is created with framework.txt
// (`created = <today>`, `runtime = .`) and runtime.txt (`created = <today>`).
// Existing files are never changed. runtime.txt is added only to root's own
// .cratchit; a runtime elsewhere must already exist. Error text otherwise.
std::expected<DataFolder, std::string> open_data_folder(std::filesystem::path const& root, std::string_view today);

// Today's local date as YYYY-MM-DD
std::string today_iso();

} // namespace cratchit
