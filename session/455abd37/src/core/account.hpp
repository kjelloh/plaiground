#pragma once

// BAS account numbers and account lists like "2610-2613, 2617"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

using AccountNo = int;

std::expected<AccountNo, std::string> parse_account_no(std::string_view text);

class AccountList {
public:
  static std::expected<AccountList, std::string> parse(std::string_view text);

  bool contains(AccountNo account) const;
  bool empty() const { return m_ranges.empty(); }

private:
  struct Range {
    AccountNo m_first;
    AccountNo m_last;
  };
  std::vector<Range> m_ranges;
};

} // namespace cratchit
