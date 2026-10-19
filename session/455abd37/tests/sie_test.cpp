#include "cspec/spec_tree.hpp"
#include "legal/compliance.hpp"
#include "sie/sie_reader.hpp"
#include "sie/sie_to_journal.hpp"

#include <gtest/gtest.h>

using namespace cratchit;

namespace {

std::filesystem::path const root{CRATCHIT_SOURCE_DIR};

sie::SieSpec load_spec() {
  auto const tree = cspec::SpecTree::load(root / "specs" / "sie" / "sie4.cspec");
  EXPECT_TRUE(tree) << tree.error();
  auto spec = sie::SieSpec::from(*tree);
  EXPECT_TRUE(spec) << spec.error();
  return *spec;
}

} // namespace

TEST(Sie, ReadsExampleFileIntoBooks) {
  auto const records = sie::read_file(root / "testdata" / "example.se", load_spec());
  ASSERT_TRUE(records) << records.error();
  auto const books = sie::to_books(*records);
  ASSERT_TRUE(books) << books.error();

  EXPECT_EQ(books->m_company_name, "Exempelbolaget AB");
  EXPECT_EQ(books->m_org_nr, "556000-0175");
  ASSERT_TRUE(books->current_year());
  EXPECT_EQ(to_yyyymmdd(books->current_year()->m_first), "20240501");
  EXPECT_EQ(books->m_accounts.at(2641).m_name, "Debiterad ingående moms"); // CP437 decoded
  EXPECT_EQ(books->m_accounts.at(1920).m_sru_code, 7281);
  EXPECT_EQ(books->m_opening_balances.at(1920).ore(), 100000); // #IB -1 is not year 0
  ASSERT_EQ(books->m_vouchers.size(), 5u);

  auto const& first = books->m_vouchers[0];
  EXPECT_EQ(voucher_id(first), "A-1");
  EXPECT_EQ(first.m_text, "Inköp \"molntjänst\"");
  EXPECT_EQ(first.m_postings.size(), 3u);

  // #BTRANS and #RTRANS are not postings; the #TRANS after #RTRANS is
  auto const& corrected = books->m_vouchers[2];
  ASSERT_EQ(corrected.m_postings.size(), 3u);
  EXPECT_EQ(corrected.m_postings[1].m_account, 6110);

  EXPECT_TRUE(legal::check_books(*books).empty());
}

TEST(Sie, SpecDecidesFieldsAndTypes) {
  auto const spec = load_spec();
  auto const bad_date = sie::read_records("#VER A 1 2024-07-15 \"x\"\n{\n}\n", spec);
  ASSERT_FALSE(bad_date);
  EXPECT_NE(bad_date.error().find("field 'date'"), std::string::npos);

  auto const too_many = sie::read_records("#KONTO 1920 \"PlusGiro\" extra\n", spec);
  EXPECT_FALSE(too_many);

  auto const trans_outside = sie::read_records("#TRANS 1920 {} 1.00\n", spec);
  EXPECT_FALSE(trans_outside);

  auto const unknown = sie::read_records("#NYTT 1 \"two\"\n", spec);
  ASSERT_TRUE(unknown);
  EXPECT_FALSE(unknown->front().m_known);
  EXPECT_EQ(unknown->front().m_fields.at("2"), "two");
}

TEST(Sie, ObjectListsAndEmptyOptionalFields) {
  auto const records = sie::read_records("#VER A 1 20240101 \"\"\n{\n#TRANS 1920 {1 \"a b\"} -1.00 \"\" \"text\"\n}\n", load_spec());
  ASSERT_TRUE(records) << records.error();
  auto const& trans = records->front().m_block.front();
  EXPECT_EQ(trans.m_fields.at("objects"), "{1 \"a b\"}");
  EXPECT_EQ(trans.m_fields.at("transaction_date"), "");
  EXPECT_EQ(trans.m_fields.at("text"), "text");
}
