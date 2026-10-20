#include "core/data_folder.hpp"

#include <ctime>
#include <format>
#include <fstream>

namespace cratchit {

std::expected<DataFolder, std::string> open_data_folder(std::filesystem::path const& root, std::string_view today) {
  namespace fs = std::filesystem;
  DataFolder result;
  auto const folder = root / ".cratchit";
  result.framework_file = folder / "framework.txt";

  std::error_code ec;
  if (fs::exists(folder, ec)) {
    if (!fs::is_directory(folder, ec)) {
      return std::unexpected(std::format("{} exists but is not a folder", folder.string()));
    }
    if (!fs::is_regular_file(result.framework_file, ec)) {
      return std::unexpected(std::format("{} has no framework.txt file", folder.string()));
    }
    return result;
  }

  if (!fs::create_directory(folder, ec)) {
    return std::unexpected(std::format("Cannot create {}: {}", folder.string(), ec.message()));
  }
  std::ofstream out(result.framework_file);
  out << "created = " << today << '\n';
  if (!out) {
    return std::unexpected(std::format("Cannot write {}", result.framework_file.string()));
  }
  result.initiated.push_back(fs::path(".cratchit") / "framework.txt");
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
