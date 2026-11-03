#include "core/ink2/form.hpp"

#include <algorithm>
#include <format>

namespace cratchit::ink2 {

namespace {

bool is_bas_account(std::string_view account) {
  return account.size() == 4 && std::ranges::all_of(account, [](char c) { return c >= '0' && c <= '9'; }) &&
         account.front() >= '1' && account.front() <= '8';
}

// Whether a group with this net (debit positive) meets its condition
bool meets(Condition condition, Amount net) {
  switch (condition) {
  case Condition::always:
    return true;
  case Condition::credit:
    return net <= 0;
  case Condition::debit:
    return net > 0;
  case Condition::profit:
    return net >= 0;
  case Condition::loss:
    return net < 0;
  }
  return false;
}

std::string join(std::vector<std::string> const& texts) {
  std::string out;
  for (auto const& text : texts) {
    out += (out.empty() ? "" : ", ") + text;
  }
  return out;
}

} // namespace

Form build_form(std::vector<Line> const& lines, Balances const& balances) {
  Form form;
  std::vector<Field const*> fields;
  for (auto const& line : lines) {
    for (auto const& field : line.fields) {
      fields.push_back(&field);
      form.fields[field.sru];
    }
  }
  Amount outside = 0;
  std::vector<std::string> outside_accounts;
  for (auto const& [account, amount] : balances) {
    if (amount == 0) {
      continue;
    }
    if (!is_bas_account(account)) {
      outside += amount;
      outside_accounts.push_back(account);
      continue;
    }
    std::vector<std::string> taken_by;
    for (auto const* field : fields) {
      bool const takes = std::ranges::any_of(field->accounts.groups, [&](AccountGroup const& group) {
        if (!group.contains(account)) {
          return false;
        }
        Amount net = 0;
        for (auto const& [other, other_amount] : balances) {
          if (group.contains(other)) {
            net += other_amount;
          }
        }
        return meets(group.condition, net);
      });
      if (takes) {
        taken_by.push_back(field->sru);
        auto& out = form.fields[field->sru];
        out.used = true;
        out.exact += field->sign * amount;
        out.accounts[account] = amount;
      }
    }
    if (taken_by.empty()) {
      form.errors.push_back(
          std::format("Account {} has an amount {} but is in no field of the mapping", account, format_amount(amount)));
    } else if (taken_by.size() > 1) {
      form.errors.push_back(std::format("Account {} ({}) is in more than one field: {}", account, format_amount(amount),
                                        join(taken_by)));
    }
  }
  if (!outside_accounts.empty()) {
    form.notes.push_back(std::format("Left out, as not BAS accounts of class 1-8: {} (together {})",
                                     join(outside_accounts), format_amount(outside)));
  }
  for (auto& [sru, field] : form.fields) {
    field.kronor = to_kronor(field.exact);
  }
  auto const sums = sums_of(form);
  if (sums.assets != sums.equity_and_debt) {
    form.notes.push_back(std::format("Rounding: tillgångar (72xx) add up to {} kr but eget kapital och skulder (73xx) to "
                                     "{} kr, as each field is rounded on its own",
                                     sums.assets, sums.equity_and_debt));
  }
  if (sums.result != sums.year_result) {
    form.notes.push_back(std::format("Rounding: the fields of the resultaträkning add up to {} kr but årets resultat "
                                     "(3.26 less 3.27) is {} kr, as each field is rounded on its own",
                                     sums.result, sums.year_result));
  }
  return form;
}

Sums sums_of(Form const& form) {
  Sums sums;
  for (auto const& [sru, field] : form.fields) {
    if (sru == "7450") {
      sums.year_result += field.kronor;
    } else if (sru == "7550") {
      sums.year_result -= field.kronor;
    } else if (sru.starts_with("72")) {
      sums.assets += field.kronor;
    } else if (sru.starts_with("73")) {
      sums.equity_and_debt += field.kronor;
    } else if (sru.starts_with("74")) {
      sums.result += field.kronor;
    } else if (sru.starts_with("75")) {
      sums.result -= field.kronor;
    }
  }
  return sums;
}

} // namespace cratchit::ink2
