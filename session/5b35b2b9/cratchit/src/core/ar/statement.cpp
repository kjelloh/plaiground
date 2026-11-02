#include "core/ar/statement.hpp"

#include <algorithm>
#include <format>

namespace cratchit::ar {

namespace {

// The index of the heading that starts the scope of the sum at index, or lines.size() when none
std::size_t scope_start(std::vector<StatementLine> const& lines, std::size_t index) {
  for (std::size_t at = index; at-- > 0;) {
    if (lines[at].kind == LineKind::heading && lines[at].level <= lines[index].level) {
      return at;
    }
  }
  return lines.size();
}

// The index after the last line under the heading at index
std::size_t scope_end(std::vector<StatementLine> const& lines, std::size_t index) {
  auto at = index + 1;
  while (at < lines.size() && !(lines[at].kind != LineKind::item && lines[at].level <= lines[index].level)) {
    ++at;
  }
  return at;
}

} // namespace

Kronor to_kronor(Amount amount) {
  return amount >= 0 ? (amount + 50) / 100 : -((-amount + 50) / 100);
}

Balances year_balances(sie::Document const& document, int year) {
  Balances balances;
  for (auto const& balance : document.closing) {
    if (balance.year == year) {
      balances[balance.account] += balance.amount;
    }
  }
  for (auto const& result : document.results) {
    if (result.year == year) {
      balances[result.account] += result.amount;
    }
  }
  return balances;
}

Statement build_statement(std::vector<Line> const& layout, std::vector<Balances> const& years) {
  Statement statement;
  // Every account with an amount must be on exactly one line
  std::map<std::string, std::vector<std::string>> lines_of;
  for (auto const& balances : years) {
    for (auto const& [account, amount] : balances) {
      if (amount == 0 || lines_of.contains(account)) {
        continue;
      }
      auto& texts = lines_of[account];
      for (auto const& line : layout) {
        if ((line.kind == LineKind::item || line.kind == LineKind::outside) && line.accounts.contains(account)) {
          texts.push_back(line.text);
        }
      }
      if (texts.empty()) {
        statement.errors.push_back(std::format("Account {} has an amount but is on no line of the årsredovisning", account));
      } else if (texts.size() > 1) {
        statement.errors.push_back(std::format("Account {} is on more than one line: {}", account, texts[0]));
      }
    }
  }
  for (auto const& line : layout) {
    if (line.kind == LineKind::outside) {
      continue;
    }
    StatementLine out{line.level, line.kind, line.text, std::vector<Amount>(years.size(), 0), {}, {}, false};
    if (line.kind == LineKind::item) {
      out.accounts.resize(years.size());
      for (std::size_t year = 0; year < years.size(); ++year) {
        for (auto const& [account, amount] : years[year]) {
          if (amount != 0 && line.accounts.contains(account)) {
            out.accounts[year][account] = amount;
            out.exact[year] += account.starts_with('1') ? amount : -amount;
            out.shown = true;
          }
        }
      }
    }
    statement.lines.push_back(std::move(out));
  }
  auto& lines = statement.lines;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (lines[index].kind == LineKind::sum) {
      for (auto at = scope_start(lines, index) + 1; at < index; ++at) {
        if (lines[at].kind == LineKind::item) {
          lines[index].shown = lines[index].shown || lines[at].shown;
          for (std::size_t year = 0; year < years.size(); ++year) {
            lines[index].exact[year] += lines[at].exact[year];
          }
        }
      }
    } else if (lines[index].kind == LineKind::heading) {
      auto const end = scope_end(lines, index);
      lines[index].shown = std::any_of(lines.begin() + static_cast<std::ptrdiff_t>(index) + 1,
                                       lines.begin() + static_cast<std::ptrdiff_t>(end),
                                       [](StatementLine const& line) { return line.kind == LineKind::item && line.shown; });
    }
  }
  for (auto& line : lines) {
    for (auto const amount : line.exact) {
      line.amounts.push_back(to_kronor(amount));
    }
  }
  return statement;
}

std::vector<Amount> const* amounts_of(Statement const& statement, std::string_view text) {
  auto const found = std::ranges::find(statement.lines, text, &StatementLine::text);
  return found == statement.lines.end() ? nullptr : &found->exact;
}

std::vector<std::string> check_statement(Statement const& statement) {
  std::vector<std::string> findings;
  auto const differ = [&](std::string_view label, std::vector<Amount> const& left, std::vector<Amount> const& right) {
    for (std::size_t year = 0; year < left.size() && year < right.size(); ++year) {
      if (left[year] != right[year]) {
        findings.push_back(std::format("Year {}: {} differ by {} ({} against {})", -static_cast<int>(year), label,
                                       format_amount(left[year] - right[year]), format_amount(left[year]),
                                       format_amount(right[year])));
      }
    }
  };
  auto const assets = amounts_of(statement, "Summa tillgångar");
  auto const equity_and_debt = amounts_of(statement, "Summa eget kapital och skulder");
  if (assets && equity_and_debt) {
    differ("Summa tillgångar and Summa eget kapital och skulder", *assets, *equity_and_debt);
  }
  // Årets resultat is first a sum in the resultaträkning, then an item in the balansräkning
  auto const first = std::ranges::find(statement.lines, "Årets resultat", &StatementLine::text);
  if (first != statement.lines.end()) {
    auto const second = std::find_if(first + 1, statement.lines.end(),
                                     [](StatementLine const& line) { return line.text == "Årets resultat"; });
    if (second != statement.lines.end()) {
      differ("Årets resultat in resultaträkning and balansräkning", first->exact, second->exact);
    }
  }
  return findings;
}

} // namespace cratchit::ar
