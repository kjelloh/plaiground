#include "core/account_types.hpp"

namespace cratchit {

namespace {

std::expected<void, std::string> read_level(cspec::SpecTree const& spec, std::string const& level, std::map<int, char>& out) {
  for (auto const& key : spec.children(level)) {
    auto const path = level + "." + key + ".type";
    auto const type = spec.value(path);
    if (!type) continue;
    if (*type == "mixed") continue;
    if (type->size() != 1 || std::string_view{"TSIK"}.find(type->front()) == std::string_view::npos) {
      return std::unexpected(spec.where(path) + ": type must be T, S, I, K or mixed");
    }
    out[std::stoi(key)] = type->front();
  }
  return {};
}

} // namespace

std::expected<AccountTypes, std::string> AccountTypes::from(cspec::SpecTree const& spec) {
  AccountTypes types;
  for (auto const& [level, target] : {std::pair{"account", &types.m_by_account}, std::pair{"group", &types.m_by_group},
                                      std::pair{"class", &types.m_by_class}}) {
    if (auto ok = read_level(spec, level, *target); !ok) return std::unexpected(ok.error());
  }
  return types;
}

std::optional<char> AccountTypes::type_of(AccountNo account, std::optional<char> sie_type) const {
  if (sie_type) return sie_type;
  if (auto const it = m_by_account.find(account); it != m_by_account.end()) return it->second;
  auto const digits = std::to_string(account);
  if (digits.size() >= 2) {
    if (auto const it = m_by_group.find(std::stoi(digits.substr(0, 2))); it != m_by_group.end()) return it->second;
  }
  if (auto const it = m_by_class.find(digits.front() - '0'); it != m_by_class.end()) return it->second;
  return std::nullopt;
}

} // namespace cratchit
