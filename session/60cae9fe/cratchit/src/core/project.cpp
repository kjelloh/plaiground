#include "core/project.hpp"

#include "core/path_value.hpp"

#include <algorithm>
#include <format>
#include <random>
#include <ranges>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

constexpr std::string_view list_prefix = "project.";

fs::path project_file_name() {
  return fs::path(".cratchit") / "project.txt";
}

} // namespace

std::string format_uuid(std::uint64_t high, std::uint64_t low) {
  high = (high & ~std::uint64_t{0xF000}) | 0x4000;                 // version 4
  low = (low & ~(std::uint64_t{0xC} << 60)) | std::uint64_t{0x8} << 60; // variant 10xx
  return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}", high >> 32, (high >> 16) & 0xFFFF, high & 0xFFFF,
                     low >> 48, low & 0xFFFF'FFFF'FFFF);
}

std::string new_uuid() {
  std::random_device device;
  std::seed_seq seed{device(), device(), device(), device(), device(), device(), device(), device()};
  std::mt19937_64 engine(seed);
  auto const high = engine();
  return format_uuid(high, engine());
}

bool is_uuid(std::string_view text) {
  if (text.size() != 36) {
    return false;
  }
  for (std::size_t i = 0; i < text.size(); ++i) {
    auto const c = text[i];
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (c != '-') {
        return false;
      }
    } else if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
      return false;
    }
  }
  return true;
}

std::expected<std::vector<ListedProject>, std::string> listed_projects(fs::path const& workspace_file) {
  auto const entries = read_path_values(workspace_file);
  if (!entries) {
    return std::unexpected(entries.error());
  }
  auto const workspace_folder = workspace_file.parent_path().parent_path();
  std::vector<ListedProject> result;
  for (auto const& entry : *entries) {
    if (!entry.path.starts_with(list_prefix)) {
      continue;
    }
    auto const uuid = entry.path.substr(list_prefix.size());
    if (!is_uuid(uuid)) {
      return std::unexpected(std::format("{}: '{}' does not name a project uuid", workspace_file.string(), entry.path));
    }
    // An absolute folder replaces the workspace folder
    result.push_back({uuid, (workspace_folder / entry.value).lexically_normal()});
  }
  return result;
}

std::expected<std::optional<std::string>, std::string> last_project(fs::path const& workspace_file) {
  auto const entries = read_path_values(workspace_file);
  if (!entries) {
    return std::unexpected(entries.error());
  }
  auto const entry = find_value(*entries, "project");
  if (!entry || entry->empty()) {
    return std::optional<std::string>{};
  }
  return std::optional<std::string>{*entry};
}

std::expected<bool, std::string> remember_project(fs::path const& workspace_file, ListedProject const& project) {
  auto entries = read_path_values(workspace_file);
  if (!entries) {
    return std::unexpected(entries.error());
  }
  auto const before = *entries;
  auto const list_path = std::string(list_prefix) + project.uuid;
  if (!find_value(*entries, list_path)) {
    // After the last listed project, so the list stays together
    auto const last_listed = std::ranges::find_if(std::views::reverse(*entries), [](PathValue const& entry) {
      return entry.path.starts_with(list_prefix);
    });
    auto const at = last_listed == std::views::reverse(*entries).end() ? entries->end() : last_listed.base();
    entries->insert(at, {list_path, project.folder.string()});
  }
  set_value(*entries, "project", project.uuid);
  if (std::ranges::equal(*entries, before, [](PathValue const& a, PathValue const& b) {
        return a.path == b.path && a.value == b.value;
      })) {
    return false;
  }
  if (auto written = write_path_values(workspace_file, *entries); !written) {
    return std::unexpected(written.error());
  }
  return true;
}

std::expected<Project, std::string> read_project(fs::path const& folder) {
  auto const file = folder / project_file_name();
  std::error_code ec;
  if (!fs::is_directory(folder, ec)) {
    return std::unexpected(std::format("Project folder {} does not exist", folder.string()));
  }
  if (!fs::is_regular_file(file, ec)) {
    return std::unexpected(std::format("{} has no project.txt file", (folder / ".cratchit").string()));
  }
  auto const entries = read_path_values(file);
  if (!entries) {
    return std::unexpected(entries.error());
  }
  auto const uuid = find_value(*entries, "uuid");
  if (!uuid || !is_uuid(*uuid)) {
    return std::unexpected(std::format("{}: no valid 'uuid = <uuid>'", file.string()));
  }
  auto const name = find_value(*entries, "name");
  if (!name || name->empty()) {
    return std::unexpected(std::format("{}: no 'name = <name>'", file.string()));
  }
  return Project{*uuid, *name, folder, {}, find_value(*entries, "orgnr").value_or(""),
                 find_value(*entries, "year").value_or("")};
}

std::expected<bool, std::string> save_project_identity(fs::path const& folder, std::string_view name,
                                                       std::string_view orgnr) {
  auto const file = folder / project_file_name();
  auto entries = read_path_values(file);
  if (!entries) {
    return std::unexpected(entries.error());
  }
  auto const before = *entries;
  set_value(*entries, "name", name);
  set_value(*entries, "orgnr", orgnr);
  if (std::ranges::equal(*entries, before, [](PathValue const& a, PathValue const& b) {
        return a.path == b.path && a.value == b.value;
      })) {
    return false;
  }
  if (auto written = write_path_values(file, *entries); !written) {
    return std::unexpected(written.error());
  }
  return true;
}

std::expected<bool, std::string> save_project_year(fs::path const& folder, std::string_view start) {
  auto const file = folder / project_file_name();
  auto entries = read_path_values(file);
  if (!entries) {
    return std::unexpected(entries.error());
  }
  if (find_value(*entries, "year") == start) {
    return false;
  }
  set_value(*entries, "year", start);
  if (auto written = write_path_values(file, *entries); !written) {
    return std::unexpected(written.error());
  }
  return true;
}

std::expected<Project, std::string> open_listed_project(ListedProject const& listed) {
  auto project = read_project(listed.folder);
  if (project && project->uuid != listed.uuid) {
    return std::unexpected(std::format("{} holds project {}, not the listed {}",
                                       (listed.folder / project_file_name()).string(), project->uuid, listed.uuid));
  }
  return project;
}

std::expected<Project, std::string> create_project(fs::path const& folder, std::string_view name,
                                                   std::string_view uuid, std::string_view today) {
  auto const dot_folder = folder / ".cratchit";
  auto const file = folder / project_file_name();
  std::error_code ec;
  if (!fs::is_directory(folder, ec)) {
    return std::unexpected(std::format("Project folder {} does not exist", folder.string()));
  }
  if (fs::exists(dot_folder, ec)) {
    if (!fs::is_directory(dot_folder, ec)) {
      return std::unexpected(std::format("{} exists but is not a folder", dot_folder.string()));
    }
    if (fs::exists(file, ec)) {
      return std::unexpected(std::format("{} already holds a project.txt file", dot_folder.string()));
    }
  } else if (!fs::create_directory(dot_folder, ec)) {
    return std::unexpected(std::format("Cannot create {}: {}", dot_folder.string(), ec.message()));
  }
  PathValues const entries{{"created", std::string(today)}, {"uuid", std::string(uuid)}, {"name", std::string(name)}};
  if (auto written = write_path_values(file, entries); !written) {
    return std::unexpected(written.error());
  }
  return Project{std::string(uuid), std::string(name), folder, {{folder, project_file_name()}}, {}, {}};
}

std::optional<fs::path> default_project_folder(fs::path const& workspace_folder, std::string_view name) {
  // Not allowed in a Windows file name, or a separator somewhere
  constexpr std::string_view not_allowed = "/\\:*?\"<>|";
  if (name.empty() || name == "." || name == ".." || name.find_first_of(not_allowed) != std::string_view::npos ||
      std::ranges::any_of(name, [](char c) { return static_cast<unsigned char>(c) < 0x20; }) || name.back() == '.' ||
      name.back() == ' ') {
    return std::nullopt;
  }
  return workspace_folder / fs::path(std::u8string(name.begin(), name.end()));
}

} // namespace cratchit
