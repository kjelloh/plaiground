#include "eskd_reader.hpp"
#include "eskd_writer.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace std::chrono;
using cratchit::Amount;
using cratchit::parse_eskd;
using cratchit::read_eskd_file;
using cratchit::to_eskd;
using cratchit::VatReturn;

namespace {

std::filesystem::path const testdata{std::filesystem::path{CRATCHIT_SOURCE_DIR} / "testdata"};

Amount kronor(std::int64_t value) { return Amount::from_cents(value * 100); }

std::string read_bytes(std::filesystem::path const& path) {
  std::ifstream file{path, std::ios::binary};
  return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

// Element names inside <Moms>, in file order.
std::vector<std::string> moms_element_names(std::string_view text) {
  std::vector<std::string> names;
  auto pos = text.find("<Moms>") + 6;
  while (true) {
    auto const open = text.find('<', pos);
    if (text.substr(open, 2) == "</") break;
    auto const close = text.find('>', open);
    names.emplace_back(text.substr(open + 1, close - open - 1));
    pos = text.find("</", close) + 1;
  }
  return names;
}

VatReturn example_1() {
  return VatReturn{.m_org_nr = "556000-0175",
                   .m_period = 2024y / January,
                   .m_boxes = {{5, kronor(100000)}, {10, kronor(25000)}, {48, kronor(1000)}, {49, kronor(24000)}}};
}

} // namespace

TEST(EskdWriter, ExactLayout) {
  EXPECT_EQ(to_eskd(example_1()).value(),
            "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
            "<!DOCTYPE eSKDUpload PUBLIC \"-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV\" "
            "\"https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd\">\n"
            "<eSKDUpload Version=\"6.0\"><OrgNr>556000-0175</OrgNr><Moms><Period>202401</Period>"
            "<ForsMomsEjAnnan>100000</ForsMomsEjAnnan><MomsUtgHog>25000</MomsUtgHog>"
            "<MomsIngAvdr>1000</MomsIngAvdr><MomsBetala>24000</MomsBetala></Moms></eSKDUpload>\n");
}

TEST(EskdWriter, NegativeAmountIsRefund) {
  auto vat_return = example_1();
  vat_return.m_boxes[49] = kronor(-30000);
  EXPECT_NE(to_eskd(vat_return).value().find("<MomsBetala>-30000</MomsBetala>"), std::string::npos);
}

TEST(EskdWriter, NoteIsLatin1AndEscaped) {
  auto vat_return = example_1();
  vat_return.m_note = "Rättelse <Q1> & mer";
  EXPECT_NE(to_eskd(vat_return).value().find("<TextUpplysningMoms>R\xE4ttelse &lt;Q1&gt; &amp; mer</TextUpplysningMoms>"),
            std::string::npos);
}

TEST(EskdWriter, Rejects) {
  auto check = [](auto change) {
    auto vat_return = example_1();
    change(vat_return);
    return to_eskd(vat_return).has_value();
  };
  EXPECT_FALSE(check([](VatReturn& r) { r.m_org_nr = "5560000175"; }));
  EXPECT_FALSE(check([](VatReturn& r) { r.m_org_nr = "556000-017X"; }));
  EXPECT_FALSE(check([](VatReturn& r) { r.m_boxes.erase(49); }));
  EXPECT_FALSE(check([](VatReturn& r) { r.m_boxes[9] = kronor(1); }));
  EXPECT_FALSE(check([](VatReturn& r) { r.m_boxes[48] = Amount::from_cents(199750); }));
  EXPECT_FALSE(check([](VatReturn& r) { r.m_note = std::string(301, 'x'); }));
  EXPECT_FALSE(check([](VatReturn& r) { r.m_note = "€"; }));
  EXPECT_TRUE(check([](VatReturn& r) { r.m_note = std::string(300, 'x'); }));
}

TEST(EskdReader, SkvExamples) {
  EXPECT_EQ(read_eskd_file(testdata / "skv_example_1.eskd").value(), example_1());

  auto const refund = read_eskd_file(testdata / "skv_example_2.eskd").value();
  EXPECT_EQ(refund.m_boxes.at(49), kronor(-30000));

  auto const all = read_eskd_file(testdata / "skv_example_6.eskd").value();
  EXPECT_EQ(all.m_boxes.size(), 29u);
  EXPECT_EQ(all.m_note, "Skriv den upplysning du vill skicka in");
}

TEST(EskdReader, Rejects) {
  EXPECT_FALSE(parse_eskd("<eSKDUpload Version=\"5.0\"></eSKDUpload>").has_value());
  EXPECT_FALSE(parse_eskd("<eSKDUpload Version=\"6.0\"><OrgNr>556000-0175</OrgNr><Moms></Moms></eSKDUpload>")
                   .has_value());  // no Period
  EXPECT_FALSE(parse_eskd("<eSKDUpload Version=\"6.0\"><OrgNr>556000-0175</OrgNr><Moms><Period>202401</Period>"
                          "<Okand>1</Okand></Moms></eSKDUpload>")
                   .has_value());
  EXPECT_FALSE(parse_eskd("<eSKDUpload Version=\"6.0\"><OrgNr>556000-0175</OrgNr><Moms><Period>202401</Period>"
                          "<MomsBetala>1.50</MomsBetala></Moms></eSKDUpload>")
                   .has_value());
}

TEST(Eskd, ElementOrderMatchesSkvCompleteExample) {
  auto const skv_text = read_bytes(testdata / "skv_example_6.eskd");
  auto const written = to_eskd(parse_eskd(skv_text).value()).value();
  EXPECT_EQ(moms_element_names(written), moms_element_names(skv_text));
}
