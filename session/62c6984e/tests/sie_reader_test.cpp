#include "sie_reader.hpp"

#include <gtest/gtest.h>

using namespace std::chrono;
using cratchit::Amount;
using cratchit::parse_sie;
using cratchit::read_sie_file;

namespace {

std::filesystem::path const source_dir{CRATCHIT_SOURCE_DIR};

} // namespace

TEST(ReadSieFile, ExampleFile) {
  auto const result = read_sie_file(source_dir / "testdata" / "example.se");
  ASSERT_TRUE(result.has_value()) << result.error();
  auto const& doc = *result;

  EXPECT_EQ(doc.org_nr, "556000-0175");

  ASSERT_EQ(doc.financial_years.size(), 2u);
  EXPECT_EQ(doc.financial_years[0].index, 0);
  EXPECT_EQ(doc.financial_years[0].start, 2024y / May / 1d);
  EXPECT_EQ(doc.financial_years[0].end, 2025y / April / 30d);
  EXPECT_EQ(doc.financial_years[1].index, -1);

  EXPECT_EQ(doc.accounts.size(), 5u);
  EXPECT_EQ(doc.accounts.at(2641), "Debiterad ingående moms");

  ASSERT_EQ(doc.vouchers.size(), 2u);
  auto const& first = doc.vouchers[0];
  EXPECT_EQ(first.series, "A");
  EXPECT_EQ(first.number, "1");
  EXPECT_EQ(first.date, 2024y / July / 15d);
  EXPECT_EQ(first.text, "Inköp \"molntjänst\"");
  ASSERT_EQ(first.postings.size(), 3u);
  EXPECT_EQ(first.postings[0].account, 6540);
  EXPECT_EQ(first.postings[0].amount, Amount::from_cents(10'000));
  EXPECT_EQ(first.postings[0].text, "Molntjänst");
  EXPECT_FALSE(first.postings[0].date.has_value());
  EXPECT_EQ(first.postings[2].amount, Amount::from_cents(-12'500));
}

TEST(ReadSieFile, CorrectionHistoryIsIgnored) {
  auto const doc = read_sie_file(source_dir / "testdata" / "example.se").value();
  auto const& second = doc.vouchers[1];
  ASSERT_EQ(second.postings.size(), 2u);  // #BTRANS and #RTRANS are skipped
  EXPECT_EQ(second.postings[0].account, 1920);
  EXPECT_EQ(second.postings[1].account, 1610);
  EXPECT_EQ(second.postings[1].amount, Amount::from_cents(31'840));
  EXPECT_EQ(second.postings[1].date, 2024y / August / 2d);
}

TEST(ReadSieFile, MissingFile) {
  auto const result = read_sie_file(source_dir / "testdata" / "no_such_file.se");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("Cannot open"), std::string::npos);
}

TEST(ParseSie, ErrorsNameTheLine) {
  auto const result = parse_sie("#ORGNR 556000-0175\n#VER A 1 2024071 \"x\"\n{\n}\n");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().rfind("Line 2:", 0), 0u) << result.error();
}

TEST(ParseSie, RejectsBrokenVoucherBlocks) {
  EXPECT_FALSE(parse_sie("#VER A 1 20240715 \"x\"\n#TRANS 1920 {} 1\n").has_value());  // no '{'
  EXPECT_FALSE(parse_sie("#VER A 1 20240715 \"x\"\n{\n#TRANS 1920 {} 1\n").has_value());  // no '}'
  EXPECT_FALSE(parse_sie("}\n").has_value());
  EXPECT_FALSE(parse_sie("#VER A 1 20240715 \"x\"\n{\n#KONTO 1920 \"Bank\"\n}\n").has_value());
}

TEST(ParseSie, UnknownRecordsAreIgnored) {
  auto const doc = parse_sie("#PSALDO 0 202405 1920 {} 100\n#KSUMMA\n").value();
  EXPECT_TRUE(doc.vouchers.empty());
}
