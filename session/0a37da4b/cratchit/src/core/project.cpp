#include "core/project.hpp"

#include "core/path_value.hpp"

#include <algorithm>
#include <format>
#include <system_error>
#include <tuple>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

// The key prefix of a remembered project in workspace.txt
constexpr std::string_view project_prefix = "project.";

} // namespace

std::expected<std::vector<ListedProject>, std::string> list_projects(Workspace const& workspace) {
  auto const entries = read_path_values(workspace.file);
  if (!entries) return std::unexpected(entries.error());
  std::vector<ListedProject> result;
  for (auto const& entry : *entries) {
    if (!entry.path.starts_with(project_prefix)) continue;
    auto uuid = entry.path.substr(project_prefix.size());
    if (uuid.empty()) return std::unexpected(std::format("{}: '{}' saknar uuid", workspace.file.string(), entry.path));
    if (entry.value.empty()) {
      return std::unexpected(std::format("{}: '{}' saknar mapp", workspace.file.string(), entry.path));
    }
    fs::path folder(entry.value);
    if (folder.is_relative()) folder = workspace.folder / folder;
    auto caption = folder.filename().string();
    if (auto const opened = open_project(folder, uuid)) caption = opened->name;
    result.push_back({std::move(uuid), std::move(folder), std::move(caption)});
  }
  std::ranges::sort(result, {}, [](ListedProject const& p) { return std::tie(p.caption, p.folder); });
  return result;
}

std::expected<Project, std::string> open_project(fs::path const& folder, std::string_view uuid) {
  Project result;
  result.folder = folder;
  result.file = folder / ".cratchit" / "project.txt";
  std::error_code ec;
  if (!fs::is_regular_file(result.file, ec)) {
    return std::unexpected(std::format("{} saknas", result.file.string()));
  }
  auto const entries = read_path_values(result.file);
  if (!entries) return std::unexpected(entries.error());
  auto found_uuid = find_value(*entries, "uuid");
  if (!found_uuid || found_uuid->empty()) return std::unexpected(std::format("{}: 'uuid' saknas", result.file.string()));
  if (*found_uuid != uuid) {
    return std::unexpected(
        std::format("{}: uuid {} är inte {} som samlingsmappen har för mappen", result.file.string(), *found_uuid, uuid));
  }
  auto name = find_value(*entries, "name");
  if (!name || name->empty()) return std::unexpected(std::format("{}: 'name' saknas", result.file.string()));
  result.uuid = std::move(*found_uuid);
  result.name = std::move(*name);
  return result;
}

} // namespace cratchit
