#include "core/dot_folder.hpp"

#include "core/path_value.hpp"

#include <ctime>
#include <format>
#include <fstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cstdlib>
#endif

namespace cratchit {

namespace fs = std::filesystem;

namespace {

std::expected<void, std::string> write_new_file(fs::path const& file, std::string_view text) {
  std::ofstream out(file);
  out << text;
  if (!out) {
    return std::unexpected(std::format("Kan inte skriva {}", file.string()));
  }
  return {};
}

#ifdef _WIN32
constexpr std::string_view user_home_variable = "USERPROFILE";

// Wide, so that a folder name is read correctly whatever the ANSI code page holds
std::optional<fs::path> environment_user_home() {
  auto const size = GetEnvironmentVariableW(L"USERPROFILE", nullptr, 0);
  if (size == 0) {
    return std::nullopt;
  }
  std::wstring value(size, L'\0');
  value.resize(GetEnvironmentVariableW(L"USERPROFILE", value.data(), size));
  return fs::path(value);
}
#else
constexpr std::string_view user_home_variable = "HOME";

std::optional<fs::path> environment_user_home() {
  auto const* value = std::getenv("HOME");
  if (value == nullptr) {
    return std::nullopt;
  }
  return fs::path(value);
}
#endif

} // namespace

std::expected<fs::path, std::string> choose_dot_folder_root(std::optional<fs::path> const& user_home) {
  if (!user_home || user_home->empty()) {
    return std::unexpected(std::format("Hittar inte hemkatalogen. {} är inte satt.", user_home_variable));
  }
  if (!user_home->is_absolute()) {
    return std::unexpected(std::format("{} måste vara en hel sökväg: {}", user_home_variable, user_home->string()));
  }
  std::error_code ec;
  if (!fs::is_directory(*user_home, ec)) {
    return std::unexpected(std::format("{} pekar på en mapp som inte finns: {}", user_home_variable, user_home->string()));
  }
  return *user_home;
}

std::expected<fs::path, std::string> dot_folder_root() { return choose_dot_folder_root(environment_user_home()); }

std::expected<DotFolder, std::string> open_dot_folder(fs::path const& root, std::string_view today) {
  DotFolder result;
  auto const folder = root / ".cratchit";
  auto const framework_name = fs::path(".cratchit") / "framework.txt";
  auto const runtime_name = fs::path(".cratchit") / "runtime.txt";
  result.framework_file = root / framework_name;

  std::error_code ec;
  if (fs::exists(folder, ec)) {
    if (!fs::is_directory(folder, ec)) {
      return std::unexpected(std::format("{} finns men är inte en mapp", folder.string()));
    }
    if (!fs::is_regular_file(result.framework_file, ec)) {
      return std::unexpected(std::format("{} saknar filen framework.txt", folder.string()));
    }
  } else {
    if (!fs::create_directory(folder, ec)) {
      return std::unexpected(std::format("Kan inte skapa {}: {}", folder.string(), ec.message()));
    }
    if (auto written = write_new_file(result.framework_file, std::format("created = {}\nruntime = .\n", today)); !written) {
      return std::unexpected(written.error());
    }
    result.initiated.push_back({root, framework_name});
  }

  auto const framework = read_path_values(result.framework_file);
  if (!framework) {
    return std::unexpected(framework.error());
  }
  auto const runtime_entry = find_value(*framework, "runtime").value_or(".");
  if (runtime_entry.empty()) {
    return std::unexpected(std::format("{}: 'runtime' anger ingen mapp", result.framework_file.string()));
  }
  auto const runtime_root = (root / runtime_entry).lexically_normal(); // an absolute entry replaces root
  if (!fs::is_directory(runtime_root, ec)) {
    return std::unexpected(std::format("Runtime-mappen {} finns inte", runtime_root.string()));
  }
  bool const is_self = fs::equivalent(runtime_root, root, ec);
  result.runtime_file = (is_self ? root : runtime_root) / runtime_name;

  if (fs::is_regular_file(result.runtime_file, ec)) {
    return result;
  }
  if (!is_self || fs::exists(result.runtime_file, ec)) {
    return std::unexpected(std::format("Ingen .cratchit/runtime.txt i runtime-mappen {}", runtime_root.string()));
  }
  if (auto written = write_new_file(result.runtime_file, std::format("created = {}\n", today)); !written) {
    return std::unexpected(written.error());
  }
  result.initiated.push_back({root, runtime_name});
  return result;
}

std::string today_iso() {
  std::time_t const now = std::time(nullptr);
  std::tm local{};
#ifdef _WIN32
  localtime_s(&local, &now);
#else
  localtime_r(&now, &local);
#endif
  return std::format("{:04}-{:02}-{:02}", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
}

} // namespace cratchit
