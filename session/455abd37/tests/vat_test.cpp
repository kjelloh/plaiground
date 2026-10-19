#include "cspec/spec_tree.hpp"
#include "sie/sie_reader.hpp"
#include "sie/sie_to_journal.hpp"
#include "vat/eskd_writer.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

using namespace cratchit;

namespace {

std::filesystem::path const root{CRATCHIT_SOURCE_DIR};
std::filesystem::path const specs = root / "specs";

Books example_books() {
  auto const tree = cspec::SpecTree::load(specs / "sie" / "sie4.cspec");
  auto const spec = sie::SieSpec::from(*tree);
  auto const records = sie::read_file(root / "testdata" / "example.se", *spec);
  EXPECT_TRUE(records) << records.error();
  auto books = sie::to_books(*records);
  EXPECT_TRUE(books) << books.error();
  return *books;
}

vat::VatSpec vat_spec() {
  auto spec = vat::load_vat_spec(specs);
  EXPECT_TRUE(spec) << spec.error();
  return *spec;
}

vat::VatPeriod period(std::string_view yyyymm) { return *vat::VatPeriod::parse(yyyymm, "quarter"); }

} // namespace

TEST(Vat, PeriodRange) {
  auto const range = period("202409").range();
  EXPECT_EQ(to_yyyymmdd(range.m_first), "20240701");
  EXPECT_EQ(to_yyyymmdd(range.m_last), "20240930");
  EXPECT_FALSE(vat::VatPeriod::parse("202408", "quarter"));
}

TEST(Vat, ComputesBoxesFromJournalUsingSpecMapping) {
  auto const result = vat::compute_vat_return({example_books()}, period("202409"), "M", vat_spec());
  ASSERT_TRUE(result) << result.error();
  auto const& boxes = result->m_boxes;
  EXPECT_EQ(boxes.at(20).ore(), 40000);  // 9021
  EXPECT_EQ(boxes.at(30).ore(), 10000);  // 2614, credit
  EXPECT_EQ(boxes.at(48).ore(), 17500);  // 2641 + 2645 = 175.20, truncated
  EXPECT_EQ(boxes.at(49).ore(), -7500);  // 100 - 175
  EXPECT_EQ(boxes.size(), 4u);           // zero boxes left out
  EXPECT_EQ(result->m_vouchers_used, (std::vector<std::string>{"A-1", "A-2", "A-3"})); // not M-1, not A-4
}

TEST(Vat, UnmappedVatAccountIsAnError) {
  auto books = example_books();
  books.m_vouchers[0].m_postings[1].m_account = 2660;
  auto const result = vat::compute_vat_return({books}, period("202409"), "M", vat_spec());
  ASSERT_FALSE(result);
  EXPECT_NE(result.error().find("2660"), std::string::npos);
}

TEST(Vat, EskdWriterReproducesSkvExample6) {
  auto const vat = vat_spec();
  auto eskd = vat::load_eskd_spec(specs);
  ASSERT_TRUE(eskd) << eskd.error();
  eskd->m_include_doctype = false; // SKV's examples have no DOCTYPE and one element per line
  eskd->m_body_on_one_line = false;

  vat::VatReturn vat_return{"556000-0175", *vat::VatPeriod::parse("202401", "month"), {}, "Skriv den upplysning du vill skicka in", {}};
  std::map<int, std::int64_t> const kronor{{5, 100000}, {6, 200000}, {7, 300000}, {8, 400000}, {20, 5000},  {21, 6000},  {22, 7000},
                                           {23, 8000},  {24, 9000},  {50, 10000}, {35, 11000}, {36, 12000}, {37, 13000}, {38, 14000},
                                           {39, 15000}, {40, 16000}, {41, 17000}, {42, 18000}, {10, 200000}, {11, 15000}, {12, 5000},
                                           {30, 2500},  {31, 1000},  {32, 500},   {60, 2000},  {61, 350},   {62, 150},   {48, 1000},
                                           {49, 225500}};
  for (auto const& [box, value] : kronor) vat_return.m_boxes[box] = Amount::from_ore(value * 100);

  auto const content = vat::to_eskd(vat_return, *eskd, vat);
  ASSERT_TRUE(content) << content.error();
  std::ifstream file{root / "testdata" / "skv_example_6.eskd", std::ios::binary};
  std::ostringstream expected;
  expected << file.rdbuf();
  EXPECT_EQ(*content, expected.str());
}

TEST(Vat, EskdDefaultLayoutHasDoctypeAndOneLineBody) {
  auto const vat = vat_spec();
  auto const eskd = vat::load_eskd_spec(specs);
  ASSERT_TRUE(eskd);
  vat::VatReturn vat_return{"556000-0175", period("202203"), {{48, Amount::from_ore(289900)}, {49, Amount::from_ore(-289900)}}, "", {}};
  auto const content = vat::to_eskd(vat_return, *eskd, vat);
  ASSERT_TRUE(content) << content.error();
  EXPECT_EQ(*content, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n" + eskd->m_doctype +
                          "\n<eSKDUpload Version=\"6.0\"><OrgNr>556000-0175</OrgNr><Moms><Period>202203</Period>"
                          "<MomsIngAvdr>2899</MomsIngAvdr><MomsBetala>-2899</MomsBetala></Moms></eSKDUpload>\n");
}
