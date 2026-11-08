#include "core/project.hpp"

#include "core/path_value.hpp"

#include <algorithm>
#include <format>
#include <system_error>
#include <tuple>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

fs::path project_file_of(fs::path const& folder) { return folder / ".cratchit" / "project.txt"; }

} // namespace

std::expected<std::vector<ListedProject>, std::string> list_projects(fs::path const& workspace_folder) {
  std::vector<ListedProject> result;
  std::error_code ec;
  fs::directory_iterator entries(workspace_folder, ec);
  if (ec) return std::unexpected(std::format("Kan inte läsa {}: {}", workspace_folder.string(), ec.message()));
  for (auto const& entry : entries) {
    std::error_code entry_ec;
    if (!entry.is_directory(entry_ec)) continue;
    auto const folder = entry.path();
    if (!fs::is_regular_file(project_file_of(folder), entry_ec)) continue;
    auto caption = folder.filename().string();
    if (auto const opened = open_project(folder)) caption = opened->name;
    result.push_back({folder, std::move(caption)});
  }
  std::ranges::sort(result, {}, [](ListedProject const& p) { return std::tie(p.caption, p.folder); });
  return result;
}

std::expected<Project, std::string> open_project(fs::path const& folder) {
  Project result;
  result.folder = folder;
  result.file = project_file_of(folder);
  std::error_code ec;
  if (!fs::is_regular_file(result.file, ec)) {
    return std::unexpected(std::format("{} saknas", result.file.string()));
  }
  auto const entries = read_path_values(result.file);
  if (!entries) return std::unexpected(entries.error());
  auto name = find_value(*entries, "name");
  if (!name || name->empty()) return std::unexpected(std::format("{}: 'name' saknas", result.file.string()));
  result.name = std::move(*name);
  return result;
}

} // namespace cratchit
