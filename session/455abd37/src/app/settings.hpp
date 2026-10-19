#pragma once

// Company settings (interface settings). Schema and defaults: specs/settings/settings.cspec.
// The user's values: .cratchit/settings.cspec

#include <expected>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace cratchit {

class Settings {
public:
  static std::expected<Settings, std::string> load(std::filesystem::path const& specs_dir,
                                                   std::optional<std::filesystem::path> const& user_file = std::nullopt);

  // The value of a setting listed in the schema, e.g. get("vat.settlement_series").
  std::string get(std::string_view name) const;

  std::map<std::string, std::string, std::less<>> const& values() const { return m_values; }

private:
  std::map<std::string, std::string, std::less<>> m_values;
};

} // namespace cratchit
