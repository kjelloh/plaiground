#include "core/dot_folder.hpp"

#include "core/path_value.hpp"

#include <cstdint>
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

#ifdef __APPLE__
#include <mach-o/dyld.h>
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

// Where the running program file is, as the platform tells it. Not yet
// resolved: it may be a link.
#if defined(_WIN32)
std::expected<fs::path, std::string> program_file() {
  std::wstring buffer(MAX_PATH, L'\0');
  for (;;) {
    auto const size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size == 0) return std::unexpected(std::string("GetModuleFileNameW misslyckades"));
    if (size < buffer.size()) {
      buffer.resize(size);
      return fs::path(buffer);
    }
    buffer.resize(buffer.size() * 2); // the path did not fit
  }
}
#elif defined(__APPLE__)
std::expected<fs::path, std::string> program_file() {
  std::uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size); // gives the size needed
  std::string buffer(size, '\0');
  if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
    return std::unexpected(std::string("_NSGetExecutablePath misslyckades"));
  }
  buffer.resize(std::char_traits<char>::length(buffer.c_str()));
  return fs::path(buffer);
}
#elif defined(__linux__)
std::expected<fs::path, std::string> program_file() {
  std::error_code ec;
  auto path = fs::read_symlink("/proc/self/exe", ec);
  if (ec) return std::unexpected(std::format("Kan inte läsa /proc/self/exe: {}", ec.message()));
  return path;
}
#else
#error "program_file() is not written for this platform"
#endif

} // namespace

std::expected<fs::path, std::string> program_folder() {
  auto const file = program_file();
  if (!file) return std::unexpected(std::format("Hittar inte programmets mapp: {}", file.error()));
  std::error_code ec;
  auto const resolved = fs::canonical(*file, ec);
  if (ec) return std::unexpected(std::format("Hittar inte programmets mapp: {}: {}", file->string(), ec.message()));
  return resolved.parent_path();
}

std::expected<fs::path, std::string> choose_user_home(std::optional<fs::path> const& user_home) {
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

std::expected<fs::path, std::string> user_home() { return choose_user_home(environment_user_home()); }

std::expected<Framework, std::string> open_framework(fs::path const& program_folder) {
  Framework result;
  result.file = program_folder / ".cratchit" / "framework.txt";
  std::error_code ec;
  if (!fs::is_regular_file(result.file, ec)) {
    return std::unexpected(std::format("{} saknas. Installationen av cratchit är inte hel.", result.file.string()));
  }
  auto const entries = read_path_values(result.file);
  if (!entries) return std::unexpected(entries.error());
  auto version = find_value(*entries, "version");
  if (!version || version->empty()) {
    return std::unexpected(std::format("{}: 'version' saknas", result.file.string()));
  }
  result.version = std::move(*version);
  return result;
}

std::expected<Runtime, std::string> open_runtime(fs::path const& home, std::string_view today) {
  Runtime result;
  auto const folder = home / ".cratchit";
  auto const runtime_name = fs::path(".cratchit") / "runtime.txt";
  result.file = home / runtime_name;

  std::error_code ec;
  if (fs::exists(folder, ec)) {
    if (!fs::is_directory(folder, ec)) {
      return std::unexpected(std::format("{} finns men är inte en mapp", folder.string()));
    }
  } else if (!fs::create_directory(folder, ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", folder.string(), ec.message()));
  }

  if (fs::is_regular_file(result.file, ec)) return result;
  if (fs::exists(result.file, ec)) {
    return std::unexpected(std::format("{} finns men är inte en fil", result.file.string()));
  }
  if (auto written = write_new_file(result.file, std::format("created = {}\n", today)); !written) {
    return std::unexpected(written.error());
  }
  result.initiated.push_back({home, runtime_name});
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
