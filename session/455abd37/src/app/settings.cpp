#include "app/settings.hpp"

#include "core/text.hpp"
#include "cspec/spec_tree.hpp"

#include <stdexcept>

namespace cratchit {

namespace {

constexpr std::string_view setting_prefix = "setting.";
constexpr std::string_view default_suffix = ".default";

} // namespace

std::expected<Settings, std::string> Settings::load(std::filesystem::path const& specs_dir,
                                                    std::optional<std::filesystem::path> const& user_file) {
  auto const schema = cspec::SpecTree::load(specs_dir / "settings" / "settings.cspec");
  if (!schema) return std::unexpected(schema.error());

  Settings settings;
  for (auto const& entry : schema->entries()) {
    std::string_view path = entry.m_path;
    if (!path.starts_with(setting_prefix) || !path.ends_with(default_suffix)) continue;
    path.remove_prefix(setting_prefix.size());
    path.remove_suffix(default_suffix.size());
    settings.m_values.emplace(std::string{path}, entry.m_value);
  }

  if (!user_file || !std::filesystem::exists(*user_file)) return settings;
  auto const user = cspec::SpecTree::load(*user_file);
  if (!user) return std::unexpected(user.error());
  for (auto const& entry : user->entries()) {
    auto const it = settings.m_values.find(entry.m_path);
    if (it == settings.m_values.end()) return std::unexpected(user->where(entry.m_path) + ": unknown setting \"" + entry.m_path + "\"");
    if (auto const allowed = schema->value("setting." + entry.m_path + ".allowed")) {
      bool ok = false;
      for (auto const& value : text::split_list(*allowed)) ok = ok || value == entry.m_value;
      if (!ok) return std::unexpected(user->where(entry.m_path) + ": allowed values are " + *allowed);
    }
    it->second = entry.m_value;
  }
  return settings;
}

std::string Settings::get(std::string_view name) const {
  auto const it = m_values.find(name);
  if (it == m_values.end()) throw std::logic_error("setting \"" + std::string{name} + "\" is not in specs/settings/settings.cspec");
  return it->second;
}

} // namespace cratchit
