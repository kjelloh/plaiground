#include "core/data_folder.hpp"

#include "core/path_value.hpp"

#include <ctime>
#include <format>
#include <fstream>
#include <sstream>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

std::expected<void, std::string> write_new_file(fs::path const& file, std::string_view text) {
  std::ofstream out(file);
  out << text;
  if (!out) {
    return std::unexpected(std::format("Cannot write {}", file.string()));
  }
  return {};
}

std::expected<PathValues, std::string> read_path_values(fs::path const& file) {
  std::ifstream in(file);
  std::ostringstream text;
  text << in.rdbuf();
  if (!in) {
    return std::unexpected(std::format("Cannot read {}", file.string()));
  }
  auto entries = parse_path_values(text.str());
  if (!entries) {
    return std::unexpected(std::format("{}: {}", file.string(), entries.error()));
  }
  return entries;
}

} // namespace

std::expected<DataFolder, std::string> open_data_folder(fs::path const& root, std::string_view today) {
  DataFolder result;
  auto const folder = root / ".cratchit";
  auto const framework_name = fs::path(".cratchit") / "framework.txt";
  auto const runtime_name = fs::path(".cratchit") / "runtime.txt";
  result.framework_file = root / framework_name;

  std::error_code ec;
  if (fs::exists(folder, ec)) {
    if (!fs::is_directory(folder, ec)) {
      return std::unexpected(std::format("{} exists but is not a folder", folder.string()));
    }
    if (!fs::is_regular_file(result.framework_file, ec)) {
      return std::unexpected(std::format("{} has no framework.txt file", folder.string()));
    }
  } else {
    if (!fs::create_directory(folder, ec)) {
      return std::unexpected(std::format("Cannot create {}: {}", folder.string(), ec.message()));
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
    return std::unexpected(std::format("{}: 'runtime' has no folder", result.framework_file.string()));
  }
  auto const runtime_root = (root / runtime_entry).lexically_normal(); // an absolute entry replaces root
  if (!fs::is_directory(runtime_root, ec)) {
    return std::unexpected(std::format("Runtime folder {} does not exist", runtime_root.string()));
  }
  bool const is_self = fs::equivalent(runtime_root, root, ec);
  result.runtime_file = (is_self ? root : runtime_root) / runtime_name;

  if (fs::is_regular_file(result.runtime_file, ec)) {
    return result;
  }
  if (!is_self || fs::exists(result.runtime_file, ec)) {
    return std::unexpected(std::format("No .cratchit/runtime.txt in runtime folder {}", runtime_root.string()));
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
