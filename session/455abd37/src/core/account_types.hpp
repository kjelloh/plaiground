#pragma once

// BAS account type lookup, driven by specs/bas/account_types.cspec

#include "core/account.hpp"
#include "cspec/spec_tree.hpp"

#include <expected>
#include <map>
#include <optional>
#include <string>

namespace cratchit {

class AccountTypes {
public:
  static std::expected<AccountTypes, std::string> from(cspec::SpecTree const& spec);

  // 'T', 'S', 'I' or 'K'. 'sie_type' is #KTYP from the SIE file, which wins.
  // std::nullopt when the spec says "mixed" and nothing more specific is known.
  std::optional<char> type_of(AccountNo account, std::optional<char> sie_type = std::nullopt) const;

private:
  std::map<int, char> m_by_account;
  std::map<int, char> m_by_group;
  std::map<int, char> m_by_class;
};

} // namespace cratchit
