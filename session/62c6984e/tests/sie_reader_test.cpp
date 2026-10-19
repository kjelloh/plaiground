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

  EXPECT_EQ(doc.m_org_nr, "556000-0175");

  ASSERT_EQ(doc.m_financial_years.size(), 2u);
  EXPECT_EQ(doc.m_financial_years[0].m_index, 0);
  EXPECT_EQ(doc.m_financial_years[0].m_start, 2024y / May / 1d);
  EXPECT_EQ(doc.m_financial_years[0].m_end, 2025y / April / 30d);
  EXPECT_EQ(doc.m_financial_years[1].m_index, -1);

  EXPECT_EQ(doc.m_accounts.size(), 5u);
  EXPECT_EQ(doc.m_accounts.at(2641), "Debiterad ingående moms");

  ASSERT_EQ(doc.m_vouchers.size(), 2u);
  auto const& first = doc.m_vouchers[0];
  EXPECT_EQ(first.m_series, "A");
  EXPECT_EQ(first.m_number, "1");
  EXPECT_EQ(first.m_date, 2024y / July / 15d);
  EXPECT_EQ(first.m_text, "Inköp \"molntjänst\"");
  ASSERT_EQ(first.m_postings.size(), 3u);
  EXPECT_EQ(first.m_postings[0].m_account, 6540);
  EXPECT_EQ(first.m_postings[0].m_amount, Amount::from_cents(10'000));
  EXPECT_EQ(first.m_postings[0].m_text, "Molntjänst");
  EXPECT_FALSE(first.m_postings[0].m_date.has_value());
  EXPECT_EQ(first.m_postings[2].m_amount, Amount::from_cents(-12'500));
}

TEST(ReadSieFile, CorrectionHistoryIsIgnored) {
  auto const doc = read_sie_file(source_dir / "testdata" / "example.se").value();
  auto const& second = doc.m_vouchers[1];
  ASSERT_EQ(second.m_postings.size(), 2u);  // #BTRANS and #RTRANS are skipped
  EXPECT_EQ(second.m_postings[0].m_account, 1920);
  EXPECT_EQ(second.m_postings[1].m_account, 1610);
  EXPECT_EQ(second.m_postings[1].m_amount, Amount::from_cents(25'050));
  EXPECT_EQ(second.m_postings[1].m_date, 2024y / August / 2d);
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
  EXPECT_TRUE(doc.m_vouchers.empty());
}
