#include "core/financial_event.hpp"
#include "statements/csv_statement_reader.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

using namespace cratchit;

namespace {

std::filesystem::path const root{CRATCHIT_SOURCE_DIR};

statements::StatementSpec load_spec(std::string const& name) {
  auto const tree = cspec::SpecTree::load(root / "specs" / "statements" / (name + ".cspec"));
  EXPECT_TRUE(tree) << tree.error();
  auto spec = statements::StatementSpec::from(*tree);
  EXPECT_TRUE(spec) << spec.error();
  return *spec;
}

std::string read(std::string const& name) {
  std::ifstream file{root / "testdata" / name, std::ios::binary};
  std::ostringstream content;
  content << file.rdbuf();
  return content.str();
}

} // namespace

TEST(Statements, NordeaCsvToEvents) {
  auto const events = statements::read_statement(read("nordea_example.csv"), load_spec("nordea_csv"), "bank_statement_import",
                                                 "nordea_example.csv", 1920);
  ASSERT_TRUE(events) << events.error();
  ASSERT_EQ(events->size(), 3u);
  EXPECT_EQ(to_yyyymmdd((*events)[0].m_date), "20251229");
  EXPECT_EQ((*events)[0].m_amount.ore(), -115875);
  EXPECT_EQ((*events)[0].m_heading, "WEBBHOTELLET AB | 62500016901740");
  EXPECT_EQ((*events)[2].m_heading, "Insättning | FRÅN SPAR");
  EXPECT_EQ((*events)[0].m_source_row, 2);
}

TEST(Statements, SkvAccountCsvSkipsBalanceRows) {
  auto const events = statements::read_statement(read("skv_account_example.csv"), load_spec("skv_account_csv"),
                                                 "skv_account_statement_import", "skv_account_example.csv", 1630);
  ASSERT_TRUE(events) << events.error();
  ASSERT_EQ(events->size(), 3u);
  EXPECT_EQ((*events)[0].m_heading, "Intäktsränta"); // decoded from ISO-8859-1
  EXPECT_EQ((*events)[1].m_amount.ore(), 199700);
  EXPECT_EQ((*events)[2].m_amount.ore(), -199700);
}

TEST(Statements, EventListingRoundTrips) {
  auto const events = statements::read_statement(read("nordea_example.csv"), load_spec("nordea_csv"), "bank_statement_import",
                                                 "nordea_example.csv", 1920);
  ASSERT_TRUE(events);
  auto const listing = to_cspec(*events);
  auto const tree = cspec::SpecTree::parse(listing, "listing");
  ASSERT_TRUE(tree) << tree.error();
  auto const back = events_from_cspec(*tree);
  ASSERT_TRUE(back) << back.error();
  ASSERT_EQ(back->size(), events->size());
  EXPECT_EQ((*back)[0].m_heading, (*events)[0].m_heading);
  EXPECT_EQ((*back)[0].m_amount, (*events)[0].m_amount);
  EXPECT_EQ((*back)[0].m_account, 1920);
}
