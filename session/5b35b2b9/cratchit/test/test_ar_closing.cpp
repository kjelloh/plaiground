#include "core/ar/closing.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using cratchit::ar::Balances;
using cratchit::ar::check_closing;
using namespace std::chrono;

namespace {

constexpr cratchit::Date balance_day = 2023y / April / 30;

// The one note, or empty
std::string note_of(Balances const& balances) {
  auto const notes = check_closing(balances, balance_day);
  EXPECT_LE(notes.size(), 1u);
  return notes.empty() ? "" : notes.front();
}

} // namespace

TEST(ArClosing, ClosedBooksHaveNoNotes) {
  EXPECT_TRUE(check_closing({{"1650", 133146}, {"1630", 63000}, {"2650", -5000}, {"2510", -62500}, {"2999", 0}},
                            balance_day)
                  .empty());
}

TEST(ArClosing, VatNettingToAReceivable) {
  // Unreported ingående moms is larger than utgående moms
  EXPECT_EQ(note_of({{"2611", -10000}, {"2641", 133146}}),
            "Moms (2610-2659) nets to a receivable 1231.46 at 2023-04-30; BAS: move it to 1650 Momsfordran");
  EXPECT_EQ(note_of({{"2650", 42800}}),
            "Moms (2610-2659) nets to a receivable 428.00 at 2023-04-30; BAS: move it to 1650 Momsfordran");
  EXPECT_EQ(note_of({{"2611", -20000}, {"2641", 13314}}), "");
  // Punktskatter and OSS are not part of it
  EXPECT_EQ(note_of({{"2660", 100}}), "");
}

TEST(ArClosing, TaxNettingToAReceivable) {
  EXPECT_NE(note_of({{"2510", 30000}, {"2512", -10000}}).find("1640 Skattefordringar"), std::string::npos);
  EXPECT_EQ(note_of({{"2510", -30000}}), "");
}

TEST(ArClosing, TaxAccountWithCreditBalance) {
  EXPECT_EQ(note_of({{"1630", -2500}}), "1630 Skattekonto has a credit balance -25.00 at 2023-04-30; BAS: move it to 2850");
  EXPECT_EQ(note_of({{"1630", 2500}}), "");
}

TEST(ArClosing, RedovisningsmedelNotHeldForOthers) {
  EXPECT_NE(note_of({{"1999", 15675}}).find("Redovisningsmedel (1990-1999) 156.75"), std::string::npos);
  EXPECT_EQ(note_of({{"1990", 50000}, {"2830", -50000}}), "");
}

TEST(ArClosing, SuspenseAccountNotZero) {
  EXPECT_EQ(note_of({{"2999", -100}}), "2999 OBS-konto is -1.00 at 2023-04-30; BAS: it must be zero at year end");
}

namespace {

// The notes for years 0 and -1 of one of the session's SIE files (git ignored), or nothing when missing
std::optional<std::vector<std::string>> notes_of_file(char const* name) {
  auto const sie = std::filesystem::path(CRATCHIT_AR_SIE_FOLDER) / name;
  if (!std::filesystem::exists(sie)) {
    return std::nullopt;
  }
  auto const lexed = cratchit::sie::lex_file(sie);
  EXPECT_TRUE(lexed);
  auto const document = cratchit::sie::parse_document(lexed->lines).document;
  std::vector<std::string> notes;
  for (int index = 0; index >= -1; --index) {
    for (auto const& note : check_closing(cratchit::ar::year_balances(document, index), document.year(index)->end)) {
      notes.push_back(note.substr(0, note.find(';')));
    }
  }
  return notes;
}

} // namespace

TEST(ArClosing, FiledYearsLackTheVatClosingEntry) {
  // Deduced from the vouchers: open VAT and unreported VAT parked on 1999 "OBS" at each year end
  auto const notes = notes_of_file("TheITfiedAB20230503_152345.se");
  if (!notes) {
    GTEST_SKIP() << "No SIE file";
  }
  EXPECT_EQ(*notes, (std::vector<std::string>{
                        "Moms (2610-2659) nets to a receivable 1331.46 at 2023-04-30",
                        "Redovisningsmedel (1990-1999) 156.75 at 2023-04-30 is not matched by Avräkning för annans "
                        "räkning (2830-2839) 0.00",
                        "Moms (2610-2659) nets to a receivable 428.00 at 2022-04-30",
                        "Redovisningsmedel (1990-1999) 161.25 at 2022-04-30 is not matched by Avräkning för annans "
                        "räkning (2830-2839) 0.00",
                    }));
  EXPECT_EQ(notes_of_file("TheITfiedAB20240727_213539.se")->size(), 3u);
  EXPECT_EQ(*notes_of_file("TheITfiedAB20250817_172409.se"),
            (std::vector<std::string>{
                "Moms (2610-2659) nets to a receivable 67.60 at 2025-04-30",
                "Redovisningsmedel (1990-1999) 336.00 at 2025-04-30 is not matched by Avräkning för annans räkning "
                "(2830-2839) 0.00",
                "Moms (2610-2659) nets to a receivable 156.75 at 2024-04-30",
            }));
}
