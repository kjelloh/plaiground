// INK2R made from the session's SIE files against the INK2R the company filed for the same
// fiscal year (both git ignored, in _Resources; the tests skip when they are missing).
#include "core/balances.hpp"
#include "core/closing.hpp"
#include "core/date.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/mapping.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"
#include "core/sru.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <iterator>
#include <set>
#include <string>
#include <vector>

namespace {

std::filesystem::path const sie_folder = CRATCHIT_INK2_SIE_FOLDER;
std::filesystem::path const sru_folder = CRATCHIT_INK2_SRU_FOLDER;

std::string compact_date(cratchit::Date date) {
  return std::format("{:04}{:02}{:02}", static_cast<int>(date.year()), static_cast<unsigned>(date.month()),
                     static_cast<unsigned>(date.day()));
}

struct Made {
  cratchit::sie::FiscalYear year;
  cratchit::ink2::Form form;
};

Made make_form(std::filesystem::path const& sie) {
  auto const lexed = cratchit::sie::lex_file(sie);
  EXPECT_TRUE(lexed) << lexed.error();
  auto const parsed = cratchit::sie::parse_document(lexed->lines);
  EXPECT_TRUE(parsed.errors.empty());
  auto const unbooked = cratchit::unbooked_result(parsed.document);
  auto const document = unbooked ? cratchit::with_closing_entry(parsed.document, *unbooked) : parsed.document;
  auto const lines = cratchit::ink2::parse_mapping(cratchit::ink2::mapping_text());
  EXPECT_TRUE(lines);
  return {*document.year(0), cratchit::ink2::build_form(*lines, cratchit::year_balances(document, 0),
                                                       cratchit::ink2::sru_codes(document))};
}

// Where the made INK2R differs from the filed one, "<code>: filed <amount>, made <amount>", a field
// not given or not used counting as 0; and the fiscal year when it differs
std::vector<std::string> differences(Made const& made, cratchit::SruForm const& filed) {
  std::vector<std::string> out;
  auto const start = compact_date(made.year.start);
  auto const end = compact_date(made.year.end);
  if (filed.fields.at("7011") != start || filed.fields.at("7012") != end) {
    out.push_back(std::format("fiscal year: filed {}-{}, made {}-{}", filed.fields.at("7011"), filed.fields.at("7012"),
                              start, end));
  }
  std::set<std::string> codes;
  for (auto const& [code, value] : filed.fields) {
    if (code != "7011" && code != "7012") {
      codes.insert(code);
    }
  }
  for (auto const& [code, field] : made.form.fields) {
    if (field.used) {
      codes.insert(code);
    }
  }
  for (auto const& code : codes) {
    long long filed_amount = 0;
    if (auto const value = filed.fields.find(code); value != filed.fields.end()) {
      auto const& text = value->second;
      auto const [end_of_number, error] = std::from_chars(text.data(), text.data() + text.size(), filed_amount);
      EXPECT_TRUE(error == std::errc{} && end_of_number == text.data() + text.size()) << code << " " << text;
    }
    auto const made_field = made.form.fields.find(code);
    long long const made_amount =
        made_field != made.form.fields.end() && made_field->second.used ? made_field->second.kronor : 0;
    if (filed_amount != made_amount) {
      out.push_back(std::format("{}: filed {}, made {}", code, filed_amount, made_amount));
    }
  }
  return out;
}

struct Filed {
  char const* sie;
  char const* sru; // under sru_folder
  std::vector<std::string> differences;
  std::vector<std::string> errors;
  std::vector<std::string> sru_notes; // the notes on the books' #SRU
};

void check(Filed const& filed) {
  auto const sie = sie_folder / filed.sie;
  auto const sru = sru_folder / filed.sru;
  if (!std::filesystem::exists(sie) || !std::filesystem::exists(sru)) {
    GTEST_SKIP() << "No " << sie << " or " << sru;
  }
  auto const made = make_form(sie);
  auto const forms = cratchit::read_sru(sru);
  ASSERT_TRUE(forms) << forms.error();
  auto const ink2r = std::ranges::find(*forms, "INK2R", &cratchit::SruForm::kind);
  ASSERT_NE(ink2r, forms->end());
  EXPECT_EQ(differences(made, *ink2r), filed.differences);
  EXPECT_EQ(made.form.errors, filed.errors);
  std::vector<std::string> sru_notes;
  std::ranges::copy_if(made.form.notes, std::back_inserter(sru_notes),
                       [](std::string const& note) { return note.starts_with("#SRU"); });
  EXPECT_EQ(sru_notes, filed.sru_notes);
}

// Fortnox's #SRU agree with BAS's table but for these own accounts of the company
std::string const own_accounts = "#SRU: no #SRU in the books for 1226, 1227; the mapping decides";

} // namespace

TEST(Ink2Filed, Year2021_22) {
  check({"TheITfiedAB20221124_192100.se",
         "deklaration 2022/blanketter.sru",
         {
             // Filed: moms 2650 with a debit balance (428.09) moved to övriga fordringar, as BAS asks at
             // closing (1650 Momsfordran); the books leave it on 2650, a debt account
             "7261: filed 1179, made 751",
             // Filed: 464825.51 and 195.50 rounded down, so that the filed balansräkning (with 2650 as a
             // receivable) adds up in whole kronor: 504840 on both sides, where rounding each field gives
             // 504841 against 504840
             "7281: filed 464825, made 464826",
             "7365: filed 195, made 196",
             "7369: filed 0, made -428",
         },
         {},
         {own_accounts}});
}

TEST(Ink2Filed, Year2022_23) {
  check({"TheITfiedAB20230503_152345.se",
         "deklaration 2023/blanketter.sru",
         {
             // Filed without övriga skulder (2641, 2893, 2898): the filed balansräkning does not add up
             "7369: filed 0, made 6270",
             // Filed: 8710 (195.50, not in BAS 2023's table) in övriga rörelseintäkter with 3999
             "7413: filed 67, made -129",
             // Filed: 84192.50 rounded down so that the resultaträkning adds up in whole kronor
             "7550: filed 84192, made 84193",
         },
         {"Account 8710 has an amount -195.50 but is in no field of the mapping, nor is its #SRU 7418"},
         {own_accounts}});
}

TEST(Ink2Filed, Year2023_24) {
  // The latest of the three 2024 filings; their INK2R are the same
  check({"TheITfiedAB20240727_213539.se",
         "deklaration 2024/The ITfied AB 556782-8172/blanketter.sru",
         {
             // Filed: 25024.84 rounded down so that the balansräkning adds up in whole kronor
             "7369: filed 25024, made 25025",
         },
         {},
         {own_accounts}});
}

TEST(Ink2Filed, Year2024_25) {
  check({"TheITfiedAB20250817_172409.se", "deklaration 2025/BLANKETTER.SRU", {}, {}, {own_accounts}});
}
