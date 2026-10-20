#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// The .cratchit folder as found or initiated by open_data_folder
struct DataFolder {
  std::filesystem::path framework_file;
  std::vector<std::filesystem::path> initiated; // new files, relative to the root
};

// Opens <root>/.cratchit. When it does not exist, creates it with
// framework.txt holding `created = <today>`. An existing .cratchit is never
// changed, and is an error when it lacks required content (framework.txt).
std::expected<DataFolder, std::string> open_data_folder(std::filesystem::path const& root, std::string_view today);

// Today's local date as YYYY-MM-DD
std::string today_iso();

} // namespace cratchit
