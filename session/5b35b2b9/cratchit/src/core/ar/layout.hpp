#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cratchit::ar {

// BAS accounts as the mapping writes them, e.g. "1590-1619, 1630-1659 eller 1680-1689"
// or "4900-4999 (förutom 4910-4931 och 4980-4989)". An exclusion applies to the whole set.
struct AccountSet {
  std::vector<std::pair<int, int>> included; // first to last, both included
  std::vector<std::pair<int, int>> excluded;

  // Whether a four digit account number is in the set; false for any other text
  bool contains(std::string_view account) const;
};

// Numbers and ranges separated by ",", "eller" or "och", each optionally
// followed by "(förutom ...)" of the same. Error text quotes the set.
std::expected<AccountSet, std::string> parse_account_set(std::string_view text);

// What a line of an årsredovisning is
enum class LineKind {
  heading, // a title over the lines below it
  item,    // a post with an amount, from its accounts
  sum,     // the items since the last heading at its level or above
  outside, // accounts that are not on the årsredovisning at all
};

// A line of the resultaträkning or balansräkning, in order
struct Line {
  int level = 0; // 1 for the statements, deeper for their parts; 0 for outside
  LineKind kind = LineKind::item;
  AccountSet accounts; // for items and outside only
  std::string text;
};

// The lines of a mapping CSV with the columns Nivå;Typ;BAS Range;ÅR and a
// header row. Typ is Rubrik, Post, Summa or Utanför. Error text names the row.
std::expected<std::vector<Line>, std::string> parse_layout(std::string_view csv_text);

// The mapping built into the program: Resources/bas_to_ar_2026.csv (BAS 2026)
std::string_view layout_text();

} // namespace cratchit::ar
