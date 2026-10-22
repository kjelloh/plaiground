#include "core/workspace.hpp"

#include "core/path_value.hpp"

#include <algorithm>
#include <format>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>
#else
#include <cstdlib>
#include <fstream>
#include <sstream>
#endif

namespace cratchit {

namespace fs = std::filesystem;

namespace {

#ifdef _WIN32
constexpr std::string_view separators = "\\/";
#else
constexpr std::string_view separators = "/";
#endif

bool same_char(char a, char b) {
#ifdef _WIN32
  // Windows folder names ignore case
  auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; };
  return lower(a) == lower(b);
#else
  return a == b;
#endif
}

bool starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && std::ranges::equal(text.substr(0, prefix.size()), prefix, same_char);
}

} // namespace

std::expected<std::optional<fs::path>, std::string> workspace_entry(fs::path const& runtime_file) {
  auto const runtime = read_path_values(runtime_file);
  if (!runtime) {
    return std::unexpected(runtime.error());
  }
  auto const entry = find_value(*runtime, "workspace");
  if (!entry || entry->empty()) {
    return std::optional<fs::path>{};
  }
  auto const runtime_root = runtime_file.parent_path().parent_path();
  return std::optional<fs::path>{(runtime_root / *entry).lexically_normal()}; // an absolute entry replaces the root
}

std::expected<void, std::string> save_workspace_entry(fs::path const& runtime_file, fs::path const& folder) {
  auto runtime = read_path_values(runtime_file);
  if (!runtime) {
    return std::unexpected(runtime.error());
  }
  set_value(*runtime, "workspace", folder.string());
  return write_path_values(runtime_file, *runtime);
}

std::expected<Workspace, std::string> open_workspace(fs::path const& folder, std::string_view today,
                                                     bool add_missing_file) {
  auto const dot_folder = folder / ".cratchit";
  auto const workspace_name = fs::path(".cratchit") / "workspace.txt";
  Workspace result{folder, folder / workspace_name, {}};

  std::error_code ec;
  if (!fs::is_directory(folder, ec)) {
    return std::unexpected(std::format("Workspace folder {} does not exist", folder.string()));
  }
  if (fs::exists(dot_folder, ec)) {
    if (!fs::is_directory(dot_folder, ec)) {
      return std::unexpected(std::format("{} exists but is not a folder", dot_folder.string()));
    }
    if (fs::is_regular_file(result.workspace_file, ec)) {
      if (auto const entries = read_path_values(result.workspace_file); !entries) {
        return std::unexpected(entries.error());
      }
      return result;
    }
    if (!add_missing_file || fs::exists(result.workspace_file, ec)) {
      return std::unexpected(std::format("{} has no workspace.txt file", dot_folder.string()));
    }
  } else if (!fs::create_directory(dot_folder, ec)) {
    return std::unexpected(std::format("Cannot create {}: {}", dot_folder.string(), ec.message()));
  }
  if (auto written = write_path_values(result.workspace_file, {{"created", std::string(today)}}); !written) {
    return std::unexpected(written.error());
  }
  result.initiated.push_back({folder, workspace_name});
  return result;
}

std::optional<fs::path> xdg_documents_folder(std::string_view user_dirs, fs::path const& home) {
  constexpr std::string_view key = "XDG_DOCUMENTS_DIR=";
  while (!user_dirs.empty()) {
    auto const end = user_dirs.find('\n');
    auto line = user_dirs.substr(0, end);
    user_dirs = end == std::string_view::npos ? std::string_view{} : user_dirs.substr(end + 1);
    if (!line.starts_with(key)) {
      continue;
    }
    line.remove_prefix(key.size());
    if (line.size() < 2 || line.front() != '"' || line.find('"', 1) == std::string_view::npos) {
      return std::nullopt;
    }
    auto value = line.substr(1, line.find('"', 1) - 1);
    if (value == "$HOME") {
      return home;
    }
    if (value.starts_with("$HOME/")) {
      return home / value.substr(6);
    }
    if (value.starts_with('/')) {
      return fs::path(value);
    }
    return std::nullopt;
  }
  return std::nullopt;
}

std::optional<fs::path> default_workspace_folder() {
#ifdef _WIN32
  PWSTR documents = nullptr;
  auto const found = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &documents) == S_OK;
  std::optional<fs::path> result;
  if (found) {
    result = fs::path(documents) / "cratchit";
  }
  CoTaskMemFree(documents);
  return result;
#else
  char const* home = std::getenv("HOME");
  if (home == nullptr || *home == '\0') {
    return std::nullopt;
  }
  auto documents = fs::path(home) / "Documents";
#ifndef __APPLE__
  char const* config = std::getenv("XDG_CONFIG_HOME");
  auto const config_folder = config != nullptr && *config != '\0' ? fs::path(config) : fs::path(home) / ".config";
  std::ifstream in(config_folder / "user-dirs.dirs");
  std::ostringstream text;
  text << in.rdbuf();
  if (auto const xdg = xdg_documents_folder(text.str(), home)) {
    documents = *xdg;
  }
#endif
  return documents / "cratchit";
#endif
}

Completion complete_folder(std::string_view typed, bool files) {
  auto const cut = typed.find_last_of(separators);
  if (cut == std::string_view::npos) {
    return {};
  }
  auto const folder = fs::path(std::string(typed.substr(0, cut + 1)));
  auto const prefix = typed.substr(cut + 1);

  std::vector<std::string> names;
  std::vector<std::string> folders; // the names that are folders
  std::error_code ec;
  for (fs::directory_iterator it(folder, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end;
       it.increment(ec)) {
    std::error_code type_ec;
    bool const is_folder = it->is_directory(type_ec);
    if (!is_folder && !(files && it->is_regular_file(type_ec))) {
      continue;
    }
    auto name = it->path().filename().string();
    if (name.starts_with('.') && !prefix.starts_with('.')) {
      continue;
    }
    if (starts_with(name, prefix)) {
      if (is_folder) {
        folders.push_back(name);
      }
      names.push_back(std::move(name));
    }
  }
  if (names.empty()) {
    return {};
  }
  std::ranges::sort(names);
  if (names.size() == 1) {
    auto const is_folder = std::ranges::find(folders, names.front()) != folders.end();
    return {names.front().substr(prefix.size()) + (is_folder ? std::string(1, separators.front()) : ""), {}};
  }
  auto agreed = names.front().size();
  for (auto const& name : names) {
    auto const differs = std::ranges::mismatch(names.front(), name, same_char).in1;
    agreed = std::min(agreed, static_cast<std::size_t>(differs - names.front().begin()));
  }
  if (agreed > prefix.size()) {
    return {names.front().substr(prefix.size(), agreed - prefix.size()), {}};
  }
  return {"", std::move(names)};
}

} // namespace cratchit
