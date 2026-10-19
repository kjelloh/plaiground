#include "core/account.hpp"

#include "core/text.hpp"

namespace cratchit {

std::expected<AccountNo, std::string> parse_account_no(std::string_view text) {
  if (text.empty() || text.size() > 6) return std::unexpected("invalid account \"" + std::string{text} + "\"");
  AccountNo account = 0;
  for (char c : text) {
    if (c < '0' || c > '9') return std::unexpected("invalid account \"" + std::string{text} + "\"");
    account = account * 10 + (c - '0');
  }
  return account;
}

std::expected<AccountList, std::string> AccountList::parse(std::string_view text) {
  AccountList list;
  for (auto const& part : text::split_list(text)) {
    auto const dash = part.find('-');
    auto const first = parse_account_no(text::trim(std::string_view{part}.substr(0, dash)));
    if (!first) return std::unexpected(first.error());
    auto last = first;
    if (dash != std::string::npos) last = parse_account_no(text::trim(std::string_view{part}.substr(dash + 1)));
    if (!last) return std::unexpected(last.error());
    if (*last < *first) return std::unexpected("empty account range \"" + part + "\"");
    list.m_ranges.push_back({*first, *last});
  }
  return list;
}

bool AccountList::contains(AccountNo account) const {
  for (auto const& range : m_ranges) {
    if (range.m_first <= account && account <= range.m_last) return true;
  }
  return false;
}

} // namespace cratchit
